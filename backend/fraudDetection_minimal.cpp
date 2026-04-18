// ============================================================
//  FRAUD DETECTION ENGINE — Minimal Web Version
//  
//  Stripped down for web automation - no interactive menus
//  Generates data -> Detects fraud -> Saves to file -> Exits
//
//  CORRECTIONS APPLIED:
//  1. Multiple fraud reasons appended with '+' separator
//  2. Edge deduplication with consistent amount tracking
//  3. Chain detection uses && (both must be middlemen)
//  4. Better random number generator (mt19937)
// ============================================================

#include <iostream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <fstream>
#include <iomanip>
#include <ctime>
#include <random>
#include <string>
using namespace std;

// ============================================================
//  TRANSACTION STRUCTURE
// ============================================================
struct Transaction {
    int id, userId, receiverId;
    double amount;
    long timestamp;
    bool isFraud;
    string reason;
    
    Transaction(int i, int u, int r, double a, long t)
        : id(i), userId(u), receiverId(r), amount(a),
          timestamp(t), isFraud(false), reason("NA") {}
};

// ============================================================
//  THRESHOLDS
// ============================================================
const double LARGE_AMT = 50000.0;
const double DAILY_LIMIT = 150000.0;
const int RAPID_COUNT = 5;
const int RAPID_WINDOW = 60;
const int HUB_MIN_IN = 5;
const double CHAIN_RATIO = 0.90;
const int CHAIN_MIN = 3;

// ============================================================
//  RANDOM NUMBER GENERATOR
// ============================================================
mt19937 rng((unsigned)time(0));
int randInt(int lo, int hi) {
    return uniform_int_distribution<int>(lo, hi)(rng);
}
double randDouble(double lo, double hi) {
    return uniform_real_distribution<double>(lo, hi)(rng);
}

// ============================================================
//  UTILITY FUNCTIONS
// ============================================================
string formatTime(long ts) {
    tm* t = localtime(&ts);
    char buf[24];
    strftime(buf, sizeof(buf), "%Y-%m-%d_%H:%M:%S", t);
    return string(buf);
}

string getDate(long ts) {
    tm* t = localtime(&ts);
    char buf[12];
    strftime(buf, sizeof(buf), "%Y-%m-%d", t);
    return string(buf);
}

void markFraud(Transaction& tx, const string& r) {
    tx.isFraud = true;
    if (tx.reason == "NA" || tx.reason.empty())
        tx.reason = r;
    else if (tx.reason.find(r) == string::npos)
        tx.reason += "+" + r;
}

// ============================================================
//  MERGE SORT — O(n log n)
// ============================================================
void mergeStep(vector<Transaction>& arr, int left, int mid, int right) {
    vector<Transaction> leftArr(arr.begin()+left, arr.begin()+mid+1);
    vector<Transaction> rightArr(arr.begin()+mid+1, arr.begin()+right+1);
    int leftIndex = 0, rightIndex = 0, mergeIndex = left;
    while (leftIndex < (int)leftArr.size() && rightIndex < (int)rightArr.size())
        arr[mergeIndex++] = (leftArr[leftIndex].timestamp <= rightArr[rightIndex].timestamp) 
                            ? leftArr[leftIndex++] : rightArr[rightIndex++];
    while (leftIndex < (int)leftArr.size()) arr[mergeIndex++] = leftArr[leftIndex++];
    while (rightIndex < (int)rightArr.size()) arr[mergeIndex++] = rightArr[rightIndex++];
}

void mergeSort(vector<Transaction>& arr, int left, int right) {
    if (left >= right) return;
    int mid = left + (right - left) / 2;
    mergeSort(arr, left, mid);
    mergeSort(arr, mid+1, right);
    mergeStep(arr, left, mid, right);
}

// ============================================================
//  BINARY SEARCH — O(log n)
// ============================================================
int binarySearch(const vector<long>& timestamps, int low, int high, long target) {
    while (low < high) {
        int mid = low + (high - low) / 2;
        if (timestamps[mid] < target) low = mid + 1;
        else high = mid;
    }
    return low;
}

// ============================================================
//  DATA GENERATION
// ============================================================

/**
 * Generates normal (non-fraudulent) transactions with realistic patterns
 * 
 * Distribution Strategy:
 * - 99% small amounts (500-4500) - Normal daily transactions
 * - 1% large amounts (55k-105k) - Seeds some large transactions for testing
 * 
 * Note: The 1% bias is intentional to create realistic fraud rates (~15%)
 * For truly uniform random: use randDouble(500, 105000) instead
 * 
 * Time Complexity: O(n) where n = count
 */
vector<Transaction> generateTransactions(int count) {
    vector<Transaction> transactions;
    long currentTime = time(0);
    
    for (int i = 0; i < count; i++) {
        // Random user from pool of 1000 users
        int userId = randInt(1, 1000);
        
        // 50% chance of withdrawal (receiverId = 0), 50% chance of transfer
        int receiverId = (randInt(0, 1) == 0) ? 0 : randInt(1, 1000);
        
        // Prevent self-transfers (user sending money to themselves)
        if (receiverId && receiverId == userId) 
            receiverId = receiverId % 1000 + 1;
        
        // Bimodal amount distribution (intentional for realistic fraud rates):
        // - 1% chance: Large amount (55k-105k) - Will trigger large transaction detection
        // - 99% chance: Small amount (500-4.5k) - Normal transactions
        double amount = (randInt(0, 99) < 1) ? randDouble(55000, 105000)
                                              : randDouble(500, 4500);
        
        // Timestamp: Random time within last 24 hours (86400 seconds)
        transactions.emplace_back(i+1, userId, receiverId, amount, 
                                  currentTime - randInt(0, 86400));
    }
    return transactions;
}

/**
 * FRAUD PATTERN 1: Large Transaction Fraud
 * 
 * Injects 10 transactions with amounts exceeding the threshold (50k)
 * 
 * Detection Rule: amount > LARGE_AMT (50,000)
 * Expected Result: All 10 transactions flagged as "Large_transaction"
 * 
 * Pattern Characteristics:
 * - Random users from normal pool (1-1000) - Blends with normal data
 * - Guaranteed no self-transfers using do-while loop
 * - Amount range: 55k-105k (clearly above 50k threshold)
 * - Random timestamps within last 24 hours
 * 
 * Time Complexity: O(1) - Fixed 10 iterations
 */
void injectLarge(vector<Transaction>& transactions) {
    long currentTime = time(0);
    
    for (int i = 0; i < 10; i++) {
        int userId = randInt(1, 1000), receiverId;
        
        // Ensure sender ≠ receiver (prevents self-transfer)
        do { 
            receiverId = randInt(1, 1000); 
        } while (receiverId == userId);
        
        // Amount guaranteed to exceed LARGE_AMT threshold (50k)
        transactions.emplace_back((int)transactions.size()+1, userId, receiverId,
                                  randDouble(55000, 105000), 
                                  currentTime - randInt(0, 86400));
    }
}

/**
 * FRAUD PATTERN 2: Rapid Transaction Fraud
 * 
 * Creates 3 users making 6 transactions each in quick succession
 * 
 * Detection Rule: 5+ transactions within 60 seconds
 * Expected Result: All 18 transactions (3 users × 6 txns) flagged as "Rapid_transactions"
 * 
 * Pattern Characteristics:
 * - User IDs: 2001, 2002, 2003 (separate from normal users 1-1000)
 * - Each user makes 6 transactions in 30 seconds (5 seconds apart)
 * - Receivers: 2010-2015 (different receiver for each transaction)
 * - Staggered start times: User 2001 at T+0, 2002 at T+200, 2003 at T+400
 * - This prevents overlap between different users' rapid sequences
 * 
 * Timeline Example (User 2001):
 *   T+0s:  Transaction 1
 *   T+5s:  Transaction 2
 *   T+10s: Transaction 3
 *   T+15s: Transaction 4
 *   T+20s: Transaction 5
 *   T+25s: Transaction 6
 *   Total: 6 transactions in 25 seconds → Triggers detection (≥5 in 60s)
 * 
 * Time Complexity: O(1) - Fixed 18 transactions (3 × 6)
 */
void injectRapid(vector<Transaction>& transactions) {
    for (int userIndex = 0; userIndex < 3; userIndex++) {
        // Separate user ID range (2001-2003) to avoid conflicts
        int userId = 2001 + userIndex;
        
        // Stagger start times by 200 seconds to prevent overlap
        long baseTime = time(0) + userIndex * 200;
        
        // Create 6 rapid transactions (5 seconds apart)
        for (int txnIndex = 0; txnIndex < 6; txnIndex++)
            transactions.emplace_back((int)transactions.size()+1, 
                                      userId, 
                                      2010 + txnIndex,  // Different receiver each time
                                      randDouble(1000, 2500), 
                                      baseTime + txnIndex * 5);  // 5 second intervals
    }
}

/**
 * FRAUD PATTERN 3: Daily Limit Exceeded Fraud
 * 
 * Creates 2 users who exceed daily spending limit in a single day
 * 
 * Detection Rule: Total daily spending > DAILY_LIMIT (150,000)
 * Expected Result: All 6 transactions (2 users × 3 txns) flagged as "Daily_limit_exceeded"
 * 
 * Pattern Characteristics:
 * - User IDs: 2050, 2051 (separate from normal users and rapid users)
 * - Each user makes 3 transactions totaling 160k:
 *   • Transaction 1: 60,000 (cumulative: 60k)
 *   • Transaction 2: 60,000 (cumulative: 120k)
 *   • Transaction 3: 40,000 (cumulative: 160k) → Exceeds 150k limit
 * - All 3 transactions within 20 seconds (same day)
 * - Staggered users by 300 seconds to prevent overlap
 * 
 * Math Verification:
 *   60,000 + 60,000 + 40,000 = 160,000 > 150,000 ✓
 * 
 * Time Complexity: O(1) - Fixed 6 transactions (2 × 3)
 */
void injectDaily(vector<Transaction>& transactions) {
    for (int userIndex = 0; userIndex < 2; userIndex++) {
        // Separate user ID range (2050-2051)
        int userId = 2050 + userIndex;
        
        // Stagger users by 300 seconds
        long currentTime = time(0) + userIndex * 300;
        
        // Transaction 1: 60k (cumulative: 60k)
        transactions.emplace_back((int)transactions.size()+1, userId, 2060, 
                                  60000.0, currentTime);
        
        // Transaction 2: 60k (cumulative: 120k)
        transactions.emplace_back((int)transactions.size()+1, userId, 2061, 
                                  60000.0, currentTime + 10);
        
        // Transaction 3: 40k (cumulative: 160k > 150k limit) → Triggers detection
        transactions.emplace_back((int)transactions.size()+1, userId, 2062, 
                                  40000.0, currentTime + 20);
    }
}

/**
 * FRAUD PATTERN 4: Network Fraud (Rings, Hubs, Chains)
 * 
 * Creates 3 types of network fraud patterns:
 * A) Fraud Rings - Circular money flow (money laundering)
 * B) Fraud Hubs - Money collection points (Ponzi schemes)
 * C) Fraud Chains - Sequential money passing (money mules)
 * 
 * Total: 48 transactions (15 ring + 18 hub + 15 chain)
 * 
 * ID Allocation Strategy:
 * - Rings:  3001-3025 (3 rings × 5 nodes)
 * - Hubs:   3030-3056 (3 hubs × 7 nodes each: 1 hub + 6 spokes)
 * - Chains: 3060-3085 (3 chains × 6 nodes)
 * 
 * Note: Patterns are explicitly written (not looped) for clarity.
 * This makes it easier to understand and debug each fraud pattern.
 * The repetition is intentional for academic/teaching purposes.
 * 
 * Time Complexity: O(1) - Fixed 48 transactions
 */
void injectNetwork(vector<Transaction>& transactions) {
    long currentTime = time(0);
    
    // ═══════════════════════════════════════════════════════════
    // PATTERN A: FRAUD RINGS (Circular Money Flow)
    // ═══════════════════════════════════════════════════════════
    // Detection: DFS finds cycles in transaction graph
    // Characteristic: Money flows in a circle (A→B→C→D→A)
    // Real-world: Money laundering, hiding transaction origins
    
    // Ring 1: 3001→3002→3003→3004→3005→3001 (5-node cycle)
    int ring1[] = {3001, 3002, 3003, 3004, 3005};
    for (int nodeIndex = 0; nodeIndex < 5; nodeIndex++)
        transactions.emplace_back((int)transactions.size()+1, 
                                  ring1[nodeIndex],           // Sender
                                  ring1[(nodeIndex+1)%5],     // Receiver (modulo creates cycle)
                                  25000.0, 
                                  currentTime + nodeIndex * 5);
    
    // Ring 2: 3011→3012→3013→3014→3015→3011 (5-node cycle)
    int ring2[] = {3011, 3012, 3013, 3014, 3015};
    for (int nodeIndex = 0; nodeIndex < 5; nodeIndex++)
        transactions.emplace_back((int)transactions.size()+1, 
                                  ring2[nodeIndex], 
                                  ring2[(nodeIndex+1)%5], 
                                  26000.0, 
                                  currentTime + 50 + nodeIndex * 5);
    
    // Ring 3: 3021→3022→3023→3024→3025→3021 (5-node cycle)
    int ring3[] = {3021, 3022, 3023, 3024, 3025};
    for (int nodeIndex = 0; nodeIndex < 5; nodeIndex++)
        transactions.emplace_back((int)transactions.size()+1, 
                                  ring3[nodeIndex], 
                                  ring3[(nodeIndex+1)%5], 
                                  27000.0, 
                                  currentTime + 100 + nodeIndex * 5);
    
    // ═══════════════════════════════════════════════════════════
    // PATTERN B: FRAUD HUBS (Money Collection Points)
    // ═══════════════════════════════════════════════════════════
    // Detection: Node with inDegree ≥ 5 and outDegree = 0
    // Characteristic: Many users send to one hub, hub never sends out
    // Real-world: Ponzi schemes, pyramid schemes, scam collection accounts
    
    // Hub 1: 6 spokes (3031-3036) → 1 hub (3030)
    // Hub 3030 receives from 6 users but NEVER sends money out
    int spokes1[] = {3031, 3032, 3033, 3034, 3035, 3036};
    for (int spokeIndex = 0; spokeIndex < 6; spokeIndex++)
        transactions.emplace_back((int)transactions.size()+1, 
                                  spokes1[spokeIndex],  // Spoke sends
                                  3030,                 // Hub receives (never sends)
                                  9500.0, 
                                  currentTime + 150 + spokeIndex * 2);
    
    // Hub 2: 6 spokes (3041-3046) → 1 hub (3040)
    int spokes2[] = {3041, 3042, 3043, 3044, 3045, 3046};
    for (int spokeIndex = 0; spokeIndex < 6; spokeIndex++)
        transactions.emplace_back((int)transactions.size()+1, 
                                  spokes2[spokeIndex], 
                                  3040,  // Hub 2
                                  10000.0, 
                                  currentTime + 200 + spokeIndex * 2);
    
    // Hub 3: 6 spokes (3051-3056) → 1 hub (3050)
    int spokes3[] = {3051, 3052, 3053, 3054, 3055, 3056};
    for (int spokeIndex = 0; spokeIndex < 6; spokeIndex++)
        transactions.emplace_back((int)transactions.size()+1, 
                                  spokes3[spokeIndex], 
                                  3050,  // Hub 3
                                  10500.0, 
                                  currentTime + 250 + spokeIndex * 2);
    
    // ═══════════════════════════════════════════════════════════
    // PATTERN C: FRAUD CHAINS (Sequential Money Passing)
    // ═══════════════════════════════════════════════════════════
    // Detection: Middlemen with inDegree=1, outDegree=1, pass-through ≥90%
    // Characteristic: Money passes through multiple accounts sequentially
    // Real-world: Money mules, layering in money laundering
    
    // Chain 1: 3060→3061→3062→3063→3064→3065 (6 nodes, 5 links)
    // Middlemen (3061-3064) receive money and pass ~100% forward
    int chain1[] = {3060, 3061, 3062, 3063, 3064, 3065};
    for (int linkIndex = 0; linkIndex < 5; linkIndex++)
        transactions.emplace_back((int)transactions.size()+1, 
                                  chain1[linkIndex],      // Current node
                                  chain1[linkIndex+1],    // Next node in chain
                                  22000.0,                // Same amount (100% pass-through)
                                  currentTime + 300 + linkIndex * 4);
    
    // Chain 2: 3070→3071→3072→3073→3074→3075 (6 nodes, 5 links)
    int chain2[] = {3070, 3071, 3072, 3073, 3074, 3075};
    for (int linkIndex = 0; linkIndex < 5; linkIndex++)
        transactions.emplace_back((int)transactions.size()+1, 
                                  chain2[linkIndex], 
                                  chain2[linkIndex+1], 
                                  23000.0, 
                                  currentTime + 350 + linkIndex * 4);
    
    // Chain 3: 3080→3081→3082→3083→3084→3085 (6 nodes, 5 links)
    int chain3[] = {3080, 3081, 3082, 3083, 3084, 3085};
    for (int linkIndex = 0; linkIndex < 5; linkIndex++)
        transactions.emplace_back((int)transactions.size()+1, 
                                  chain3[linkIndex], 
                                  chain3[linkIndex+1], 
                                  24000.0, 
                                  currentTime + 400 + linkIndex * 4);
}

// ============================================================
//  DETECTION RULES
// ============================================================
void detectLarge(vector<Transaction>& transactions) {
    for (auto& transaction : transactions)
        if (transaction.amount > LARGE_AMT)
            markFraud(transaction, "Large_transaction");
}

void detectRapid(vector<Transaction>& transactions) {
    unordered_map<int, vector<int>> transactionsByUser;
    for (int txnIndex = 0; txnIndex < (int)transactions.size(); txnIndex++)
        transactionsByUser[transactions[txnIndex].userId].push_back(txnIndex);
    
    for (auto& userEntry : transactionsByUser) {
        vector<int>& transactionIndices = userEntry.second;
        vector<long> timestamps;
        for (int index : transactionIndices) 
            timestamps.push_back(transactions[index].timestamp);
        
        int leftPointer = 0;
        for (int rightPointer = 0; rightPointer < (int)transactionIndices.size(); rightPointer++) {
            leftPointer = binarySearch(timestamps, leftPointer, rightPointer, 
                                       timestamps[rightPointer] - RAPID_WINDOW);
            if (rightPointer - leftPointer + 1 >= RAPID_COUNT)
                for (int windowIndex = leftPointer; windowIndex <= rightPointer; windowIndex++)
                    markFraud(transactions[transactionIndices[windowIndex]], "Rapid_transactions");
        }
    }
}

void detectDaily(vector<Transaction>& transactions) {
    unordered_map<string, double> dailySpendByUser;
    for (auto& transaction : transactions) {
        string userDateKey = to_string(transaction.userId) + "_" + getDate(transaction.timestamp);
        dailySpendByUser[userDateKey] += transaction.amount;
        if (dailySpendByUser[userDateKey] > DAILY_LIMIT)
            markFraud(transaction, "Daily_limit_exceeded");
    }
}

void dfs(int node, unordered_map<int, vector<int>>& adjacencyList,
         unordered_map<int, int>& nodeColor, vector<int>& dfsStack,
         unordered_map<int, int>& stackPosition, unordered_set<int>& ringNodes) {
    nodeColor[node] = 1;  // Mark as visiting (gray)
    stackPosition[node] = (int)dfsStack.size();
    dfsStack.push_back(node);
    
    for (int neighbor : adjacencyList[node]) {
        if (nodeColor[neighbor] == 0) {  // Not visited (white)
            dfs(neighbor, adjacencyList, nodeColor, dfsStack, stackPosition, ringNodes);
        } else if (nodeColor[neighbor] == 1) {  // Back edge found (cycle detected)
            int cycleStart = stackPosition[neighbor];
            if ((int)dfsStack.size() - cycleStart >= 3)  // Ring must have 3+ nodes
                for (int stackIndex = cycleStart; stackIndex < (int)dfsStack.size(); stackIndex++)
                    ringNodes.insert(dfsStack[stackIndex]);
        }
    }
    
    dfsStack.pop_back();
    stackPosition.erase(node);
    nodeColor[node] = 2;  // Mark as visited (black)
}

void detectNetwork(vector<Transaction>& transactions) {
    unordered_map<int, vector<int>> adjacencyList;
    unordered_map<int, int> inDegree, outDegree;
    unordered_map<int, double> totalSent, totalReceived;
    unordered_set<string> seenEdges;
    
    // Build graph with unique edges and sum amounts per edge
    unordered_map<string, double> edgeAmounts;
    
    for (auto& transaction : transactions) {
        if (transaction.receiverId == 0) continue;
        string edgeKey = to_string(transaction.userId) + "_" + to_string(transaction.receiverId);
        
        // Sum amounts for this unique edge
        edgeAmounts[edgeKey] += transaction.amount;
        
        // Only add edge structure once
        if (!seenEdges.count(edgeKey)) {
            adjacencyList[transaction.userId].push_back(transaction.receiverId);
            inDegree[transaction.receiverId]++;
            outDegree[transaction.userId]++;
            seenEdges.insert(edgeKey);
        }
    }
    
    // Calculate totals using summed edge amounts
    for (auto& edgeEntry : edgeAmounts) {
        size_t separatorPos = edgeEntry.first.find('_');
        int senderId = stoi(edgeEntry.first.substr(0, separatorPos));
        int receiverId = stoi(edgeEntry.first.substr(separatorPos + 1));
        totalSent[senderId] += edgeEntry.second;
        totalReceived[receiverId] += edgeEntry.second;
    }
    
    unordered_set<int> allNodes;
    for (auto& entry : adjacencyList) allNodes.insert(entry.first);
    for (auto& entry : inDegree) allNodes.insert(entry.first);
    
    // Ring detection using DFS
    unordered_set<int> ringNodes;
    unordered_map<int,int> nodeColor;
    vector<int> dfsStack;
    unordered_map<int,int> stackPosition;
    for (int node : allNodes)
        if (nodeColor[node] == 0)
            dfs(node, adjacencyList, nodeColor, dfsStack, stackPosition, ringNodes);
    
    // Hub detection
    unordered_set<int> hubNodes, spokeNodes;
    for (auto& degreeEntry : inDegree) {
        int node = degreeEntry.first;
        if (degreeEntry.second >= HUB_MIN_IN && outDegree[node] == 0) {
            hubNodes.insert(node);
            for (auto& transaction : transactions)
                if (transaction.receiverId == node)
                    spokeNodes.insert(transaction.userId);
        }
    }
    
    // Chain detection - find middlemen nodes
    unordered_set<int> chainMiddlemen;
    for (auto& sentEntry : totalSent) {
        int node = sentEntry.first;
        if (!totalReceived.count(node)) continue;
        if (ringNodes.count(node)) continue;
        if (hubNodes.count(node)) continue;
        if (spokeNodes.count(node)) continue;
        if (inDegree[node] != 1) continue;
        if (outDegree[node] != 1) continue;
        if (sentEntry.second / totalReceived[node] >= CHAIN_RATIO)
            chainMiddlemen.insert(node);
    }
    
    // Link chain middlemen - both sender AND receiver must be middlemen
    unordered_map<int,int> chainNext, chainPrevious;
    for (auto& transaction : transactions) {
        if (chainMiddlemen.count(transaction.userId) && chainMiddlemen.count(transaction.receiverId)) {
            chainNext[transaction.userId] = transaction.receiverId;
            chainPrevious[transaction.receiverId] = transaction.userId;
        }
    }
    
    // Walk chains to find sequences of 3+ middlemen
    unordered_set<int> chainNodes, visitedNodes;
    for (int node : chainMiddlemen) {
        if (visitedNodes.count(node)) continue;
        int chainHead = node;
        while (chainPrevious.count(chainHead) && !visitedNodes.count(chainPrevious[chainHead]))
            chainHead = chainPrevious[chainHead];
        vector<int> chainSequence;
        int currentNode = chainHead;
        while (chainMiddlemen.count(currentNode) && !visitedNodes.count(currentNode)) {
            chainSequence.push_back(currentNode);
            visitedNodes.insert(currentNode);
            if (!chainNext.count(currentNode)) break;
            currentNode = chainNext[currentNode];
        }
        if ((int)chainSequence.size() >= CHAIN_MIN)
            for (int chainNode : chainSequence) chainNodes.insert(chainNode);
    }
    
    // Flag transactions involved in network fraud
    for (auto& transaction : transactions) {
        if (transaction.receiverId == 0) continue;
        int senderId = transaction.userId, receiverId = transaction.receiverId;
        if (ringNodes.count(senderId) || ringNodes.count(receiverId))
            markFraud(transaction, "Fraud_ring");
        else if (hubNodes.count(receiverId) || spokeNodes.count(senderId))
            markFraud(transaction, "Fraud_hub");
        else if (chainNodes.count(senderId) || chainNodes.count(receiverId))
            markFraud(transaction, "Fraud_chain");
    }
}

void detectFraud(vector<Transaction>& transactions) {
    detectLarge(transactions);
    detectRapid(transactions);
    detectDaily(transactions);
    detectNetwork(transactions);
}

// ============================================================
//  FILE OUTPUT
// ============================================================
void saveToFile(const vector<Transaction>& transactions) {
    ofstream outputFile("transactions.txt");
    outputFile << left
               << setw(6) << "ID"
               << setw(8) << "USER"
               << setw(10) << "RECEIVER"
               << setw(12) << "AMOUNT"
               << setw(22) << "TIMESTAMP"
               << setw(6) << "FRAUD"
               << "REASON\n"
               << string(90, '-') << "\n";
    for (auto& transaction : transactions)
        outputFile << setw(6) << transaction.id
                   << setw(8) << transaction.userId
                   << setw(10) << transaction.receiverId
                   << setw(12) << fixed << setprecision(2) << transaction.amount
                   << setw(22) << formatTime(transaction.timestamp)
                   << setw(6) << (transaction.isFraud ? "YES" : "NO")
                   << transaction.reason << "\n";
}

// ============================================================
//  MAIN — Streamlined for web automation
// ============================================================
int main() {
    // ═══════════════════════════════════════════════════════════
    // STEP 1: Generate Base Transactions
    // ═══════════════════════════════════════════════════════════
    // Creates 500 normal transactions with realistic patterns
    // - 99% small amounts (500-4500)
    // - 1% large amounts (55k-105k) for testing
    vector<Transaction> transactions = generateTransactions(500);
    
    // ═══════════════════════════════════════════════════════════
    // STEP 2: Inject Fraud Patterns
    // ═══════════════════════════════════════════════════════════
    // Add known fraud patterns to test detection algorithms
    
    injectLarge(transactions);     // +10 txns: Large transaction fraud
    injectRapid(transactions);     // +18 txns: Rapid transaction fraud (3 users × 6)
    injectDaily(transactions);     // +6 txns:  Daily limit fraud (2 users × 3)
    injectNetwork(transactions);   // +48 txns: Network fraud (3 rings + 3 hubs + 3 chains)
    
    // Total: 582 transactions (500 normal + 82 fraud patterns)
    
    // ═══════════════════════════════════════════════════════════
    // STEP 3: Sort by Timestamp (Required for Detection)
    // ═══════════════════════════════════════════════════════════
    // DAA #1: Merge Sort - O(n log n)
    // Sorts all transactions chronologically
    // Required for: Rapid detection (sliding window), Daily limit (accumulation)
    mergeSort(transactions, 0, (int)transactions.size() - 1);
    
    // ═══════════════════════════════════════════════════════════
    // STEP 4: Run Fraud Detection Algorithms
    // ═══════════════════════════════════════════════════════════
    // Applies all 4 detection rules:
    // 1. Large Transaction Detection - O(n)
    // 2. Rapid Transaction Detection - O(n log n)
    // 3. Daily Limit Detection - O(n)
    // 4. Network Fraud Detection - O(V + E)
    detectFraud(transactions);
    
    // ═══════════════════════════════════════════════════════════
    // STEP 5: Save Results to File
    // ═══════════════════════════════════════════════════════════
    // Writes to transactions.txt for web backend to read
    // Format: ID | USER | RECEIVER | AMOUNT | TIMESTAMP | FRAUD | REASON
    saveToFile(transactions);
    
    return 0;
}

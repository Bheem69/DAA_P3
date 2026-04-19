// ============================================================
//  FRAUD DETECTION ENGINE
//  Academic Version — Clean & Professional
//
//  Uses: vector, map, set (STL containers)
//  Avoids: complex templates, fancy C++ tricks
//
//  4 Fraud Rules:
//  1. Big Amount    — single transaction > 50,000
//  2. Rapid Sending — 5+ sends within 60 seconds
//  3. Daily Limit   — total sent in a day > 150,000
//  4. Network Fraud — Rings, Hubs, Chains
//
//  Algorithms:
//  - Merge Sort   O(n log n) — sort by timestamp
//  - Sliding Window O(n)    — rapid detection
//  - DFS          O(V+E)    — ring detection
//  - Hash Maps    O(1)      — fast lookups
// ============================================================

#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <fstream>
#include <iomanip>
#include <cstdlib>
#include <ctime>
#include <string>
#include <sstream>
#include <algorithm>
using namespace std;

// ============================================================
//  THRESHOLDS — tweak these to experiment
// ============================================================
const double BIG_AMOUNT   = 50000.0;
const double DAILY_LIMIT  = 150000.0;
const int    RAPID_COUNT  = 5;
const int    RAPID_WINDOW = 60;       // seconds
const int    HUB_MIN_IN   = 5;        // min people sending to a hub
const double CHAIN_RATIO  = 0.90;     // pass 90%+ = chain middleman
const int    CHAIN_MIN    = 3;        // min nodes to be a chain

// ============================================================
//  TRANSACTION STRUCT
// ============================================================
struct Transaction {
    int    id;
    int    sender;
    int    receiver;    // 0 = cash withdrawal
    double amount;
    long   timestamp;
    bool   isFraud;
    string reason;
    
    Transaction(int id, int sender, int receiver, double amount, long timestamp)
        : id(id), sender(sender), receiver(receiver),
          amount(amount), timestamp(timestamp),
          isFraud(false), reason("NA") {}
};

// ============================================================
//  SIMPLE RANDOM HELPERS (basic rand())
// ============================================================
int randInt(int lo, int hi) {
    return lo + rand() % (hi - lo + 1);
}

double randAmount(double lo, double hi) {
    return lo + (rand() / (double)RAND_MAX) * (hi - lo);
}

// ============================================================
//  TIME HELPERS
// ============================================================
string getDateTime(long ts) {
    struct tm* t = localtime(&ts);
    char buf[20];
    strftime(buf, sizeof(buf), "%Y-%m-%d_%H:%M:%S", t);
    return string(buf);
}

string getDate(long ts) {
    struct tm* t = localtime(&ts);
    char buf[12];
    strftime(buf, sizeof(buf), "%Y-%m-%d", t);
    return string(buf);
}

// ============================================================
//  MARK FRAUD — appends reason if transaction already flagged
// ============================================================
void markFraud(Transaction& tx, const string& reason) {
    tx.isFraud = true;
    if (tx.reason == "NA")
        tx.reason = reason;
    else if (tx.reason.find(reason) == string::npos)
        tx.reason += "+" + reason;
}

// ============================================================
//  MERGE SORT — sorts transactions oldest to newest
//  Time Complexity: O(n log n)
// ============================================================
void mergeHalves(vector<Transaction>& txns, int left, int mid, int right) {
    vector<Transaction> L(txns.begin() + left,  txns.begin() + mid + 1);
    vector<Transaction> R(txns.begin() + mid + 1, txns.begin() + right + 1);
    
    int i = 0, j = 0, k = left;
    while (i < (int)L.size() && j < (int)R.size())
        txns[k++] = (L[i].timestamp <= R[j].timestamp) ? L[i++] : R[j++];
    while (i < (int)L.size()) txns[k++] = L[i++];
    while (j < (int)R.size()) txns[k++] = R[j++];
}

void mergeSort(vector<Transaction>& txns, int left, int right) {
    if (left >= right) return;
    int mid = (left + right) / 2;
    mergeSort(txns, left, mid);
    mergeSort(txns, mid + 1, right);
    mergeHalves(txns, left, mid, right);
}

// ============================================================
//  STEP 1 — GENERATE RANDOM TRANSACTIONS
//
//  Creates 'count' random normal transactions.
//  Users: 1 to 1000
//  Amounts: 500 to 4500 (normal everyday amounts)
//  Time: random within last 24 hours
// ============================================================
vector<Transaction> generateTransactions(int count) {
    vector<Transaction> txns;
    long now = time(0);
    
    for (int i = 0; i < count; i++) {
        int sender   = randInt(1, 1000);
        int receiver = randInt(0, 1000);   // 0 = ATM withdrawal
        if (receiver == sender)            // no self-transfers
            receiver = (sender % 1000) + 1;
        
        // 99% small amounts, 1% large amounts (for realistic fraud rate)
        double amount = (randInt(0, 99) < 1) ? randAmount(55000, 105000)
                                              : randAmount(500, 4500);
        
        long when = now - randInt(0, 86400);  // last 24 hours
        txns.emplace_back((int)txns.size() + 1, sender, receiver, amount, when);
    }
    return txns;
}

// ============================================================
//  STEP 2 — INJECT FRAUD PATTERNS
//  We plant known fraud so we can verify detection works.
// ============================================================

// Rule 1 test: 10 transactions with amounts above 50,000
void injectBigAmounts(vector<Transaction>& txns) {
    long now = time(0);
    for (int i = 0; i < 10; i++) {
        int sender   = randInt(1, 1000);
        int receiver = randInt(1, 1000);
        if (receiver == sender) receiver = (sender % 1000) + 1;
        
        double bigAmt = randAmount(55000, 120000);
        txns.emplace_back((int)txns.size() + 1, sender, receiver,
                          bigAmt, now - randInt(0, 3600));
    }
}

// Rule 2 test: 3 users each send 6 times in 30 seconds
// Uses user IDs 2001, 2002, 2003 (outside normal 1-1000 range)
void injectRapidSending(vector<Transaction>& txns) {
    long now = time(0);
    for (int u = 0; u < 3; u++) {
        int  sender   = 2001 + u;
        long baseTime = now + u * 200;   // stagger each user
        
        for (int i = 0; i < 6; i++) {
            int receiver = 2010 + i;
            txns.emplace_back((int)txns.size() + 1, sender, receiver,
                              randAmount(500, 2000), baseTime + i * 5);
        }
    }
}

// Rule 3 test: 2 users each spend 160,000 in one day (limit = 150,000)
// 60k + 60k + 40k = 160k total → triggers detection
// Uses user IDs 2050, 2051
void injectDailyLimit(vector<Transaction>& txns) {
    long now = time(0);
    for (int u = 0; u < 2; u++) {
        int  sender   = 2050 + u;
        long baseTime = now + u * 300;
        
        txns.emplace_back((int)txns.size() + 1, sender, 2060, 60000.0, baseTime);
        txns.emplace_back((int)txns.size() + 1, sender, 2061, 60000.0, baseTime + 10);
        txns.emplace_back((int)txns.size() + 1, sender, 2062, 40000.0, baseTime + 20);
    }
}

// Rule 4 test: 3 rings + 3 hubs + 3 chains
// All use user IDs 3001+ to stay separate from normal users
void injectNetworkFraud(vector<Transaction>& txns) {
    long now = time(0);
    
    // RINGS — money flows in a circle: A→B→C→D→E→A
    int rings[3][5] = {
        {3001, 3002, 3003, 3004, 3005},
        {3011, 3012, 3013, 3014, 3015},
        {3021, 3022, 3023, 3024, 3025}
    };
    for (int r = 0; r < 3; r++)
        for (int i = 0; i < 5; i++)
            txns.emplace_back((int)txns.size() + 1,
                              rings[r][i], rings[r][(i+1) % 5],
                              25000.0, now + r * 60 + i * 5);
    
    // HUBS — many people send to one collector who never sends out
    int hubs[3]      = {3030, 3040, 3050};
    int spokeBase[3] = {3031, 3041, 3051};
    for (int h = 0; h < 3; h++)
        for (int s = 0; s < 6; s++)
            txns.emplace_back((int)txns.size() + 1,
                              spokeBase[h] + s, hubs[h],
                              10000.0, now + 200 + h * 60 + s * 2);
    
    // CHAINS — money passed along a line: A→B→C→D→E→F
    int chains[3][6] = {
        {3060, 3061, 3062, 3063, 3064, 3065},
        {3070, 3071, 3072, 3073, 3074, 3075},
        {3080, 3081, 3082, 3083, 3084, 3085}
    };
    for (int c = 0; c < 3; c++)
        for (int i = 0; i < 5; i++)
            txns.emplace_back((int)txns.size() + 1,
                              chains[c][i], chains[c][i + 1],
                              22000.0, now + 400 + c * 60 + i * 4);
}

// ============================================================
//  RULE 1 — BIG AMOUNT DETECTION
//  Simply check if a single transaction is above the limit.
//  Time Complexity: O(n)
// ============================================================
void detectBigAmount(vector<Transaction>& txns) {
    for (auto& tx : txns)
        if (tx.amount > BIG_AMOUNT)
            markFraud(tx, "Large_transaction");
}

// ============================================================
//  RULE 2 — RAPID SENDING DETECTION
//  Group each user's transactions. Use a sliding window to
//  count how many they sent within any 60-second period.
//  If 5 or more → flag them all.
//  Time Complexity: O(n log n)
// ============================================================
void detectRapidSending(vector<Transaction>& txns) {
    // Group transaction indices by sender
    map<int, vector<int>> byUser;
    for (int i = 0; i < (int)txns.size(); i++)
        byUser[txns[i].sender].push_back(i);
    
    // Check each user's transactions
    for (auto& entry : byUser) {
        vector<int>& indices = entry.second;
        
        // Collect timestamps for this user (already sorted globally)
        vector<long> times;
        for (int idx : indices)
            times.push_back(txns[idx].timestamp);
        
        // Sliding window: left moves forward to stay within 60 seconds
        int left = 0;
        for (int right = 0; right < (int)indices.size(); right++) {
            // Move left pointer until window is within 60 seconds
            while (times[right] - times[left] > RAPID_WINDOW)
                left++;
            
            // If window has 5+ transactions, flag all of them
            if (right - left + 1 >= RAPID_COUNT)
                for (int k = left; k <= right; k++)
                    markFraud(txns[indices[k]], "Rapid_transactions");
        }
    }
}

// ============================================================
//  RULE 3 — DAILY LIMIT DETECTION
//  For each user, track total amount sent per day.
//  As soon as the running total exceeds limit → flag.
//  Time Complexity: O(n)
// ============================================================
void detectDailyLimit(vector<Transaction>& txns) {
    // Key = "userID_date", Value = total amount sent that day
    map<string, double> dailyTotal;
    
    for (auto& tx : txns) {
        string key = to_string(tx.sender) + "_" + getDate(tx.timestamp);
        dailyTotal[key] += tx.amount;
        if (dailyTotal[key] > DAILY_LIMIT)
            markFraud(tx, "Daily_limit_exceeded");
    }
}

// ============================================================
//  RULE 4 — NETWORK FRAUD DETECTION
//  Build a graph of who sends to whom.
//  Find Rings (cycles), Hubs (collectors), Chains (mules).
//  Time Complexity: O(V + E)
// ============================================================

// DFS helper — finds nodes that form cycles (rings)
// Uses 3 colors: 0=not visited, 1=currently visiting, 2=done
void dfs(int node,
         map<int, vector<int>>& adj,
         map<int, int>& color,
         vector<int>& stack,
         map<int, int>& stackPos,
         set<int>& ringNodes) {
    color[node] = 1;                        // mark as visiting
    stackPos[node] = (int)stack.size();
    stack.push_back(node);
    
    for (int neighbor : adj[node]) {
        if (color[neighbor] == 0) {
            dfs(neighbor, adj, color, stack, stackPos, ringNodes);
        } else if (color[neighbor] == 1) {
            // Back edge = cycle found
            int start = stackPos[neighbor];
            if ((int)stack.size() - start >= 3)   // ring needs 3+ nodes
                for (int i = start; i < (int)stack.size(); i++)
                    ringNodes.insert(stack[i]);
        }
    }
    
    stack.pop_back();
    stackPos.erase(node);
    color[node] = 2;                        // mark as done
}

void detectNetworkFraud(vector<Transaction>& txns) {
    // ── Build graph ─────────────────────────────────────────
    map<int, vector<int>> adj;         // adjacency list
    map<int, int>  inCount, outCount;  // in/out degree per node
    map<string, double> edgeAmount;    // total amount per sender→receiver edge
    set<string> seenEdges;
    
    for (auto& tx : txns) {
        if (tx.receiver == 0) continue;
        
        string edgeKey = to_string(tx.sender) + "_" + to_string(tx.receiver);
        edgeAmount[edgeKey] += tx.amount;
        
        if (seenEdges.find(edgeKey) == seenEdges.end()) {
            adj[tx.sender].push_back(tx.receiver);
            inCount[tx.receiver]++;
            outCount[tx.sender]++;
            seenEdges.insert(edgeKey);
        }
    }
    
    // Total sent and received per user
    map<int, double> totalSent, totalReceived;
    for (auto& entry : edgeAmount) {
        size_t sep = entry.first.find('_');
        int from   = stoi(entry.first.substr(0, sep));
        int to     = stoi(entry.first.substr(sep + 1));
        totalSent[from]     += entry.second;
        totalReceived[to]   += entry.second;
    }
    
    // Collect all unique nodes
    set<int> allNodes;
    for (auto& entry : adj)      allNodes.insert(entry.first);
    for (auto& entry : inCount)  allNodes.insert(entry.first);
    
    // ── Ring Detection (DFS cycle detection) ────────────────
    set<int> ringNodes;
    map<int, int> color;
    vector<int> stack;
    map<int, int> stackPos;
    for (int node : allNodes)
        if (color[node] == 0)
            dfs(node, adj, color, stack, stackPos, ringNodes);
    
    // ── Hub Detection ────────────────────────────────────────
    // Hub = receives from many but never sends
    set<int> hubNodes, spokeNodes;
    for (auto& entry : inCount) {
        int node = entry.first;
        if (entry.second >= HUB_MIN_IN && outCount[node] == 0) {
            hubNodes.insert(node);
            for (auto& tx : txns)
                if (tx.receiver == node)
                    spokeNodes.insert(tx.sender);
        }
    }
    
    // ── Chain Detection ──────────────────────────────────────
    // Middleman = receives from 1, sends to 1, passes 90%+ forward
    set<int> middlemen;
    for (auto& entry : totalSent) {
        int node = entry.first;
        if (ringNodes.count(node))        continue;
        if (hubNodes.count(node))         continue;
        if (spokeNodes.count(node))       continue;
        if (inCount[node]  != 1)          continue;
        if (outCount[node] != 1)          continue;
        if (!totalReceived.count(node))   continue;
        
        double ratio = entry.second / totalReceived[node];
        if (ratio >= CHAIN_RATIO)
            middlemen.insert(node);
    }
    
    // Link middlemen: find who each middleman sends to
    map<int,int> chainNext, chainPrev;
    for (auto& tx : txns)
        if (middlemen.count(tx.sender) && middlemen.count(tx.receiver)) {
            chainNext[tx.sender]   = tx.receiver;
            chainPrev[tx.receiver] = tx.sender;
        }
    
    // Walk chains — flag sequences of 3+ middlemen
    set<int> chainNodes, visited;
    for (int node : middlemen) {
        if (visited.count(node)) continue;
        
        // Walk back to the start of this chain
        int head = node;
        while (chainPrev.count(head) && !visited.count(chainPrev[head]))
            head = chainPrev[head];
        
        // Walk forward and collect the chain
        vector<int> chain;
        int cur = head;
        while (middlemen.count(cur) && !visited.count(cur)) {
            chain.push_back(cur);
            visited.insert(cur);
            if (!chainNext.count(cur)) break;
            cur = chainNext[cur];
        }
        
        if ((int)chain.size() >= CHAIN_MIN)
            for (int n : chain)
                chainNodes.insert(n);
    }
    
    // ── Flag Transactions ────────────────────────────────────
    for (auto& tx : txns) {
        if (tx.receiver == 0) continue;
        if (ringNodes.count(tx.sender) || ringNodes.count(tx.receiver))
            markFraud(tx, "Fraud_ring");
        else if (hubNodes.count(tx.receiver) || spokeNodes.count(tx.sender))
            markFraud(tx, "Fraud_hub");
        else if (chainNodes.count(tx.sender) || chainNodes.count(tx.receiver))
            markFraud(tx, "Fraud_chain");
    }
}

// ============================================================
//  RUN ALL DETECTION RULES
// ============================================================
void detectFraud(vector<Transaction>& txns) {
    detectBigAmount(txns);
    detectRapidSending(txns);
    detectDailyLimit(txns);
    detectNetworkFraud(txns);
}

// ============================================================
//  SAVE TO FILE
// ============================================================
void saveResults(const vector<Transaction>& txns) {
    ofstream file("transactions.txt");
    file << left
         << setw(6)  << "ID"
         << setw(8)  << "USER"
         << setw(10) << "RECEIVER"
         << setw(12) << "AMOUNT"
         << setw(22) << "TIMESTAMP"
         << setw(6)  << "FRAUD"
         << "REASON\n"
         << string(90, '-') << "\n";
    
    for (const auto& tx : txns)
        file << setw(6)  << tx.id
             << setw(8)  << tx.sender
             << setw(10) << tx.receiver
             << setw(12) << fixed << setprecision(2) << tx.amount
             << setw(22) << getDateTime(tx.timestamp)
             << setw(6)  << (tx.isFraud ? "YES" : "NO")
             << tx.reason << "\n";
}

// ============================================================
//  MAIN
// ============================================================
int main() {
    srand((unsigned)time(0));
    
    // Step 1: Generate 500 random normal transactions
    vector<Transaction> txns = generateTransactions(500);
    
    // Step 2: Inject known fraud patterns for testing
    injectBigAmounts(txns);      // +10  big transactions
    injectRapidSending(txns);    // +18  rapid transactions (3 users x 6)
    injectDailyLimit(txns);      // +6   daily limit fraud (2 users x 3)
    injectNetworkFraud(txns);    // +48  ring + hub + chain fraud
    
    // Total: 582 transactions (500 normal + 82 fraud patterns)
    
    // Step 3: Sort by timestamp (required for rapid + daily detection)
    mergeSort(txns, 0, (int)txns.size() - 1);
    
    // Step 4: Run all fraud detection rules
    detectFraud(txns);
    
    // Step 5: Save results
    saveResults(txns);
    
    return 0;
}

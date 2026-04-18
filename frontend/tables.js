// ============================================================
//  TABLE RENDERING
//  Handles all table display logic
// ============================================================

import { getTransactions, getCurrentFraudFilter, setCurrentFraudFilter } from "./state.js";

// Render all transactions table
export function renderTransactionsTable() {
    const transactions = getTransactions();
    const tbody = document.getElementById("transactionTable");

    if (transactions.length === 0) {
        tbody.innerHTML = '<tr><td colspan="7" class="text-center py-12 text-gray-400">No transactions to display</td></tr>';
        return;
    }

    tbody.innerHTML = transactions.map(tx => createTransactionRow(tx)).join("");
}

// Render fraud alerts table
export function renderFraudTable(filterReason = null) {
    const transactions = getTransactions();
    const tbody = document.getElementById("fraudTable");
    let fraudTransactions = transactions.filter(tx => tx.fraud);
    
    // Apply filter if specified
    if (filterReason) {
        fraudTransactions = fraudTransactions.filter(tx => tx.reason === filterReason);
    }

    if (fraudTransactions.length === 0) {
        const message = filterReason 
            ? `No fraud detected for type: ${filterReason.replace(/_/g, " ")}`
            : "No fraud detected";
        tbody.innerHTML = `<tr><td colspan="6" class="text-center py-12 text-gray-400">${message}</td></tr>`;
        return;
    }

    tbody.innerHTML = fraudTransactions.map(tx => createFraudRow(tx)).join("");
}

// Render filtered search results
export function renderFilteredResults(transactions) {
    const tbody = document.getElementById("transactionTable");

    if (transactions.length === 0) {
        tbody.innerHTML = '<tr><td colspan="7" class="text-center py-12 text-gray-400">No matching transactions found</td></tr>';
        return;
    }

    tbody.innerHTML = transactions.map(tx => createTransactionRow(tx)).join("");
}

// Filter fraud by reason
export function filterFraudByReason(reason) {
    setCurrentFraudFilter(reason);
    renderFraudTable(reason);
}

// Clear fraud filter
export function clearFraudFilter() {
    setCurrentFraudFilter(null);
    renderFraudTable();
    
    // Remove active state from all filter buttons
    document.querySelectorAll(".fraud-filter-btn").forEach(btn => {
        btn.classList.remove("bg-indigo-600", "border-indigo-500");
        btn.classList.add("bg-gray-800", "border-gray-700");
    });
    
    // Hide clear filter button
    document.getElementById("clearFilterBtn").classList.add("hidden");
}

// Search transactions
export function searchTransactions() {
    const searchTerm = document.getElementById("searchInput").value.trim();
    
    if (!searchTerm) {
        renderTransactionsTable();
        return;
    }

    const transactions = getTransactions();
    const searchNum = parseInt(searchTerm);
    const filtered = transactions.filter(tx => 
        tx.id === searchNum || tx.user === searchNum || tx.receiver === searchNum
    );

    renderFilteredResults(filtered);
}

// Clear search
export function clearSearch() {
    document.getElementById("searchInput").value = "";
    renderTransactionsTable();
}

// ============================================================
//  HELPER FUNCTIONS - Create table rows
// ============================================================

// Create transaction row HTML
function createTransactionRow(tx) {
    const statusBadge = tx.fraud
        ? '<span class="bg-red-500 text-white px-3 py-1 rounded text-xs font-semibold">FRAUD</span>'
        : '<span class="bg-green-500 text-white px-3 py-1 rounded text-xs font-semibold">SAFE</span>';
    
    const rowClass = tx.fraud ? "bg-red-50/5" : "";
    const amountClass = tx.fraud ? "text-red-400 font-semibold" : "text-gray-200";
    const reason = formatReason(tx.reason);

    return `
        <tr class="${rowClass} hover:bg-gray-800 transition-colors">
            <td class="p-3">#${tx.id}</td>
            <td class="p-3 text-gray-300">${tx.user}</td>
            <td class="p-3 text-gray-300">${tx.receiver}</td>
            <td class="p-3 ${amountClass}">₹${tx.amount.toFixed(2)}</td>
            <td class="p-3 text-gray-400 text-xs">${tx.timestamp}</td>
            <td class="p-3">${statusBadge}</td>
            <td class="p-3 text-xs text-gray-400">${reason}</td>
        </tr>
    `;
}

// Create fraud row HTML
function createFraudRow(tx) {
    const reason = formatReason(tx.reason);

    return `
        <tr class="bg-red-50/5 hover:bg-red-50/10 transition-colors">
            <td class="p-3 text-red-400">#${tx.id}</td>
            <td class="p-3 text-gray-300">${tx.user}</td>
            <td class="p-3 text-gray-300">${tx.receiver}</td>
            <td class="p-3 text-red-400 font-semibold">₹${tx.amount.toFixed(2)}</td>
            <td class="p-3 text-gray-400 text-xs">${tx.timestamp}</td>
            <td class="p-3 text-xs text-gray-400">${reason}</td>
        </tr>
    `;
}

// Format fraud reason for display
function formatReason(reason) {
    if (reason === "NA") return "—";
    // Replace underscores with spaces and '+' with ' + ' for readability
    return reason.replace(/_/g, " ").replace(/\+/g, " + ");
}

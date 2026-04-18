// ============================================================
//  STATE MANAGEMENT
//  Centralized data storage for the application
// ============================================================

export let allTransactions = [];
export let currentView = "dashboard";
export let statusPieChart = null;
export let fraudTypesChart = null;
export let currentFraudFilter = null;

// Update transactions data
export function setTransactions(transactions) {
    allTransactions = transactions;
}

// Get all transactions
export function getTransactions() {
    return allTransactions;
}

// Update current view
export function setCurrentView(view) {
    currentView = view;
}

// Get current view
export function getCurrentView() {
    return currentView;
}

// Update status pie chart reference
export function setStatusPieChart(chart) {
    statusPieChart = chart;
}

// Get status pie chart reference
export function getStatusPieChart() {
    return statusPieChart;
}

// Update fraud types chart reference
export function setFraudTypesChart(chart) {
    fraudTypesChart = chart;
}

// Get fraud types chart reference
export function getFraudTypesChart() {
    return fraudTypesChart;
}

// Update fraud filter
export function setCurrentFraudFilter(filter) {
    currentFraudFilter = filter;
}

// Get fraud filter
export function getCurrentFraudFilter() {
    return currentFraudFilter;
}

// Calculate statistics
export function getStats() {
    const total = allTransactions.length;
    const fraudCount = allTransactions.filter(tx => tx.fraud).length;
    const fraudRate = total > 0 ? ((fraudCount / total) * 100).toFixed(1) : 0;
    
    return { total, fraudCount, fraudRate };
}

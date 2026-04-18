// ============================================================
//  FRAUD DETECTION SYSTEM - Main Application
//  Modular architecture with separated concerns
// ============================================================

import { generateTransactionsAPI, loadTransactionsAPI } from "./api.js";
import { 
    setTransactions, 
    getTransactions, 
    setCurrentView, 
    getCurrentView,
    getCurrentFraudFilter,
    setCurrentFraudFilter,
    getStats 
} from "./state.js";
import { updateCharts } from "./charts.js";
import { 
    renderTransactionsTable, 
    renderFraudTable, 
    filterFraudByReason, 
    clearFraudFilter,
    searchTransactions,
    clearSearch
} from "./tables.js";

// ============================================================
//  EVENT LISTENERS - Setup on page load
// ============================================================

document.getElementById("generateBtn").addEventListener("click", generateTransactions);

document.getElementById("navDashboard").addEventListener("click", () => showView("dashboard"));
document.getElementById("navTransactions").addEventListener("click", () => showView("transactions"));
document.getElementById("navFraudAlerts").addEventListener("click", () => showView("fraudAlerts"));

document.getElementById("searchBtn").addEventListener("click", searchTransactions);
document.getElementById("clearBtn").addEventListener("click", clearSearch);
document.getElementById("clearFilterBtn").addEventListener("click", clearFraudFilter);

// Fraud filter buttons
document.querySelectorAll(".fraud-filter-btn").forEach(btn => {
    btn.addEventListener("click", (e) => {
        const reason = e.target.dataset.reason;
        filterFraudByReason(reason);
        
        // Update button states
        document.querySelectorAll(".fraud-filter-btn").forEach(b => {
            b.classList.remove("bg-indigo-600", "border-indigo-500");
            b.classList.add("bg-gray-800", "border-gray-700");
        });
        e.target.classList.remove("bg-gray-800", "border-gray-700");
        e.target.classList.add("bg-indigo-600", "border-indigo-500");
        
        // Show clear filter button
        document.getElementById("clearFilterBtn").classList.remove("hidden");
    });
});

// Load transactions on page load
document.addEventListener("DOMContentLoaded", async () => {
    const transactions = await loadTransactionsAPI();
    setTransactions(transactions);
    updateStats();
    showView("dashboard");
    if (transactions.length > 0) {
        updateCharts();
    }
});

// ============================================================
//  VIEW MANAGEMENT
// ============================================================

function showView(view) {
    setCurrentView(view);
    
    // Hide all views
    document.getElementById("dashboardView").classList.add("hidden");
    document.getElementById("transactionsView").classList.add("hidden");
    document.getElementById("fraudAlertsView").classList.add("hidden");
    
    // Remove active state from all nav links
    document.querySelectorAll(".nav-link").forEach(link => {
        link.classList.remove("bg-gray-800");
    });
    
    // Show selected view and update nav
    const transactions = getTransactions();
    
    if (view === "dashboard") {
        document.getElementById("dashboardView").classList.remove("hidden");
        document.getElementById("navDashboard").classList.add("bg-gray-800");
        document.getElementById("pageTitle").textContent = "Dashboard";
        if (transactions.length > 0) updateCharts();
    } else if (view === "transactions") {
        document.getElementById("transactionsView").classList.remove("hidden");
        document.getElementById("navTransactions").classList.add("bg-gray-800");
        document.getElementById("pageTitle").textContent = "All Transactions";
        renderTransactionsTable();
    } else if (view === "fraudAlerts") {
        document.getElementById("fraudAlertsView").classList.remove("hidden");
        document.getElementById("navFraudAlerts").classList.add("bg-gray-800");
        document.getElementById("pageTitle").textContent = "Fraud Alerts";
        renderFraudTable(getCurrentFraudFilter());
    }
}

// ============================================================
//  DATA GENERATION
// ============================================================

async function generateTransactions() {
    const btn = document.getElementById("generateBtn");
    btn.disabled = true;
    btn.textContent = "Generating...";
    
    const data = await generateTransactionsAPI();
    
    if (data) {
        setTransactions(data.transactions);
        updateStats();
        
        // Refresh current view
        const currentView = getCurrentView();
        if (currentView === "dashboard") updateCharts();
        if (currentView === "transactions") renderTransactionsTable();
        if (currentView === "fraudAlerts") {
            setCurrentFraudFilter(null); // Reset filter on new generation
            clearFraudFilter();
        }
    }
    
    btn.disabled = false;
    btn.textContent = "Generate Transactions";
}

// ============================================================
//  STATISTICS UPDATE
// ============================================================

function updateStats() {
    const { total, fraudCount, fraudRate } = getStats();
    
    document.getElementById("totalTx").textContent = total;
    document.getElementById("fraudTx").textContent = fraudCount;
    document.getElementById("fraudRate").textContent = fraudRate + "%";
}

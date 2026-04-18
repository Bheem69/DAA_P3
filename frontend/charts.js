// ============================================================
//  CHART MANAGEMENT
//  Handles Chart.js visualizations
// ============================================================

import { 
    getTransactions, 
    getStatusPieChart, 
    setStatusPieChart,
    getFraudTypesChart,
    setFraudTypesChart
} from "./state.js";

// Update all charts
export function updateCharts() {
    const transactions = getTransactions();
    if (transactions.length === 0) return;
    
    updateStatusPieChart();
    updateFraudTypesChart();
}

// Update status pie chart (Safe vs Fraud)
function updateStatusPieChart() {
    const transactions = getTransactions();
    const fraudCount = transactions.filter(tx => tx.fraud).length;
    const safeCount = transactions.length - fraudCount;
    
    // Destroy existing chart
    const existingChart = getStatusPieChart();
    if (existingChart) existingChart.destroy();
    
    // Create new chart
    const chart = new Chart(document.getElementById('statusPieChart'), {
        type: 'doughnut',
        data: {
            labels: ['Safe', 'Fraud'],
            datasets: [{
                data: [safeCount, fraudCount],
                backgroundColor: ['rgba(16, 185, 129, 0.8)', 'rgba(239, 68, 68, 0.8)'],
                borderColor: ['rgba(16, 185, 129, 1)', 'rgba(239, 68, 68, 1)'],
                borderWidth: 2
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: true,
            plugins: {
                legend: {
                    position: 'bottom',
                    labels: { color: '#9CA3AF', padding: 15, font: { size: 12 } }
                }
            }
        }
    });
    
    setStatusPieChart(chart);
}

// Update fraud types bar chart
function updateFraudTypesChart() {
    const transactions = getTransactions();
    
    // Count fraud reasons (handle multiple reasons with '+' separator)
    const fraudReasons = {
        'Large_transaction': 0,
        'Rapid_transactions': 0,
        'Daily_limit_exceeded': 0,
        'Fraud_ring': 0,
        'Fraud_hub': 0,
        'Fraud_chain': 0
    };
    
    transactions.forEach(tx => {
        if (tx.fraud && tx.reason !== "NA") {
            // Handle multiple fraud reasons separated by '+'
            const reasons = tx.reason.split('+');
            reasons.forEach(reason => {
                if (fraudReasons.hasOwnProperty(reason)) {
                    fraudReasons[reason]++;
                }
            });
        }
    });
    
    // Destroy existing chart
    const existingChart = getFraudTypesChart();
    if (existingChart) existingChart.destroy();
    
    // Create new chart
    const chart = new Chart(document.getElementById('fraudTypesChart'), {
        type: 'bar',
        data: {
            labels: ['Large', 'Rapid', 'Daily Limit', 'Ring', 'Hub', 'Chain'],
            datasets: [{
                label: 'Count',
                data: [
                    fraudReasons['Large_transaction'],
                    fraudReasons['Rapid_transactions'],
                    fraudReasons['Daily_limit_exceeded'],
                    fraudReasons['Fraud_ring'],
                    fraudReasons['Fraud_hub'],
                    fraudReasons['Fraud_chain']
                ],
                backgroundColor: [
                    'rgba(239, 68, 68, 0.8)',
                    'rgba(251, 146, 60, 0.8)',
                    'rgba(234, 179, 8, 0.8)',
                    'rgba(168, 85, 247, 0.8)',
                    'rgba(59, 130, 246, 0.8)',
                    'rgba(236, 72, 153, 0.8)'
                ],
                borderColor: [
                    'rgba(239, 68, 68, 1)',
                    'rgba(251, 146, 60, 1)',
                    'rgba(234, 179, 8, 1)',
                    'rgba(168, 85, 247, 1)',
                    'rgba(59, 130, 246, 1)',
                    'rgba(236, 72, 153, 1)'
                ],
                borderWidth: 2
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: true,
            scales: {
                y: {
                    beginAtZero: true,
                    ticks: { color: '#9CA3AF', stepSize: 1 },
                    grid: { color: 'rgba(75, 85, 99, 0.3)' }
                },
                x: {
                    ticks: { color: '#9CA3AF', font: { size: 10 } },
                    grid: { display: false }
                }
            },
            plugins: {
                legend: { display: false }
            }
        }
    });
    
    setFraudTypesChart(chart);
}

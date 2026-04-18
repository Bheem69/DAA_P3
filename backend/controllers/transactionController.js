const { runFraudEngine, readTransactionsFile } = require("../models/transactionModel");

exports.getTransactions = async (req, res) => {
    try {
        const transactions = await readTransactionsFile();
        res.json(transactions);
    } catch (error) {
        console.error("Error fetching transactions:", error);
        res.status(500).json({ message: "Failed to fetch transactions", transactions: [] });
    }
};

exports.generateTransactions = async (req, res) => {
    try {
        await runFraudEngine();
        
        // Wait a bit for file to be written
        setTimeout(async () => {
            try {
                const transactions = await readTransactionsFile();
                res.json({ 
                    message: "Transactions generated successfully", 
                    total: transactions.length, 
                    transactions 
                });
            } catch (error) {
                res.status(500).json({ message: "Failed to read transactions" });
            }
        }, 500);
    } catch (error) {
        console.error("Error generating transactions:", error);
        res.status(500).json({ message: "Failed to generate transactions" });
    }
};

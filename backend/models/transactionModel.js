const { exec } = require("child_process");
const { promisify } = require("util");
const fs = require("fs").promises;
const path = require("path");
const rootDir = require("../utils/pathUtil");

const execAsync = promisify(exec);
const backendDir = path.join(rootDir, "backend");
const cppSourceFile = path.join(backendDir, "fraudDetection.cpp");
const exePath = path.join(backendDir, "fraudDetection");
const filePath = path.join(backendDir, "transactions.txt");

async function compileCppIfNeeded() {
    try {
        // Check if executable exists and is newer than source
        try {
            const exeStat = await fs.stat(exePath);
            const srcStat = await fs.stat(cppSourceFile);
            if (exeStat.mtimeMs > srcStat.mtimeMs) {
                console.log("Executable is up to date");
                return;
            }
        } catch (err) {
            // Executable doesn't exist, need to compile
        }

        console.log("Compiling C++ fraud detection engine...");
        await execAsync(`g++ -std=c++17 -O2 "${cppSourceFile}" -o "${exePath}"`);
        console.log("Compilation successful");
    } catch (error) {
        console.error("Compilation error:", error);
        throw new Error("Failed to compile fraud detection engine");
    }
}

async function runFraudEngine() {
    try {
        // Compile if needed
        await compileCppIfNeeded();
        
        // Run the executable
        await execAsync(`"${exePath}"`);
        console.log("Fraud detection engine executed successfully");
    } catch (error) {
        console.error("Fraud engine error:", error);
        throw error;
    }
}

async function readTransactionsFile() {
    try {
        const data = await fs.readFile(filePath, "utf8");
        
        if (!data.trim()) {
            return [];
        }

        const lines = data.trim().split("\n");
        const startIndex = lines[0]?.trim().startsWith("ID") ? 2 : 0;

        const transactions = lines
            .slice(startIndex)
            .map(line => {
                const parts = line.trim().split(/\s+/);
                if (parts.length < 6) return null;
                
                const isFraud = parts[5] === "1" || parts[5] === "YES";
                
                return {
                    id: Number(parts[0]),
                    user: Number(parts[1]),
                    receiver: Number(parts[2]),
                    amount: Number(parts[3]),
                    timestamp: parts[4],
                    fraud: isFraud,
                    reason: parts[6] || "NA"
                };
            })
            .filter(tx => tx?.id);

        return transactions;
    } catch (error) {
        console.error("Error reading transactions file:", error);
        return [];
    }
}

module.exports = { runFraudEngine, readTransactionsFile };

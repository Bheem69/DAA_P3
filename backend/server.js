const express = require("express");
const cors = require("cors");
const path = require("path");
const transactionRoutes = require("./routes/transactionRoutes");
const rootDir = require("./utils/pathUtil");

const app = express();
const PORT = 3000;

// Middleware
app.use(cors());
app.use(express.json());
app.use((req, res, next) => {
    console.log(`${req.method} ${req.url}`);
    next();
});

// API Routes
app.use("/api/transactions", transactionRoutes);

// Serve Frontend
app.get("/", (req, res) => {
    res.sendFile(path.join(rootDir, "frontend", "index.html"));
});
app.use(express.static(path.join(rootDir, "frontend")));

// Error Handler
app.use((err, req, res, next) => {
    console.error("Server Error:", err.stack);
    res.status(500).json({ message: "Something went wrong" });
});

// Start Server
app.listen(PORT, () => {
    console.log(`Server running at http://localhost:${PORT}`);
});

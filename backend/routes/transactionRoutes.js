const express = require("express");
const { generateTransactions, getTransactions } = require("../controllers/transactionController");

const router = express.Router();

router.get("/", getTransactions);
router.post("/", generateTransactions);

module.exports = router;

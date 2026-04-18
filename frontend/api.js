const BASE_URL = "/api/transactions";

export async function generateTransactionsAPI() {
    try {
        const response = await fetch(BASE_URL, { method: "POST" });
        
        if (!response.ok) {
            throw new Error("Failed to generate transactions");
        }
        
        return await response.json();
    } catch (error) {
        console.error("API Error:", error);
        alert("Failed to generate transactions. Please try again.");
        return null;
    }
}

export async function loadTransactionsAPI() {
    try {
        const response = await fetch(BASE_URL);
        
        if (!response.ok) {
            throw new Error("Failed to load transactions");
        }
        
        return await response.json();
    } catch (error) {
        console.error("API Error:", error);
        return [];
    }
}

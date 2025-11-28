import express from "express";
import { WebSocketServer } from "ws";

const PORT = process.env.PORT || 3000;

// Basic express server (keeps Render happy)
const app = express();
app.get("/", (req, res) => res.send("WiFi Car Relay WebSocket Server Running!"));
app.listen(PORT, () => console.log(`HTTP Server running on port: ${PORT}`)); 

// WebSocket Relay - Runs on PORT + 1
const wss = new WebSocketServer({ port: PORT + 1 });

let espClient = null; // Stores the single ESP8266 client connection

wss.on("connection", ws => {
    console.log("Client connected");

    ws.on("message", msg => {
        const data = msg.toString();

        if (data === "ESP_CONNECT") {
            // ESP is identifying itself
            espClient = ws;
            console.log("ESP Connected");
            return;
        }

        // --- THE RELAY ---
        // UI sends command (new JSON format) -> Relay to ESP
        if (espClient && espClient.readyState === 1) {
            // Forward the received message (data) directly to the ESP client
            espClient.send(data); 
        }
    });

    ws.on("close", () => {
        if (ws === espClient) espClient = null;
        console.log("Client disconnected");
    });
});

console.log("WebSocket Relay initialized on port", PORT + 1);
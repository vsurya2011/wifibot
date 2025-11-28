import express from "express";
import { WebSocketServer } from "ws";

// Get the PORT environment variable as an integer.
// Use 3000 as a default for local testing if the environment variable isn't set.
const PORT_STR = process.env.PORT;
const PORT = parseInt(PORT_STR) || 3000; 

// Basic express server to handle HTTP requests (and keep Render happy)
const app = express();
app.get("/", (req, res) => res.send("WiFi Car Relay WebSocket Server Running!"));
// The express app must listen on the primary PORT
app.listen(PORT, () => console.log(`HTTP Server running on port: ${PORT}`)); 

// WebSocket Relay - Runs on PORT + 1. 
// Now that PORT is an integer, this will calculate correctly (e.g., 10000 + 1 = 10001).
const WS_PORT = PORT + 1;
const wss = new WebSocketServer({ port: WS_PORT });

let espClient = null; 

wss.on("connection", ws => {
    console.log("Client connected");

    ws.on("message", msg => {
        const data = msg.toString();

        if (data === "ESP_CONNECT") {
            espClient = ws;
            console.log("ESP Connected");
            return;
        }

        // Forward the control command to the ESP client
        if (espClient && espClient.readyState === 1) {
            espClient.send(data);
        }
    });

    ws.on("close", () => {
        if (ws === espClient) espClient = null;
        console.log("Client disconnected");
    });
});

console.log("WebSocket Relay initialized on port", WS_PORT);
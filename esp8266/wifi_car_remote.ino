#include <ESP8266WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h> 

WebSocketsClient webSocket;

// ➡️ CHANGE THESE: Your home Wi-Fi credentials
const char* ssid = "12345678";
const char* password = "12345678";

// 🔄 IMPORTANT: Replace with your deployed Render App URL
const char* WS_HOST = "your-app-name.onrender.com"; 
const int WS_PORT = 10001; 
const char* WS_PATH = "/";

// ➡️ L298N Motor Pins (NodeMCU D Pins)
const int ENA_Left = D1;  // GPIO 5 (PWM for Left Motor Speed)
const int IN1_Left = D2;  // GPIO 4 (Left Motor Direction)
const int IN2_Left = D3;  // GPIO 0 (Left Motor Direction)
const int ENB_Right = D6; // GPIO 12 (PWM for Right Motor Speed)
const int IN3_Right = D5; // GPIO 14 (Right Motor Direction)
const int IN4_Right = D4; // GPIO 2 (Right Motor Direction)

// --- Function Prototypes ---
void setupMotors();
void driveCar(int angle, int throttleVal, bool fwd);
void handleSimpleCommand(const char* dir);
void stopCar();
void handleCommand(char *data);
void onWebSocketEvent(WStype_t type, uint8_t* payload, size_t length);

// --- Motor Driver Functions ---
void setupMotors() {
  pinMode(ENA_Left, OUTPUT); pinMode(IN1_Left, OUTPUT); pinMode(IN2_Left, OUTPUT);
  pinMode(ENB_Right, OUTPUT); pinMode(IN3_Right, OUTPUT); pinMode(IN4_Right, OUTPUT);
  stopCar(); 
}

void driveCar(int angle, int throttleVal, bool fwd) {
  if (throttleVal == 0) {
    stopCar();
    return;
  }

  const int minSpeed = 50; 
  int baseSpeed = map(throttleVal, 0, 255, minSpeed, 255); 
  if (throttleVal < minSpeed) baseSpeed = 0;

  int leftSpeed = baseSpeed;
  int rightSpeed = baseSpeed;

  // Steering Logic
  int steerInfluence = map(abs(angle), 0, 90, 0, baseSpeed);

  if (angle > 5) { // Turning Right (Slow down right side)
    rightSpeed = baseSpeed - steerInfluence;
  } else if (angle < -5) { // Turning Left (Slow down left side)
    leftSpeed = baseSpeed - steerInfluence;
  }
  
  leftSpeed = constrain(leftSpeed, 0, 255);
  rightSpeed = constrain(rightSpeed, 0, 255);

  // Apply Directions
  if (fwd) {
    digitalWrite(IN1_Left, HIGH); digitalWrite(IN2_Left, LOW);
    digitalWrite(IN3_Right, HIGH); digitalWrite(IN4_Right, LOW);
  } else {
    digitalWrite(IN1_Left, LOW); digitalWrite(IN2_Left, HIGH);
    digitalWrite(IN3_Right, LOW); digitalWrite(IN4_Right, HIGH);
  }

  // Apply Speed (PWM)
  analogWrite(ENA_Left, leftSpeed);
  analogWrite(ENB_Right, rightSpeed);
}

void handleSimpleCommand(const char* dir) {
  const int fixedSpeed = 200; // Use a fixed speed for simple button mode

  if (strcmp(dir, "F") == 0) {
    // Forward
    digitalWrite(IN1_Left, HIGH); digitalWrite(IN2_Left, LOW);
    digitalWrite(IN3_Right, HIGH); digitalWrite(IN4_Right, LOW);
    analogWrite(ENA_Left, fixedSpeed); analogWrite(ENB_Right, fixedSpeed);
  } else if (strcmp(dir, "B") == 0) {
    // Backward
    digitalWrite(IN1_Left, LOW); digitalWrite(IN2_Left, HIGH);
    digitalWrite(IN3_Right, LOW); digitalWrite(IN4_Right, HIGH);
    analogWrite(ENA_Left, fixedSpeed); analogWrite(ENB_Right, fixedSpeed);
  } else if (strcmp(dir, "L") == 0) {
    // Turn Left (e.g., spin left)
    digitalWrite(IN1_Left, LOW); digitalWrite(IN2_Left, HIGH);
    digitalWrite(IN3_Right, HIGH); digitalWrite(IN4_Right, LOW);
    analogWrite(ENA_Left, fixedSpeed); analogWrite(ENB_Right, fixedSpeed);
  } else if (strcmp(dir, "R") == 0) {
    // Turn Right (e.g., spin right)
    digitalWrite(IN1_Left, HIGH); digitalWrite(IN2_Left, LOW);
    digitalWrite(IN3_Right, LOW); digitalWrite(IN4_Right, HIGH);
    analogWrite(ENA_Left, fixedSpeed); analogWrite(ENB_Right, fixedSpeed);
  } else { // "S" - Stop
    stopCar();
  }
}

void stopCar() {
  digitalWrite(IN1_Left, LOW); digitalWrite(IN2_Left, LOW);
  digitalWrite(IN3_Right, LOW); digitalWrite(IN4_Right, LOW);
  analogWrite(ENA_Left, 0);
  analogWrite(ENB_Right, 0);
}

// --- Command Handling ---
void handleCommand(char *data) {
  // Commands can be:
  // Mode 2: {"mode":2,"angle":-90,"throttle":200,"isForward":true,"isBraking":false}
  // Mode 1: {"mode":1,"dir":"F"}
  
  StaticJsonDocument<128> doc;
  DeserializationError error = deserializeJson(doc, data);

  if (error) {
    Serial.print(F("JSON parsing failed: "));
    Serial.println(error.f_str());
    return;
  }

  int mode = doc["mode"];

  if (mode == 2) {
    // MODE 2: Steering Wheel (Continuous)
    int angle = doc["angle"];
    int throttle = doc["throttle"];
    bool isForward = doc["isForward"];
    bool isBraking = doc["isBraking"];

    if (isBraking) {
      stopCar();
    } else {
      driveCar(angle, throttle, isForward);
    }

  } else if (mode == 1) {
    // MODE 1: Simple Buttons (Discrete)
    const char* dir = doc["dir"];
    handleSimpleCommand(dir);
  }
}

// --- WebSocket Client Functions ---
void onWebSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
  switch(type) {
    case WStype_CONNECTED:
      Serial.println("WS: Connected to server");
      webSocket.sendTXT("ESP_CONNECT"); 
      break;

    case WStype_TEXT:
      // Received the control command
      handleCommand((char*)payload);
      break;
      
    case WStype_DISCONNECTED:
      Serial.println("WS: Disconnected from server. Stopping car.");
      stopCar();
      break;

    case WStype_ERROR:
      Serial.print("WS: Error. Stopping car.");
      stopCar();
      break;
  }
}

void setup() {
  Serial.begin(115200);
  setupMotors();
  
  // 1. Connect to the home router
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nWiFi Connected!");

  // 2. Connect to the Render Relay
  webSocket.begin(WS_HOST, WS_PORT, WS_PATH); 
  webSocket.onEvent(onWebSocketEvent);
  webSocket.setReconnectInterval(5000); 
}

void loop() {
  webSocket.loop();
}
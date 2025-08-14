/**
 * PicoWiFiManager - Dual Core Example
 * 
 * Demonstrates using Pico's dual-core architecture with WiFi management
 * 
 * Core 0: WiFi management and network operations
 * Core 1: Application logic and sensor processing
 * 
 * This example shows how to efficiently utilize both cores while
 * maintaining stable WiFi connectivity.
 */

#include "PicoWiFiManager.h"
#include <pico/multicore.h>

// WiFi Manager
PicoWiFiManager wifiManager;

// Inter-core communication
struct AppData {
    bool wifiConnected;
    String wifiSSID;
    int32_t wifiRSSI;
    IPAddress wifiIP;
    uint32_t sensorValue;
    float temperature;
    uint32_t loopCounter;
    bool requestReset;
};

// Shared data structure (protected by mutex)
AppData sharedData;
mutex_t dataMutex;

// Core 1 task handle
bool core1Running = false;

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n=== PicoWiFiManager Dual Core Example ===");
    Serial.println("Core 0: WiFi Management");
    Serial.println("Core 1: Application Logic");
    
    // Initialize mutex for shared data
    mutex_init(&dataMutex);
    
    // Initialize shared data
    updateSharedData();
    
    // Configure WiFi manager
    setupWiFiManager();
    
    // Start Core 1 application task
    startCore1Task();
    
    Serial.println("✓ Dual core setup complete");
    Serial.println("Commands: 'status', 'reset', 'config', 'core1-stop', 'core1-start'");
}

void setupWiFiManager() {
    // Custom configuration for dual-core operation
    PicoWiFiConfig config;
    strcpy(config.deviceName, "PicoDualCore");
    strcpy(config.apPassword, "dualcore123");
    config.autoReconnect = true;
    config.maxReconnectAttempts = 5;
    
    wifiManager.setConfig(config);
    
    if (!wifiManager.begin()) {
        Serial.println("Failed to initialize WiFi manager!");
        while (true) delay(1000);
    }
    
    // Set up callbacks
    wifiManager.onConnect([]() {
        Serial.println("🎉 Core 0: WiFi connected!");
        updateSharedData();
    });
    
    wifiManager.onDisconnect([]() {
        Serial.println("⚠ Core 0: WiFi disconnected!");
        updateSharedData();
    });
    
    wifiManager.onStatusChange([](ConnectionStatus status) {
        Serial.printf("📡 Core 0: WiFi status - %s\n", wifiManager.getStatusString().c_str());
        updateSharedData();
    });
    
    // Start WiFi connection
    if (!wifiManager.autoConnect()) {
        Serial.println("Core 0: Initial WiFi connection failed");
    }
}

void startCore1Task() {
    Serial.println("🚀 Starting Core 1 application task...");
    
    // Launch core1_task on the second core
    multicore_launch_core1(core1_task);
    core1Running = true;
    
    Serial.println("✓ Core 1 started successfully");
}

void stopCore1Task() {
    if (!core1Running) return;
    
    Serial.println("🛑 Stopping Core 1 task...");
    core1Running = false;
    
    // Wait a moment for core1 to stop gracefully
    delay(1000);
    
    // Force reset core1 if needed
    multicore_reset_core1();
    
    Serial.println("✓ Core 1 stopped");
}

void updateSharedData() {
    // Update shared data with current WiFi status
    // This should be called from Core 0 only
    
    mutex_enter_blocking(&dataMutex);
    
    sharedData.wifiConnected = wifiManager.isConnected();
    sharedData.wifiSSID = wifiManager.getSSID();
    sharedData.wifiRSSI = wifiManager.getRSSI();
    sharedData.wifiIP = wifiManager.getLocalIP();
    
    mutex_exit(&dataMutex);
}

// Core 1 task function
void core1_task() {
    Serial.println("✨ Core 1: Application task started");
    
    uint32_t loopCount = 0;
    uint32_t lastSensorRead = 0;
    uint32_t lastStatusPrint = 0;
    
    while (core1Running) {
        uint32_t now = millis();
        
        // Simulate sensor readings every 1 second
        if (now - lastSensorRead >= 1000) {
            // Simulate reading sensors (ADC, I2C devices, etc.)
            uint32_t sensorValue = analogRead(A0);  // Example: read ADC
            float temperature = 20.0 + (sensorValue * 0.01); // Simulate temperature
            
            // Update shared data
            mutex_enter_blocking(&dataMutex);
            sharedData.sensorValue = sensorValue;
            sharedData.temperature = temperature;
            sharedData.loopCounter = loopCount;
            mutex_exit(&dataMutex);
            
            lastSensorRead = now;
        }
        
        // Print status every 10 seconds
        if (now - lastStatusPrint >= 10000) {
            printCore1Status();
            lastStatusPrint = now;
        }
        
        // Check for reset request from Core 0
        mutex_enter_blocking(&dataMutex);
        bool resetRequested = sharedData.requestReset;
        if (resetRequested) {
            sharedData.requestReset = false;
        }
        mutex_exit(&dataMutex);
        
        if (resetRequested) {
            Serial.println("🔄 Core 1: Reset request received");
            // Perform any cleanup here
            break;
        }
        
        // Simulate application work
        performApplicationWork();
        
        loopCount++;
        
        // Yield to other tasks
        sleep_ms(100);
    }
    
    Serial.println("👋 Core 1: Task ended");
}

void performApplicationWork() {
    // Simulate CPU-intensive application work
    // This runs on Core 1, leaving Core 0 free for WiFi management
    
    static uint32_t workCounter = 0;
    workCounter++;
    
    // Example: Process data, handle sensors, run algorithms, etc.
    // This is where your main application logic would go
    
    // Simulate some computational work
    if (workCounter % 100 == 0) {
        // Do something computationally intensive occasionally
        for (volatile int i = 0; i < 1000; i++) {
            // Busy work
        }
    }
}

void printCore1Status() {
    // Print Core 1 status using shared data
    mutex_enter_blocking(&dataMutex);
    
    Serial.printf("🔄 Core 1 Status:\n");
    Serial.printf("  Loop Count: %lu\n", sharedData.loopCounter);
    Serial.printf("  Sensor Value: %lu\n", sharedData.sensorValue);
    Serial.printf("  Temperature: %.2f°C\n", sharedData.temperature);
    Serial.printf("  WiFi Status: %s\n", sharedData.wifiConnected ? "Connected" : "Disconnected");
    
    if (sharedData.wifiConnected) {
        Serial.printf("  SSID: %s\n", sharedData.wifiSSID.c_str());
        Serial.printf("  RSSI: %ld dBm\n", sharedData.wifiRSSI);
        Serial.printf("  IP: %s\n", sharedData.wifiIP.toString().c_str());
    }
    
    mutex_exit(&dataMutex);
}

void handleSerialCommands() {
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        command.toLowerCase();
        
        if (command == "status") {
            printSystemStatus();
        }
        else if (command == "reset") {
            Serial.println("🔄 Requesting system reset...");
            wifiManager.reset();
        }
        else if (command == "config") {
            Serial.println("⚙ Starting config portal...");
            wifiManager.startConfigPortal();
        }
        else if (command == "core1-stop") {
            stopCore1Task();
        }
        else if (command == "core1-start") {
            if (!core1Running) {
                startCore1Task();
            } else {
                Serial.println("Core 1 is already running");
            }
        }
        else if (command == "core1-status") {
            printCore1Status();
        }
        else if (command == "help") {
            printHelp();
        }
        else {
            Serial.printf("Unknown command: %s\n", command.c_str());
            Serial.println("Type 'help' for available commands");
        }
    }
}

void printSystemStatus() {
    Serial.println("\n📊 System Status (Dual Core):");
    Serial.println("================================");
    
    // Core 0 (WiFi) status
    Serial.println("Core 0 (WiFi Management):");
    Serial.printf("  Status: %s\n", wifiManager.getStatusString().c_str());
    Serial.printf("  Free Heap: %zu bytes\n", wifiManager.getFreeHeap());
    Serial.printf("  Uptime: %lu seconds\n", wifiManager.getUptime() / 1000);
    
    if (wifiManager.isConnected()) {
        Serial.printf("  Connected to: %s\n", wifiManager.getSSID().c_str());
        Serial.printf("  IP Address: %s\n", wifiManager.getLocalIP().toString().c_str());
        Serial.printf("  Signal: %d dBm\n", wifiManager.getRSSI());
    }
    
    Serial.println();
    
    // Core 1 (Application) status
    Serial.printf("Core 1 (Application): %s\n", core1Running ? "Running" : "Stopped");
    if (core1Running) {
        printCore1Status();
    }
    
    Serial.println("================================");
}

void printHelp() {
    Serial.println("\n📋 Available Commands:");
    Serial.println("  status      - Show detailed system status");
    Serial.println("  reset       - Perform factory reset");
    Serial.println("  config      - Start WiFi configuration portal");
    Serial.println("  core1-stop  - Stop Core 1 application task");
    Serial.println("  core1-start - Start Core 1 application task");
    Serial.println("  core1-status- Show Core 1 status only");
    Serial.println("  help        - Show this help message");
    Serial.println();
}

void loop() {
    // Core 0 main loop - handle WiFi management
    wifiManager.loop();
    
    // Handle serial commands
    handleSerialCommands();
    
    // Update shared data periodically
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate > 5000) { // Every 5 seconds
        updateSharedData();
        lastUpdate = millis();
    }
    
    // Brief status update
    static unsigned long lastBriefStatus = 0;
    if (millis() - lastBriefStatus > 30000) { // Every 30 seconds
        Serial.printf("📡 Core 0: %s | Core 1: %s\n", 
                     wifiManager.getStatusString().c_str(),
                     core1Running ? "Active" : "Stopped");
        lastBriefStatus = millis();
    }
    
    // Core 0 can handle other network-related tasks here
    // Example: HTTP server, MQTT client, OTA updates, etc.
    
    delay(50); // Short delay to prevent overwhelming the system
}
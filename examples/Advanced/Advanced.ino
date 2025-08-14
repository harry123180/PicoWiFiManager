/**
 * PicoWiFiManager - Advanced Example
 * 
 * Demonstrates advanced features of PicoWiFiManager including:
 * - Custom configuration
 * - Static IP setup
 * - Network scanning
 * - Storage management
 * - Diagnostics
 */

#include "PicoWiFiManager.h"

// Create custom configuration
PicoWiFiConfig customConfig;
PicoWiFiManager wifiManager;

// Custom parameters
bool useStaticIP = false;
IPAddress staticIP(192, 168, 1, 100);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns1(8, 8, 8, 8);
IPAddress dns2(8, 8, 4, 4);

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n=== PicoWiFiManager Advanced Example ===");
    
    // Configure custom settings
    setupCustomConfig();
    
    // Initialize WiFi manager with custom config
    wifiManager.setConfig(customConfig);
    
    if (!wifiManager.begin()) {
        Serial.println("Failed to initialize WiFi manager!");
        while (true) delay(1000);
    }
    
    // Set up detailed callbacks
    setupCallbacks();
    
    // Enable debug output
    wifiManager.enableDebug(true);
    
    Serial.println("Configuration complete. Starting connection...");
    
    // Attempt connection
    if (wifiManager.autoConnect()) {
        Serial.println("✓ WiFi connected successfully!");
        printConnectionInfo();
    } else {
        Serial.println("✗ Failed to connect to WiFi");
    }
    
    // Print initial diagnostics
    wifiManager.printDiagnostics();
}

void setupCustomConfig() {
    // Device settings
    strcpy(customConfig.deviceName, "PicoAdvanced");
    strcpy(customConfig.apPassword, "advanced123");
    
    // Timeout settings
    customConfig.configPortalTimeout = 600;  // 10 minutes
    customConfig.connectTimeout = 20;        // 20 seconds
    customConfig.maxReconnectAttempts = 5;
    
    // Network settings
    if (useStaticIP) {
        customConfig.useStaticIP = true;
        customConfig.staticIP = staticIP;
        customConfig.gateway = gateway;
        customConfig.subnet = subnet;
        customConfig.primaryDNS = dns1;
        customConfig.secondaryDNS = dns2;
    }
    
    // Hardware settings
    customConfig.ledPin = LED_BUILTIN;
    customConfig.resetPin = 2;
    
    // Behavior settings
    customConfig.autoReconnect = true;
    customConfig.enableSerial = true;
    
    Serial.println("✓ Custom configuration applied");
}

void setupCallbacks() {
    wifiManager.onConnect([]() {
        Serial.println("\n🎉 WiFi Connection Established!");
        printConnectionInfo();
        
        // Start your application services here
        startApplicationServices();
    });
    
    wifiManager.onDisconnect([]() {
        Serial.println("\n⚠ WiFi Connection Lost!");
        
        // Stop application services if needed
        stopApplicationServices();
    });
    
    wifiManager.onConfigModeStart([]() {
        Serial.println("\n⚙ Configuration Portal Started");
        Serial.printf("SSID: %s\n", customConfig.deviceName);
        Serial.printf("Password: %s\n", customConfig.apPassword);
        Serial.println("Access Point IP: 192.168.4.1");
        Serial.println("Web Interface: http://192.168.4.1");
        
        // You might want to indicate config mode with different LED pattern
        // or display message on screen
    });
    
    wifiManager.onConfigModeEnd([]() {
        Serial.println("\n✓ Configuration Portal Stopped");
    });
    
    wifiManager.onStatusChange([](ConnectionStatus status) {
        Serial.printf("📡 Status Change: %s\n", wifiManager.getStatusString().c_str());
        
        // Handle status-specific actions
        switch (status) {
            case ConnectionStatus::CONNECTING:
                Serial.println("   Attempting connection...");
                break;
            case ConnectionStatus::CONNECTED:
                Serial.println("   Connection successful!");
                break;
            case ConnectionStatus::DISCONNECTED:
                Serial.println("   Disconnected from network");
                break;
            case ConnectionStatus::CONFIG_MODE:
                Serial.println("   Configuration mode active");
                break;
            case ConnectionStatus::ERROR:
                Serial.println("   Error state - check configuration");
                break;
        }
    });
}

void printConnectionInfo() {
    if (!wifiManager.isConnected()) {
        Serial.println("Not connected to WiFi");
        return;
    }
    
    Serial.println("\n📊 Connection Information:");
    Serial.printf("  SSID: %s\n", wifiManager.getSSID().c_str());
    Serial.printf("  IP Address: %s\n", wifiManager.getLocalIP().toString().c_str());
    Serial.printf("  Signal Strength: %d dBm\n", wifiManager.getRSSI());
    Serial.printf("  MAC Address: %s\n", wifiManager.getMACAddress().c_str());
    Serial.printf("  Uptime: %lu seconds\n", wifiManager.getUptime() / 1000);
    Serial.printf("  Free Heap: %zu bytes\n", wifiManager.getFreeHeap());
}

void startApplicationServices() {
    Serial.println("🚀 Starting application services...");
    
    // Example: Start web server, MQTT client, sensors, etc.
    // webServer.begin();
    // mqttClient.connect();
    // sensors.init();
    
    Serial.println("✓ Application services started");
}

void stopApplicationServices() {
    Serial.println("🛑 Stopping application services...");
    
    // Example: Stop services gracefully
    // webServer.stop();
    // mqttClient.disconnect();
    // sensors.stop();
    
    Serial.println("✓ Application services stopped");
}

void handleSerialCommands() {
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        command.toLowerCase();
        
        if (command == "status") {
            wifiManager.printDiagnostics();
        }
        else if (command == "reset") {
            Serial.println("Performing factory reset...");
            wifiManager.reset();
        }
        else if (command == "config") {
            if (wifiManager.isConfigMode()) {
                Serial.println("Config portal already active");
            } else {
                Serial.println("Starting config portal...");
                wifiManager.startConfigPortal();
            }
        }
        else if (command == "scan") {
            performNetworkScan();
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

void performNetworkScan() {
    Serial.println("\n📡 Scanning for WiFi networks...");
    
    int networks = WiFi.scanNetworks();
    
    if (networks == 0) {
        Serial.println("No networks found");
    } else {
        Serial.printf("Found %d networks:\n", networks);
        
        for (int i = 0; i < networks; i++) {
            Serial.printf("%2d: %-20s (%3d dBm) %s\n",
                i + 1,
                WiFi.SSID(i),
                WiFi.RSSI(i),
                WiFi.encryptionType(i) == ENC_TYPE_NONE ? "Open" : "Secured"
            );
        }
    }
    
    Serial.println();
}

void printHelp() {
    Serial.println("\n📋 Available Commands:");
    Serial.println("  status  - Show detailed status and diagnostics");
    Serial.println("  reset   - Perform factory reset (clears all settings)");
    Serial.println("  config  - Start configuration portal");
    Serial.println("  scan    - Scan for available WiFi networks");
    Serial.println("  help    - Show this help message");
    Serial.println();
}

void loop() {
    // Handle WiFi manager
    wifiManager.loop();
    
    // Handle serial commands
    handleSerialCommands();
    
    // Periodic status updates
    static unsigned long lastStatusUpdate = 0;
    if (millis() - lastStatusUpdate > 30000) { // Every 30 seconds
        if (wifiManager.isConnected()) {
            Serial.printf("📊 Status: Connected to %s (RSSI: %d dBm) | Uptime: %lu s | Free Heap: %zu bytes\n",
                         wifiManager.getSSID().c_str(),
                         wifiManager.getRSSI(),
                         wifiManager.getUptime() / 1000,
                         wifiManager.getFreeHeap());
        } else if (!wifiManager.isConfigMode()) {
            Serial.printf("📊 Status: %s | Uptime: %lu s\n",
                         wifiManager.getStatusString().c_str(),
                         wifiManager.getUptime() / 1000);
        }
        lastStatusUpdate = millis();
    }
    
    // Your main application logic here
    
    delay(100);
}
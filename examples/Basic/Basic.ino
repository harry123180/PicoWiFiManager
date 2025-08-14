/**
 * PicoWiFiManager - Basic Example
 * 
 * Demonstrates basic WiFi connection management for Raspberry Pi Pico 2 W
 * 
 * This example shows:
 * - Auto-connection to saved WiFi networks
 * - Fallback to configuration portal when connection fails
 * - Basic status monitoring
 */

#include "PicoWiFiManager.h"

// Create WiFi manager instance
PicoWiFiManager wifiManager;

void setup() {
    Serial.begin(115200);
    delay(2000); // Wait for serial monitor
    
    Serial.println("\n=== PicoWiFiManager Basic Example ===");
    
    // Initialize WiFi manager
    if (!wifiManager.begin()) {
        Serial.println("Failed to initialize WiFi manager!");
        while (true) {
            delay(1000);
        }
    }
    
    // Set device name (will appear as AP name if config portal starts)
    wifiManager.setDeviceName("MyPico2W");
    
    // Set timeout for configuration portal
    wifiManager.setTimeout(300); // 5 minutes
    
    // Set up callbacks to monitor connection status
    wifiManager.onConnect([]() {
        Serial.println("✓ WiFi connected successfully!");
        Serial.print("IP address: ");
        Serial.println(wifiManager.getLocalIP());
        Serial.print("Signal strength: ");
        Serial.print(wifiManager.getRSSI());
        Serial.println(" dBm");
    });
    
    wifiManager.onConfigModeStart([]() {
        Serial.println("⚙ Starting configuration portal...");
        Serial.println("Connect to WiFi network: MyPico2W");
        Serial.println("Password: picowifi123");
        Serial.println("Then open browser to: 192.168.4.1");
    });
    
    wifiManager.onStatusChange([](ConnectionStatus status) {
        Serial.print("Status changed to: ");
        Serial.println(wifiManager.getStatusString());
    });
    
    // Attempt to connect
    Serial.println("Attempting WiFi connection...");
    if (!wifiManager.autoConnect()) {
        Serial.println("Failed to connect to WiFi");
    }
}

void loop() {
    // This must be called regularly to handle WiFi events
    wifiManager.loop();
    
    // Your application code here
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 10000) { // Print status every 10 seconds
        if (wifiManager.isConnected()) {
            Serial.printf("Connected to: %s (RSSI: %d dBm)\n", 
                         wifiManager.getSSID().c_str(), 
                         wifiManager.getRSSI());
        } else if (!wifiManager.isConfigMode()) {
            Serial.println("WiFi disconnected");
        }
        lastPrint = millis();
    }
    
    delay(100);
}
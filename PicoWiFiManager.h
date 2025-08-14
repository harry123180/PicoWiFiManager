/**
 * PicoWiFiManager - WiFi Connection Manager for Raspberry Pi Pico 2 W
 * 
 * A clean, Pico-optimized WiFi configuration manager with web portal
 * No ESP32/ESP8266 compatibility layer - designed specifically for Pico
 * 
 * Author: PicoWiFiManager Project
 * License: MIT
 */

#ifndef PICO_WIFI_MANAGER_H
#define PICO_WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "ConfigPortal.h"
#include "StorageManager.h"
#include "NetworkScanner.h"

// Status LED behavior
enum class LEDMode {
    OFF,
    ON,
    FAST_BLINK,    // Config mode
    SLOW_BLINK,    // Connecting
    PULSE          // Error
};

// Connection status
enum class ConnectionStatus {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    CONFIG_MODE,
    ERROR
};

// Configuration structure
struct PicoWiFiConfig {
    char deviceName[32] = "Pico2W";
    char apPassword[64] = "picowifi123";
    uint16_t configPortalTimeout = 300; // 5 minutes
    uint16_t connectTimeout = 30;       // 30 seconds
    uint8_t maxReconnectAttempts = 3;
    bool autoReconnect = true;
    bool enableSerial = true;
    uint8_t ledPin = LED_BUILTIN;
    uint8_t resetPin = 2;
    
    // Advanced settings
    bool useStaticIP = false;
    IPAddress staticIP;
    IPAddress gateway;
    IPAddress subnet;
    IPAddress primaryDNS;
    IPAddress secondaryDNS;
};

// Callback function types
typedef std::function<void(void)> PicoWiFiCallback;
typedef std::function<void(ConnectionStatus)> StatusCallback;

class PicoWiFiManager {
public:
    PicoWiFiManager();
    explicit PicoWiFiManager(const PicoWiFiConfig& config);
    ~PicoWiFiManager();

    // Main public interface
    bool begin();
    bool autoConnect();
    bool autoConnect(const char* ssid, const char* password = nullptr);
    
    void loop();
    void reset();
    void disconnect();
    
    // Configuration
    void setConfig(const PicoWiFiConfig& config);
    PicoWiFiConfig getConfig() const;
    void setDeviceName(const char* name);
    void setTimeout(uint16_t seconds);
    void setResetPin(uint8_t pin);
    void setLEDPin(uint8_t pin);
    
    // Network management
    bool startConfigPortal();
    bool startConfigPortal(const char* ssid, const char* password = nullptr);
    void stopConfigPortal();
    
    // Status and information
    ConnectionStatus getStatus() const;
    String getStatusString() const;
    bool isConnected() const;
    bool isConfigMode() const;
    
    // Network information
    String getSSID() const;
    IPAddress getLocalIP() const;
    int32_t getRSSI() const;
    String getMACAddress() const;
    
    // Callbacks
    void onConfigModeStart(PicoWiFiCallback callback);
    void onConfigModeEnd(PicoWiFiCallback callback);
    void onConnect(PicoWiFiCallback callback);
    void onDisconnect(PicoWiFiCallback callback);
    void onStatusChange(StatusCallback callback);
    
    // Dual-core support
    void enableDualCore(bool enable = true);
    bool isDualCoreEnabled() const;
    
    // Debug and diagnostics
    void enableDebug(bool enable = true);
    void printDiagnostics() const;
    uint32_t getUptime() const;
    size_t getFreeHeap() const;

private:
    PicoWiFiConfig _config;
    ConnectionStatus _status;
    
    // Component managers
    ConfigPortal* _portal;
    StorageManager* _storage;
    NetworkScanner* _scanner;
    WebServer* _server;
    
    // Internal state
    bool _isInitialized;
    bool _configMode;
    bool _dualCoreEnabled;
    bool _debugEnabled;
    
    uint32_t _startTime;
    uint32_t _lastReconnectAttempt;
    uint8_t _reconnectAttempts;
    
    unsigned long _lastLEDUpdate;
    bool _ledState;
    
    // Callbacks
    PicoWiFiCallback _onConfigStart;
    PicoWiFiCallback _onConfigEnd;
    PicoWiFiCallback _onConnect;
    PicoWiFiCallback _onDisconnect;
    StatusCallback _onStatusChange;
    
    // Core methods
    bool connectWiFi(const char* ssid, const char* password);
    void handleReconnection();
    void updateLED();
    void setStatus(ConnectionStatus status);
    void checkResetButton();
    
    // Dual-core task
    static void core1Task();
    void runCore1();
    
    // Debug helpers
    void debugPrint(const String& message) const;
    void debugPrintf(const char* format, ...) const;
    
    // Static instance for dual-core
    static PicoWiFiManager* _instance;
};

#endif // PICO_WIFI_MANAGER_H
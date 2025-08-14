/**
 * PicoWiFiManager - Implementation
 * 
 * Core implementation of the Pico-optimized WiFi Manager
 */

#include "PicoWiFiManager.h"
#include <pico/stdlib.h>

// Static instance for dual-core support
PicoWiFiManager* PicoWiFiManager::_instance = nullptr;

PicoWiFiManager::PicoWiFiManager() : PicoWiFiManager(PicoWiFiConfig()) {
}

PicoWiFiManager::PicoWiFiManager(const PicoWiFiConfig& config) 
    : _config(config)
    , _status(ConnectionStatus::DISCONNECTED)
    , _portal(nullptr)
    , _storage(nullptr)
    , _scanner(nullptr)
    , _server(nullptr)
    , _isInitialized(false)
    , _configMode(false)
    , _dualCoreEnabled(false)
    , _debugEnabled(config.enableSerial)
    , _startTime(0)
    , _lastReconnectAttempt(0)
    , _reconnectAttempts(0)
    , _lastLEDUpdate(0)
    , _ledState(false) {
    
    _instance = this;
}

PicoWiFiManager::~PicoWiFiManager() {
    if (_portal) delete _portal;
    if (_storage) delete _storage;
    if (_scanner) delete _scanner;
    if (_server) delete _server;
    
    if (_instance == this) {
        _instance = nullptr;
    }
}

bool PicoWiFiManager::begin() {
    if (_isInitialized) return true;
    
    _startTime = millis();
    
    debugPrint("PicoWiFiManager starting...");
    
    // Initialize GPIO
    pinMode(_config.ledPin, OUTPUT);
    pinMode(_config.resetPin, INPUT_PULLUP);
    
    // Initialize storage
    _storage = new StorageManager();
    if (!_storage->begin()) {
        debugPrint("Failed to initialize storage");
        return false;
    }
    
    // Initialize network scanner
    _scanner = new NetworkScanner();
    
    // Initialize config portal
    _portal = new ConfigPortal(this);
    
    // Set up callbacks
    _portal->onConnect([this](const String& ssid, const String& password) {
        debugPrintf("Portal connect request: %s", ssid.c_str());
        if (connectWiFi(ssid.c_str(), password.c_str())) {
            _storage->saveWiFiCredentials(ssid.c_str(), password.c_str());
            stopConfigPortal();
            setStatus(ConnectionStatus::CONNECTED);
        }
    });
    
    _portal->onReset([this]() {
        debugPrint("Reset requested from portal");
        reset();
    });
    
    setStatus(ConnectionStatus::DISCONNECTED);
    _isInitialized = true;
    
    debugPrint("PicoWiFiManager initialized successfully");
    return true;
}

bool PicoWiFiManager::autoConnect() {
    if (!_isInitialized && !begin()) {
        return false;
    }
    
    WiFiCredentials credentials;
    if (_storage->loadWiFiCredentials(credentials) && credentials.valid) {
        debugPrintf("Attempting auto-connect to: %s", credentials.ssid);
        return autoConnect(credentials.ssid, credentials.password);
    } else {
        debugPrint("No saved credentials, starting config portal");
        return startConfigPortal();
    }
}

bool PicoWiFiManager::autoConnect(const char* ssid, const char* password) {
    if (!_isInitialized && !begin()) {
        return false;
    }
    
    debugPrintf("Auto-connecting to: %s", ssid);
    
    if (connectWiFi(ssid, password)) {
        debugPrint("Auto-connect successful");
        return true;
    } else {
        debugPrint("Auto-connect failed, starting config portal");
        return startConfigPortal();
    }
}

void PicoWiFiManager::loop() {
    if (!_isInitialized) return;
    
    // Handle web server
    if (_server) {
        _server->handleClient();
    }
    
    // Handle config portal
    if (_portal && _portal->isActive()) {
        _portal->handle();
    }
    
    // Check reset button
    checkResetButton();
    
    // Handle reconnection if needed
    if (!_configMode && _config.autoReconnect) {
        handleReconnection();
    }
    
    // Update LED
    updateLED();
    
    // Yield for other tasks
    yield();
}

bool PicoWiFiManager::startConfigPortal() {
    return startConfigPortal(_config.deviceName, _config.apPassword);
}

bool PicoWiFiManager::startConfigPortal(const char* ssid, const char* password) {
    debugPrintf("Starting config portal: %s", ssid);
    
    if (_onConfigStart) _onConfigStart();
    
    _configMode = true;
    setStatus(ConnectionStatus::CONFIG_MODE);
    
    if (_portal->start(ssid, password)) {
        debugPrintf("Config portal started at %s", _portal->getAPIP().toString().c_str());
        return true;
    } else {
        debugPrint("Failed to start config portal");
        _configMode = false;
        setStatus(ConnectionStatus::ERROR);
        return false;
    }
}

void PicoWiFiManager::stopConfigPortal() {
    if (!_configMode) return;
    
    debugPrint("Stopping config portal");
    _portal->stop();
    _configMode = false;
    
    if (_onConfigEnd) _onConfigEnd();
}

bool PicoWiFiManager::connectWiFi(const char* ssid, const char* password) {
    if (!ssid || strlen(ssid) == 0) {
        debugPrint("Invalid SSID provided");
        return false;
    }
    
    debugPrintf("Connecting to: %s", ssid);
    setStatus(ConnectionStatus::CONNECTING);
    
    // Stop any existing connection
    WiFi.disconnect();
    delay(100);
    
    // Set WiFi mode
    WiFi.mode(WIFI_STA);
    delay(100);
    
    // Apply static IP if configured
    if (_config.useStaticIP) {
        // Arduino-Pico WiFi.config parameter order: local_ip, dns_server, gateway, subnet
        WiFi.config(_config.staticIP, _config.primaryDNS, _config.gateway, _config.subnet);
        debugPrint("Static IP configuration applied");
    }
    
    // Begin connection
    if (password && strlen(password) > 0) {
        WiFi.begin(ssid, password);
    } else {
        WiFi.begin(ssid);
    }
    
    // Wait for connection
    uint32_t startTime = millis();
    while (WiFi.status() != WL_CONNECTED && 
           (millis() - startTime) < (_config.connectTimeout * 1000)) {
        delay(100);
        updateLED(); // Show connecting status
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        debugPrintf("Connected! IP: %s", WiFi.localIP().toString().c_str());
        setStatus(ConnectionStatus::CONNECTED);
        _reconnectAttempts = 0;
        
        if (_onConnect) _onConnect();
        return true;
    } else {
        debugPrint("Connection failed");
        setStatus(ConnectionStatus::DISCONNECTED);
        return false;
    }
}

void PicoWiFiManager::handleReconnection() {
    if (WiFi.status() == WL_CONNECTED || _configMode) {
        return;
    }
    
    uint32_t now = millis();
    if (now - _lastReconnectAttempt < 10000) { // Wait 10 seconds between attempts
        return;
    }
    
    if (_reconnectAttempts >= _config.maxReconnectAttempts) {
        debugPrint("Max reconnection attempts reached, starting config portal");
        startConfigPortal();
        return;
    }
    
    debugPrintf("Reconnection attempt %d/%d", _reconnectAttempts + 1, _config.maxReconnectAttempts);
    _lastReconnectAttempt = now;
    _reconnectAttempts++;
    
    WiFiCredentials credentials;
    if (_storage->loadWiFiCredentials(credentials) && credentials.valid) {
        connectWiFi(credentials.ssid, credentials.password);
    }
}

void PicoWiFiManager::updateLED() {
    if (_config.ledPin == 255) return; // LED disabled
    
    uint32_t now = millis();
    uint32_t interval;
    
    switch (_status) {
        case ConnectionStatus::CONNECTED:
            digitalWrite(_config.ledPin, HIGH);
            return;
            
        case ConnectionStatus::CONNECTING:
            interval = 200; // Fast blink
            break;
            
        case ConnectionStatus::CONFIG_MODE:
            interval = 100; // Very fast blink
            break;
            
        case ConnectionStatus::ERROR:
            interval = 1000; // Slow blink
            break;
            
        default:
            digitalWrite(_config.ledPin, LOW);
            return;
    }
    
    if (now - _lastLEDUpdate >= interval) {
        _ledState = !_ledState;
        digitalWrite(_config.ledPin, _ledState);
        _lastLEDUpdate = now;
    }
}

void PicoWiFiManager::setStatus(ConnectionStatus status) {
    if (_status != status) {
        _status = status;
        debugPrintf("Status changed to: %s", getStatusString().c_str());
        
        if (_onStatusChange) {
            _onStatusChange(status);
        }
    }
}

void PicoWiFiManager::checkResetButton() {
    if (_config.resetPin == 255) return; // Reset button disabled
    
    static uint32_t pressStart = 0;
    static bool pressed = false;
    
    bool currentState = digitalRead(_config.resetPin) == LOW;
    
    if (currentState && !pressed) {
        // Button just pressed
        pressed = true;
        pressStart = millis();
    } else if (!currentState && pressed) {
        // Button released
        pressed = false;
        uint32_t pressDuration = millis() - pressStart;
        
        if (pressDuration > 3000) { // Long press - factory reset
            debugPrint("Factory reset triggered");
            reset();
        } else if (pressDuration > 100) { // Short press - restart config portal
            debugPrint("Config portal restart triggered");
            if (!_configMode) {
                startConfigPortal();
            }
        }
    }
}

void PicoWiFiManager::reset() {
    debugPrint("Performing factory reset");
    
    stopConfigPortal();
    WiFi.disconnect();
    
    if (_storage) {
        _storage->clearAll();
    }
    
    delay(1000);
    rp2040.restart();
}

// Getters
ConnectionStatus PicoWiFiManager::getStatus() const {
    return _status;
}

String PicoWiFiManager::getStatusString() const {
    switch (_status) {
        case ConnectionStatus::DISCONNECTED: return "Disconnected";
        case ConnectionStatus::CONNECTING: return "Connecting";
        case ConnectionStatus::CONNECTED: return "Connected";
        case ConnectionStatus::CONFIG_MODE: return "Config Mode";
        case ConnectionStatus::ERROR: return "Error";
        default: return "Unknown";
    }
}

bool PicoWiFiManager::isConnected() const {
    return _status == ConnectionStatus::CONNECTED && WiFi.status() == WL_CONNECTED;
}

bool PicoWiFiManager::isConfigMode() const {
    return _configMode;
}

String PicoWiFiManager::getSSID() const {
    return WiFi.SSID();
}

IPAddress PicoWiFiManager::getLocalIP() const {
    return WiFi.localIP();
}

int32_t PicoWiFiManager::getRSSI() const {
    return WiFi.RSSI();
}

String PicoWiFiManager::getMACAddress() const {
    return WiFi.macAddress();
}

uint32_t PicoWiFiManager::getUptime() const {
    return millis() - _startTime;
}

size_t PicoWiFiManager::getFreeHeap() const {
    return rp2040.getFreeHeap();
}

// Configuration setters
void PicoWiFiManager::setConfig(const PicoWiFiConfig& config) {
    _config = config;
    _debugEnabled = config.enableSerial;
}

void PicoWiFiManager::setDeviceName(const char* name) {
    if (name && strlen(name) > 0) {
        strncpy(_config.deviceName, name, sizeof(_config.deviceName) - 1);
        _config.deviceName[sizeof(_config.deviceName) - 1] = '\0';
    }
}

void PicoWiFiManager::setTimeout(uint16_t seconds) {
    _config.configPortalTimeout = seconds;
}

// Callback setters
void PicoWiFiManager::onConfigModeStart(PicoWiFiCallback callback) {
    _onConfigStart = callback;
}

void PicoWiFiManager::onConfigModeEnd(PicoWiFiCallback callback) {
    _onConfigEnd = callback;
}

void PicoWiFiManager::onConnect(PicoWiFiCallback callback) {
    _onConnect = callback;
}

void PicoWiFiManager::onDisconnect(PicoWiFiCallback callback) {
    _onDisconnect = callback;
}

void PicoWiFiManager::onStatusChange(StatusCallback callback) {
    _onStatusChange = callback;
}

void PicoWiFiManager::enableDebug(bool enable) {
    _debugEnabled = enable;
}

bool PicoWiFiManager::isDualCoreEnabled() const {
    return _dualCoreEnabled;
}

void PicoWiFiManager::enableDualCore(bool enable) {
    _dualCoreEnabled = enable;
}

// Debug helpers
void PicoWiFiManager::debugPrint(const String& message) const {
    if (_debugEnabled) {
        Serial.printf("[PicoWiFiManager] %s\n", message.c_str());
    }
}

void PicoWiFiManager::debugPrintf(const char* format, ...) const {
    if (_debugEnabled) {
        va_list args;
        va_start(args, format);
        Serial.printf("[PicoWiFiManager] ");
        Serial.printf(format, args);
        Serial.println();
        va_end(args);
    }
}

void PicoWiFiManager::printDiagnostics() const {
    Serial.println("=== PicoWiFiManager Diagnostics ===");
    Serial.printf("Status: %s\n", getStatusString().c_str());
    Serial.printf("Config Mode: %s\n", _configMode ? "Yes" : "No");
    Serial.printf("Uptime: %lu ms\n", getUptime());
    Serial.printf("Free Heap: %zu bytes\n", getFreeHeap());
    
    if (isConnected()) {
        Serial.printf("SSID: %s\n", getSSID().c_str());
        Serial.printf("IP: %s\n", getLocalIP().toString().c_str());
        Serial.printf("RSSI: %ld dBm\n", getRSSI());
        Serial.printf("MAC: %s\n", getMACAddress().c_str());
    }
    
    if (_storage) {
        _storage->printDiagnostics();
    }
    
    Serial.println("=====================================");
}
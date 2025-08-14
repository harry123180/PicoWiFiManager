/**
 * StorageManager - Persistent Storage Implementation
 */

#include "StorageManager.h"

StorageManager::StorageManager() 
    : _initialized(false)
    , _eepromSize(512) {
}

StorageManager::~StorageManager() {
}

bool StorageManager::begin(size_t eepromSize) {
    _eepromSize = eepromSize;
    
    EEPROM.begin(_eepromSize);
    
    if (!loadFromEEPROM()) {
        Serial.println("No valid storage data found, initializing defaults");
        initializeDefaults();
        saveToEEPROM();
    }
    
    _initialized = true;
    Serial.println("StorageManager initialized");
    return true;
}

void StorageManager::format() {
    initializeDefaults();
    saveToEEPROM();
    Serial.println("Storage formatted");
}

bool StorageManager::saveWiFiCredentials(const char* ssid, const char* password) {
    if (!_initialized || !ssid) return false;
    
    strncpy(_data.wifi.ssid, ssid, sizeof(_data.wifi.ssid) - 1);
    _data.wifi.ssid[sizeof(_data.wifi.ssid) - 1] = '\0';
    
    if (password) {
        strncpy(_data.wifi.password, password, sizeof(_data.wifi.password) - 1);
        _data.wifi.password[sizeof(_data.wifi.password) - 1] = '\0';
    } else {
        memset(_data.wifi.password, 0, sizeof(_data.wifi.password));
    }
    
    _data.wifi.valid = true;
    
    return saveToEEPROM();
}

bool StorageManager::loadWiFiCredentials(WiFiCredentials& credentials) {
    if (!_initialized) return false;
    
    credentials = _data.wifi;
    return credentials.valid;
}

bool StorageManager::hasWiFiCredentials() {
    return _initialized && _data.wifi.valid;
}

void StorageManager::clearWiFiCredentials() {
    _data.wifi.clear();
    saveToEEPROM();
}

bool StorageManager::saveNetworkConfig(const NetworkConfig& config) {
    if (!_initialized) return false;
    
    _data.network = config;
    return saveToEEPROM();
}

bool StorageManager::loadNetworkConfig(NetworkConfig& config) {
    if (!_initialized) return false;
    
    config = _data.network;
    return true;
}

void StorageManager::clearNetworkConfig() {
    _data.network = NetworkConfig();
    saveToEEPROM();
}

bool StorageManager::saveDeviceConfig(const DeviceConfig& config) {
    if (!_initialized) return false;
    
    _data.device = config;
    return saveToEEPROM();
}

bool StorageManager::loadDeviceConfig(DeviceConfig& config) {
    if (!_initialized) return false;
    
    config = _data.device;
    return true;
}

void StorageManager::clearDeviceConfig() {
    _data.device = DeviceConfig();
    saveToEEPROM();
}

bool StorageManager::saveAll(const WiFiCredentials& wifi, 
                           const NetworkConfig& network, 
                           const DeviceConfig& device) {
    if (!_initialized) return false;
    
    _data.wifi = wifi;
    _data.network = network;
    _data.device = device;
    
    return saveToEEPROM();
}

bool StorageManager::loadAll(WiFiCredentials& wifi, 
                           NetworkConfig& network, 
                           DeviceConfig& device) {
    if (!_initialized) return false;
    
    wifi = _data.wifi;
    network = _data.network;
    device = _data.device;
    
    return true;
}

void StorageManager::clearAll() {
    _data.wifi.clear();
    _data.network = NetworkConfig();
    _data.device = DeviceConfig();
    saveToEEPROM();
}

bool StorageManager::isValid() {
    return _initialized && validateData(_data);
}

bool StorageManager::isCorrupted() {
    return _initialized && !validateData(_data);
}

uint32_t StorageManager::getChecksum() {
    return _data.checksum;
}

size_t StorageManager::getUsedSpace() {
    return sizeof(StorageData);
}

size_t StorageManager::getTotalSpace() {
    return _eepromSize;
}

void StorageManager::printDiagnostics() {
    Serial.println("=== Storage Manager Diagnostics ===");
    Serial.printf("Initialized: %s\n", _initialized ? "Yes" : "No");
    Serial.printf("EEPROM Size: %zu bytes\n", _eepromSize);
    Serial.printf("Used Space: %zu bytes\n", getUsedSpace());
    Serial.printf("Valid: %s\n", isValid() ? "Yes" : "No");
    Serial.printf("Magic: 0x%08X\n", _data.magic);
    Serial.printf("Version: %d\n", _data.version);
    Serial.printf("Checksum: 0x%08X\n", _data.checksum);
    
    if (_data.wifi.valid) {
        Serial.printf("WiFi SSID: %s\n", _data.wifi.ssid);
        Serial.println("WiFi Password: [Set]");
    } else {
        Serial.println("WiFi: Not configured");
    }
    
    Serial.println("====================================");
}

bool StorageManager::performIntegrityCheck() {
    if (!_initialized) return false;
    
    return validateData(_data);
}

bool StorageManager::repairIfNeeded() {
    if (!_initialized) return false;
    
    if (!validateData(_data)) {
        Serial.println("Storage corrupted, attempting repair...");
        initializeDefaults();
        return saveToEEPROM();
    }
    
    return true;
}

// Private methods
bool StorageManager::loadFromEEPROM() {
    EEPROM.get(EEPROM_START_ADDRESS, _data);
    return validateData(_data);
}

bool StorageManager::saveToEEPROM() {
    _data.checksum = calculateChecksum(_data);
    EEPROM.put(EEPROM_START_ADDRESS, _data);
    EEPROM.commit();
    return true;
}

uint32_t StorageManager::calculateChecksum(const StorageData& data) {
    // Simple XOR checksum for demonstration
    // In production, use CRC32
    uint32_t checksum = 0;
    const uint8_t* ptr = (const uint8_t*)&data;
    size_t size = sizeof(StorageData) - sizeof(data.checksum);
    
    for (size_t i = 0; i < size; i++) {
        checksum ^= ptr[i];
    }
    
    return checksum;
}

bool StorageManager::validateData(const StorageData& data) {
    // Check magic number
    if (data.magic != STORAGE_MAGIC) {
        return false;
    }
    
    // Check version
    if (data.version != STORAGE_VERSION) {
        return false;
    }
    
    // Check checksum
    uint32_t expectedChecksum = calculateChecksum(data);
    if (data.checksum != expectedChecksum) {
        return false;
    }
    
    // Validate WiFi credentials if marked as valid
    if (data.wifi.valid) {
        if (!isValidSSID(data.wifi.ssid)) {
            return false;
        }
    }
    
    return true;
}

void StorageManager::initializeDefaults() {
    _data = StorageData(); // Reset to defaults
    _data.magic = STORAGE_MAGIC;
    _data.version = STORAGE_VERSION;
    _data.checksum = 0;
}

bool StorageManager::isValidSSID(const char* ssid) {
    if (!ssid) return false;
    
    int len = strlen(ssid);
    if (len == 0 || len > 31) return false;
    
    // Check for valid printable ASCII characters
    for (int i = 0; i < len; i++) {
        if (ssid[i] < 32 || ssid[i] > 126) {
            return false;
        }
    }
    
    return true;
}

bool StorageManager::isValidPassword(const char* password) {
    if (!password) return true; // Empty password is valid for open networks
    
    int len = strlen(password);
    if (len > 63) return false; // WPA max password length
    
    return true;
}

bool StorageManager::isValidHostname(const char* hostname) {
    if (!hostname) return false;
    
    int len = strlen(hostname);
    if (len == 0 || len > 31) return false;
    
    return true;
}

bool StorageManager::isValidIP(uint32_t ip) {
    return ip != 0;
}

void StorageManager::debugPrint(const String& message) {
    Serial.printf("[StorageManager] %s\n", message.c_str());
}

void StorageManager::debugPrintf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    Serial.printf("[StorageManager] ");
    Serial.printf(format, args);
    Serial.println();
    va_end(args);
}
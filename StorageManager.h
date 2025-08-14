/**
 * StorageManager - Persistent Storage Manager for PicoWiFiManager
 * 
 * Handles WiFi credentials and configuration storage in Pico's Flash memory
 * Uses EEPROM emulation with data validation and corruption recovery
 */

#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include <Arduino.h>
#include <EEPROM.h>

// Storage structure version for migration support
#define STORAGE_VERSION 1
#define STORAGE_MAGIC 0x50494345  // Magic number to validate data (PICE in hex)

// Maximum sizes
#define MAX_SSID_LENGTH 32
#define MAX_PASSWORD_LENGTH 64
#define MAX_HOSTNAME_LENGTH 32

struct WiFiCredentials {
    char ssid[MAX_SSID_LENGTH];
    char password[MAX_PASSWORD_LENGTH];
    bool valid;
    
    WiFiCredentials() {
        memset(ssid, 0, sizeof(ssid));
        memset(password, 0, sizeof(password));
        valid = false;
    }
    
    void clear() {
        memset(ssid, 0, sizeof(ssid));
        memset(password, 0, sizeof(password));
        valid = false;
    }
};

struct NetworkConfig {
    bool useStaticIP;
    uint32_t staticIP;
    uint32_t gateway;
    uint32_t subnet;
    uint32_t primaryDNS;
    uint32_t secondaryDNS;
    
    NetworkConfig() {
        useStaticIP = false;
        staticIP = 0;
        gateway = 0;
        subnet = 0;
        primaryDNS = 0;
        secondaryDNS = 0;
    }
};

struct DeviceConfig {
    char hostname[MAX_HOSTNAME_LENGTH];
    bool autoReconnect;
    uint8_t maxReconnectAttempts;
    uint16_t connectTimeout;
    
    DeviceConfig() {
        strncpy(hostname, "pico2w", sizeof(hostname) - 1);
        hostname[sizeof(hostname) - 1] = '\0';
        autoReconnect = true;
        maxReconnectAttempts = 3;
        connectTimeout = 30;
    }
};

// Main storage structure
struct StorageData {
    uint32_t magic;           // Magic number for validation
    uint8_t version;          // Structure version
    uint32_t checksum;        // CRC32 checksum
    
    WiFiCredentials wifi;
    NetworkConfig network;
    DeviceConfig device;
    
    uint8_t reserved[64];     // Reserved for future use
    
    StorageData() {
        magic = STORAGE_MAGIC;
        version = STORAGE_VERSION;
        checksum = 0;
        memset(reserved, 0, sizeof(reserved));
    }
};

class StorageManager {
public:
    StorageManager();
    ~StorageManager();
    
    // Initialization
    bool begin(size_t eepromSize = 512);
    void format();
    
    // WiFi credentials management
    bool saveWiFiCredentials(const char* ssid, const char* password);
    bool loadWiFiCredentials(WiFiCredentials& credentials);
    bool hasWiFiCredentials();
    void clearWiFiCredentials();
    
    // Network configuration
    bool saveNetworkConfig(const NetworkConfig& config);
    bool loadNetworkConfig(NetworkConfig& config);
    void clearNetworkConfig();
    
    // Device configuration
    bool saveDeviceConfig(const DeviceConfig& config);
    bool loadDeviceConfig(DeviceConfig& config);
    void clearDeviceConfig();
    
    // Complete storage operations
    bool saveAll(const WiFiCredentials& wifi, 
                 const NetworkConfig& network, 
                 const DeviceConfig& device);
    bool loadAll(WiFiCredentials& wifi, 
                 NetworkConfig& network, 
                 DeviceConfig& device);
    void clearAll();
    
    // Storage status and diagnostics
    bool isValid();
    bool isCorrupted();
    uint32_t getChecksum();
    size_t getUsedSpace();
    size_t getTotalSpace();
    
    // Debug and maintenance
    void printDiagnostics();
    bool performIntegrityCheck();
    bool repairIfNeeded();

private:
    bool _initialized;
    size_t _eepromSize;
    StorageData _data;
    
    // Internal operations
    bool loadFromEEPROM();
    bool saveToEEPROM();
    uint32_t calculateChecksum(const StorageData& data);
    bool validateData(const StorageData& data);
    void initializeDefaults();
    
    // Data validation helpers
    bool isValidSSID(const char* ssid);
    bool isValidPassword(const char* password);
    bool isValidHostname(const char* hostname);
    bool isValidIP(uint32_t ip);
    
    // Corruption recovery
    bool attemptRecovery();
    void createBackup();
    bool restoreFromBackup();
    
    // Utilities
    void debugPrint(const String& message);
    void debugPrintf(const char* format, ...);
    
    static const int EEPROM_START_ADDRESS = 0;
    static const uint32_t CRC32_POLYNOMIAL = 0xEDB88320;
    
    uint32_t crc32(const uint8_t* data, size_t length);
};

#endif // STORAGE_MANAGER_H
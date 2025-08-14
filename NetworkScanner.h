/**
 * NetworkScanner - WiFi Network Scanner for PicoWiFiManager
 * 
 * Handles WiFi network scanning, filtering, and sorting
 * Optimized for Pico 2 W with async scanning and caching
 */

#ifndef NETWORK_SCANNER_H
#define NETWORK_SCANNER_H

#include <Arduino.h>
#include <WiFi.h>
#include <vector>
#include <memory>

// Network information structure
struct ScannedNetwork {
    String ssid;
    int32_t rssi;
    uint8_t channel;
    uint8_t encType;
    String bssid;
    bool hidden;
    
    ScannedNetwork() : rssi(-100), channel(0), encType(ENC_TYPE_NONE), hidden(false) {}
    
    ScannedNetwork(const String& s, int32_t r, uint8_t c, uint8_t e, const String& b, bool h = false)
        : ssid(s), rssi(r), channel(c), encType(e), bssid(b), hidden(h) {}
    
    String getSecurityString() const {
        switch (encType) {
            case ENC_TYPE_NONE: return "Open";
            case ENC_TYPE_WEP: return "WEP";
            case ENC_TYPE_TKIP: return "WPA";
            case ENC_TYPE_CCMP: return "WPA2";
            case ENC_TYPE_AUTO: return "WPA/WPA2";
            default: return "Secured";
        }
    }
    
    bool isSecure() const {
        return encType != ENC_TYPE_NONE;
    }
    
    int getSignalQuality() const {
        if (rssi <= -100) return 0;
        if (rssi >= -50) return 100;
        return 2 * (rssi + 100);
    }
};

// Scan configuration
struct ScanConfig {
    bool showHidden = false;
    bool removeDuplicates = true;
    int8_t minSignalQuality = 10;    // Minimum signal quality percentage
    uint8_t maxResults = 20;         // Maximum networks to return
    uint32_t cacheTimeout = 30000;   // Cache timeout in milliseconds
    bool sortBySignal = true;        // Sort by signal strength (strongest first)
    bool async = true;               // Use async scanning when possible
    
    // Channel filtering (0 = all channels)
    uint8_t channelStart = 0;
    uint8_t channelEnd = 0;
};

class NetworkScanner {
public:
    NetworkScanner();
    ~NetworkScanner();
    
    // Configuration
    void setConfig(const ScanConfig& config);
    ScanConfig getConfig() const { return _config; }
    
    // Scanning operations
    bool startScan();
    bool startAsyncScan();
    bool isScanComplete();
    bool isScanInProgress();
    
    // Results retrieval
    std::vector<ScannedNetwork> getResults(bool forceRescan = false);
    int getNetworkCount();
    ScannedNetwork getNetwork(int index);
    
    // Network lookup
    bool findNetwork(const String& ssid, ScannedNetwork& network);
    bool isNetworkVisible(const String& ssid);
    int32_t getNetworkRSSI(const String& ssid);
    
    // Filtering and sorting
    void filterResults(std::vector<ScannedNetwork>& networks);
    void sortResults(std::vector<ScannedNetwork>& networks);
    void removeDuplicates(std::vector<ScannedNetwork>& networks);
    
    // Cache management
    void clearCache();
    bool isCacheValid();
    uint32_t getCacheAge();
    
    // Status and diagnostics
    bool isAvailable();
    String getLastError();
    void printResults();
    void printDiagnostics();
    
    // Callbacks
    typedef std::function<void(int networkCount)> ScanCompleteCallback;
    typedef std::function<void(const String& error)> ScanErrorCallback;
    
    void onScanComplete(ScanCompleteCallback callback) { _onScanComplete = callback; }
    void onScanError(ScanErrorCallback callback) { _onScanError = callback; }

private:
    ScanConfig _config;
    std::vector<ScannedNetwork> _networks;
    uint32_t _lastScanTime;
    bool _scanInProgress;
    bool _asyncScanStarted;
    String _lastError;
    
    // Callbacks
    ScanCompleteCallback _onScanComplete;
    ScanErrorCallback _onScanError;
    
    // Internal operations
    bool performScan();
    void processScanResults();
    bool validateSSID(const String& ssid);
    bool shouldIncludeNetwork(const ScannedNetwork& network);
    
    // Signal quality helpers
    static String getRSSIDescription(int32_t rssi);
    static String getChannelDescription(uint8_t channel);
    static bool compareBySignal(const ScannedNetwork& a, const ScannedNetwork& b);
    static bool compareBySSID(const ScannedNetwork& a, const ScannedNetwork& b);
    
    // Error handling
    void setError(const String& error);
    void clearError();
    
    // Debug helpers
    void debugPrint(const String& message);
    void debugPrintf(const char* format, ...);
    
    static const uint32_t DEFAULT_CACHE_TIMEOUT = 30000;
    static const int DEFAULT_MIN_SIGNAL_QUALITY = 10;
    static const int MAX_SCAN_NETWORKS = 50;
};

// Utility functions for network analysis
namespace NetworkUtils {
    String formatBSSID(const String& bssid);
    String formatSignalStrength(int32_t rssi);
    String getChannelInfo(uint8_t channel);
    bool isValidSSID(const String& ssid);
    int calculateDistance(int32_t rssi, int frequency = 2400);
    String getWiFiModeString(WiFiMode_t mode);
}

#endif // NETWORK_SCANNER_H
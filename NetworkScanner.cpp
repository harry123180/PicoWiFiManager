/**
 * NetworkScanner - WiFi Network Scanner Implementation
 */

#include "NetworkScanner.h"
#include <algorithm>

NetworkScanner::NetworkScanner() 
    : _lastScanTime(0)
    , _scanInProgress(false)
    , _asyncScanStarted(false) {
    _config = ScanConfig(); // Use defaults
}

NetworkScanner::~NetworkScanner() {
}

void NetworkScanner::setConfig(const ScanConfig& config) {
    _config = config;
}

bool NetworkScanner::startScan() {
    if (_scanInProgress) {
        return false;
    }
    
    _scanInProgress = true;
    _lastError = "";
    
    return performScan();
}

bool NetworkScanner::startAsyncScan() {
    // For simplicity, use synchronous scan
    // Pico may not support async WiFi scanning
    return startScan();
}

bool NetworkScanner::isScanComplete() {
    return !_scanInProgress;
}

bool NetworkScanner::isScanInProgress() {
    return _scanInProgress;
}

std::vector<ScannedNetwork> NetworkScanner::getResults(bool forceRescan) {
    if (forceRescan || !isCacheValid()) {
        startScan();
    }
    
    return _networks;
}

int NetworkScanner::getNetworkCount() {
    return _networks.size();
}

ScannedNetwork NetworkScanner::getNetwork(int index) {
    if (index >= 0 && index < _networks.size()) {
        return _networks[index];
    }
    return ScannedNetwork();
}

bool NetworkScanner::findNetwork(const String& ssid, ScannedNetwork& network) {
    for (const auto& net : _networks) {
        if (net.ssid.equals(ssid)) {
            network = net;
            return true;
        }
    }
    return false;
}

bool NetworkScanner::isNetworkVisible(const String& ssid) {
    ScannedNetwork dummy;
    return findNetwork(ssid, dummy);
}

int32_t NetworkScanner::getNetworkRSSI(const String& ssid) {
    ScannedNetwork network;
    if (findNetwork(ssid, network)) {
        return network.rssi;
    }
    return -100; // Very weak signal if not found
}

void NetworkScanner::filterResults(std::vector<ScannedNetwork>& networks) {
    auto it = networks.begin();
    while (it != networks.end()) {
        if (!shouldIncludeNetwork(*it)) {
            it = networks.erase(it);
        } else {
            ++it;
        }
    }
}

void NetworkScanner::sortResults(std::vector<ScannedNetwork>& networks) {
    if (_config.sortBySignal) {
        std::sort(networks.begin(), networks.end(), compareBySignal);
    } else {
        std::sort(networks.begin(), networks.end(), compareBySSID);
    }
}

void NetworkScanner::removeDuplicates(std::vector<ScannedNetwork>& networks) {
    if (!_config.removeDuplicates) return;
    
    auto it = std::unique(networks.begin(), networks.end(),
        [](const ScannedNetwork& a, const ScannedNetwork& b) {
            return a.ssid.equals(b.ssid);
        });
    
    networks.erase(it, networks.end());
}

void NetworkScanner::clearCache() {
    _networks.clear();
    _lastScanTime = 0;
}

bool NetworkScanner::isCacheValid() {
    return (millis() - _lastScanTime) < _config.cacheTimeout;
}

uint32_t NetworkScanner::getCacheAge() {
    return millis() - _lastScanTime;
}

bool NetworkScanner::isAvailable() {
    return true; // WiFi scanning is always available on Pico 2 W
}

String NetworkScanner::getLastError() {
    return _lastError;
}

void NetworkScanner::printResults() {
    Serial.printf("=== Network Scan Results (%d networks) ===\n", _networks.size());
    
    for (int i = 0; i < _networks.size(); i++) {
        const ScannedNetwork& net = _networks[i];
        Serial.printf("%2d: %-20s %4d dBm %3d%% Ch%2d %s %s\n",
            i + 1,
            net.ssid.c_str(),
            net.rssi,
            net.getSignalQuality(),
            net.channel,
            net.getSecurityString().c_str(),
            net.hidden ? "(Hidden)" : ""
        );
    }
    
    Serial.println("==========================================");
}

void NetworkScanner::printDiagnostics() {
    Serial.println("=== NetworkScanner Diagnostics ===");
    Serial.printf("Scan in progress: %s\n", _scanInProgress ? "Yes" : "No");
    Serial.printf("Networks found: %d\n", _networks.size());
    Serial.printf("Cache valid: %s\n", isCacheValid() ? "Yes" : "No");
    Serial.printf("Cache age: %lu ms\n", getCacheAge());
    Serial.printf("Last error: %s\n", _lastError.c_str());
    Serial.println("==================================");
}

// Private methods
bool NetworkScanner::performScan() {
    debugPrint("Starting WiFi scan...");
    
    int networkCount = WiFi.scanNetworks(_config.showHidden);
    
    if (networkCount < 0) {
        setError("Scan failed");
        _scanInProgress = false;
        return false;
    }
    
    debugPrintf("Found %d networks", networkCount);
    
    _networks.clear();
    
    for (int i = 0; i < networkCount && _networks.size() < _config.maxResults; i++) {
        ScannedNetwork network;
        network.ssid = WiFi.SSID(i);
        network.rssi = WiFi.RSSI(i);
        network.channel = WiFi.channel(i);
        network.encType = WiFi.encryptionType(i);
        uint8_t bssid[6];
        WiFi.BSSID(bssid);
        network.bssid = String("");
        for (int j = 0; j < 6; j++) {
            if (j > 0) network.bssid += ":";
            if (bssid[j] < 16) network.bssid += "0";
            network.bssid += String(bssid[j], HEX);
        }
        network.bssid.toUpperCase();
        network.hidden = (network.ssid.length() == 0);
        
        if (shouldIncludeNetwork(network)) {
            _networks.push_back(network);
        }
    }
    
    // Post-process results
    if (_config.removeDuplicates) {
        removeDuplicates(_networks);
    }
    
    if (_config.sortBySignal) {
        sortResults(_networks);
    }
    
    _lastScanTime = millis();
    _scanInProgress = false;
    
    debugPrintf("Scan complete: %d networks after filtering", _networks.size());
    
    if (_onScanComplete) {
        _onScanComplete(_networks.size());
    }
    
    return true;
}

bool NetworkScanner::shouldIncludeNetwork(const ScannedNetwork& network) {
    // Skip hidden networks if not configured to show them
    if (network.hidden && !_config.showHidden) {
        return false;
    }
    
    // Skip networks with weak signal
    if (network.getSignalQuality() < _config.minSignalQuality) {
        return false;
    }
    
    // Validate SSID
    if (!validateSSID(network.ssid)) {
        return false;
    }
    
    return true;
}

bool NetworkScanner::validateSSID(const String& ssid) {
    if (ssid.length() == 0 || ssid.length() > 32) {
        return false;
    }
    
    // Check for valid characters (printable ASCII)
    for (unsigned int i = 0; i < ssid.length(); i++) {
        char c = ssid.charAt(i);
        if (c < 32 || c > 126) {
            return false;
        }
    }
    
    return true;
}

void NetworkScanner::setError(const String& error) {
    _lastError = error;
    debugPrint("Error: " + error);
    
    if (_onScanError) {
        _onScanError(error);
    }
}

void NetworkScanner::clearError() {
    _lastError = "";
}

void NetworkScanner::debugPrint(const String& message) {
    Serial.printf("[NetworkScanner] %s\n", message.c_str());
}

void NetworkScanner::debugPrintf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    Serial.printf("[NetworkScanner] ");
    Serial.printf(format, args);
    Serial.println();
    va_end(args);
}

// Static comparison functions
bool NetworkScanner::compareBySignal(const ScannedNetwork& a, const ScannedNetwork& b) {
    return a.rssi > b.rssi; // Stronger signal first
}

bool NetworkScanner::compareBySSID(const ScannedNetwork& a, const ScannedNetwork& b) {
    return a.ssid < b.ssid; // Alphabetical order
}

// Utility functions
namespace NetworkUtils {
    String formatBSSID(const String& bssid) {
        String formatted = bssid;
        formatted.toUpperCase();
        return formatted;
    }
    
    String formatSignalStrength(int32_t rssi) {
        if (rssi >= -50) return "Excellent";
        if (rssi >= -60) return "Good";
        if (rssi >= -70) return "Fair";
        if (rssi >= -80) return "Weak";
        return "Very Weak";
    }
    
    String getChannelInfo(uint8_t channel) {
        if (channel >= 1 && channel <= 14) {
            return "2.4 GHz";
        }
        return "Unknown";
    }
    
    bool isValidSSID(const String& ssid) {
        return ssid.length() > 0 && ssid.length() <= 32;
    }
    
    int calculateDistance(int32_t rssi, int frequency) {
        // Simple distance estimation (very approximate)
        if (rssi == 0) return -1;
        
        double ratio = (double)(27.55 - (20 * log10(frequency)) + abs(rssi)) / 20.0;
        return (int)pow(10, ratio);
    }
    
    String getWiFiModeString(WiFiMode_t mode) {
        switch (mode) {
            case WIFI_OFF: return "OFF";
            case WIFI_STA: return "STA";
            case WIFI_AP: return "AP";
            case WIFI_AP_STA: return "AP_STA";
            default: return "Unknown";
        }
    }
}
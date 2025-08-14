/**
 * ConfigPortal - Web Configuration Portal for PicoWiFiManager
 * 
 * Handles the captive portal web interface for WiFi configuration
 * Optimized for Pico 2 W with responsive design and minimal memory usage
 */

#ifndef CONFIG_PORTAL_H
#define CONFIG_PORTAL_H

#include <Arduino.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <functional>

// Forward declarations
class PicoWiFiManager;
class NetworkScanner;

struct NetworkInfo {
    String ssid;
    int32_t rssi;
    bool encrypted;
    String security;
};

class ConfigPortal {
public:
    explicit ConfigPortal(PicoWiFiManager* manager);
    ~ConfigPortal();
    
    // Portal lifecycle
    bool start(const char* ssid, const char* password = nullptr);
    void stop();
    void handle();
    
    bool isActive() const { return _active; }
    IPAddress getAPIP() const { return _apIP; }
    
    // Configuration
    void setTimeout(uint32_t seconds);
    void setTitle(const String& title);
    void setCustomHTML(const String& html);
    
    // Callbacks
    typedef std::function<void(const String& ssid, const String& password)> ConnectCallback;
    typedef std::function<void()> ResetCallback;
    
    void onConnect(ConnectCallback callback) { _onConnect = callback; }
    void onReset(ResetCallback callback) { _onReset = callback; }

private:
    PicoWiFiManager* _manager;
    WebServer* _server;
    DNSServer* _dnsServer;
    NetworkScanner* _scanner;
    
    bool _active;
    IPAddress _apIP;
    uint32_t _timeout;
    uint32_t _startTime;
    String _title;
    String _customHTML;
    
    // Callbacks
    ConnectCallback _onConnect;
    ResetCallback _onReset;
    
    // HTTP handlers
    void handleRoot();
    void handleWiFi();
    void handleScan();
    void handleConnect();
    void handleInfo();
    void handleReset();
    void handleNotFound();
    void handleExit();
    
    // Setup
    void setupRoutes();
    
    // HTML generators
    String generateHeader();
    String generateFooter();
    String generateCSS();
    String generateJS();
    String generateNetworkList();
    String generateInfoPage();
    
    // Utilities
    String getSignalIcon(int32_t rssi);
    String getSecurityIcon(bool encrypted);
    String formatRSSI(int32_t rssi);
    bool captivePortalRedirect();
    
    // Template strings
    static const char* HTML_HEAD;
    static const char* HTML_CSS;
    static const char* HTML_JS;
    static const char* HTML_ROOT;
    static const char* HTML_WIFI_FORM;
    static const char* HTML_INFO;
    static const char* HTML_SUCCESS;
    static const char* HTML_FAIL;
};

#endif // CONFIG_PORTAL_H
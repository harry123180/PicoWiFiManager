/**
 * ConfigPortal - Web Configuration Portal Implementation
 */

#include "ConfigPortal.h"
#include "PicoWiFiManager.h"

ConfigPortal::ConfigPortal(PicoWiFiManager* manager) 
    : _manager(manager)
    , _server(nullptr)
    , _dnsServer(nullptr)
    , _scanner(nullptr)
    , _active(false)
    , _apIP(192, 168, 4, 1)
    , _timeout(300000)
    , _startTime(0)
    , _title("Pico WiFi Setup") {
}

ConfigPortal::~ConfigPortal() {
    if (_server) delete _server;
    if (_dnsServer) delete _dnsServer;
}

bool ConfigPortal::start(const char* ssid, const char* password) {
    Serial.println("Starting ConfigPortal...");
    
    // Stop any existing connection
    WiFi.disconnect();
    delay(100);
    
    // Start AP mode
    WiFi.mode(WIFI_AP);
    
    bool result;
    if (password && strlen(password) > 0) {
        result = WiFi.softAP(ssid, password);
    } else {
        result = WiFi.softAP(ssid);
    }
    
    if (!result) {
        Serial.println("Failed to start AP");
        return false;
    }
    
    delay(1000);
    _apIP = WiFi.softAPIP();
    
    // Create web server
    if (!_server) {
        _server = new WebServer(80);
    }
    
    // Setup routes
    setupRoutes();
    
    // Start DNS server for captive portal
    if (!_dnsServer) {
        _dnsServer = new DNSServer();
    }
    _dnsServer->start(53, "*", _apIP);
    
    _server->begin();
    _active = true;
    _startTime = millis();
    
    Serial.printf("AP started: %s\n", ssid);
    Serial.printf("IP: %s\n", _apIP.toString().c_str());
    
    return true;
}

void ConfigPortal::stop() {
    if (_active) {
        _active = false;
        if (_server) {
            _server->stop();
        }
        if (_dnsServer) {
            _dnsServer->stop();
        }
        WiFi.softAPdisconnect(true);
        Serial.println("ConfigPortal stopped");
    }
}

void ConfigPortal::handle() {
    if (_active && _server) {
        // Handle DNS requests for captive portal
        if (_dnsServer) {
            _dnsServer->processNextRequest();
        }
        
        _server->handleClient();
        
        // Check timeout
        if (_timeout > 0 && (millis() - _startTime > _timeout)) {
            Serial.println("ConfigPortal timeout");
            stop();
        }
    }
}

void ConfigPortal::setTimeout(uint32_t seconds) {
    _timeout = seconds * 1000;
}

void ConfigPortal::setTitle(const String& title) {
    _title = title;
}

void ConfigPortal::setCustomHTML(const String& html) {
    _customHTML = html;
}

void ConfigPortal::setupRoutes() {
    _server->on("/", [this]() { handleRoot(); });
    _server->on("/scan", [this]() { handleScan(); });
    _server->on("/connect", HTTP_POST, [this]() { handleConnect(); });
    _server->on("/info", [this]() { handleInfo(); });
    _server->on("/reset", [this]() { handleReset(); });
    
    // Apple captive portal detection
    _server->on("/hotspot-detect.html", [this]() { handleRoot(); });
    _server->on("/library/test/success.html", [this]() { handleRoot(); });
    _server->on("/captive", [this]() { handleRoot(); });
    
    // Microsoft Windows captive portal detection
    _server->on("/ncsi.txt", [this]() { 
        _server->send(200, "text/plain", "Microsoft NCSI"); 
    });
    _server->on("/connecttest.txt", [this]() { 
        _server->send(200, "text/plain", "Microsoft Connect Test"); 
    });
    
    // Android captive portal detection
    _server->on("/generate_204", [this]() { 
        _server->sendHeader("Location", "/");
        _server->send(302, "text/plain", ""); 
    });
    
    _server->onNotFound([this]() { handleNotFound(); });
}

void ConfigPortal::handleRoot() {
    // Add proper headers for captive portal detection
    _server->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    _server->sendHeader("Pragma", "no-cache");
    _server->sendHeader("Expires", "-1");
    
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<title>" + _title + "</title>";
    html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
    html += "<meta http-equiv='Cache-Control' content='no-cache, no-store, must-revalidate'>";
    html += "<meta http-equiv='Pragma' content='no-cache'>";
    html += "<meta http-equiv='Expires' content='0'>";
    html += "<style>";
    html += "body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,Arial,sans-serif;margin:20px;background:#f5f5f5}";
    html += ".container{max-width:400px;margin:0 auto;background:white;padding:20px;border-radius:8px;box-shadow:0 2px 10px rgba(0,0,0,0.1)}";
    html += "h1{color:#333;text-align:center;margin-bottom:30px}";
    html += "h3{color:#666;margin-bottom:15px}";
    html += ".network-item{background:#f8f9fa;margin:5px 0;padding:10px;border-radius:4px;cursor:pointer;border:1px solid #e9ecef}";
    html += ".network-item:hover{background:#e9ecef}";
    html += ".btn{background:#007cba;color:white;padding:12px 24px;border:none;border-radius:4px;cursor:pointer;width:100%;font-size:16px;margin:5px 0}";
    html += ".btn:hover{background:#005a87}";
    html += "input[type=text],input[type=password]{width:100%;padding:12px;margin:5px 0;border:1px solid #ccc;border-radius:4px;box-sizing:border-box;font-size:16px}";
    html += ".btn-secondary{background:#6c757d;margin-right:10px;width:auto;display:inline-block}";
    html += "</style></head><body>";
    
    html += "<div class='container'>";
    html += "<h1>WiFi " + _title + "</h1>";
    html += "<h3>選擇網路:</h3>";
    
    // Scan networks
    int networks = WiFi.scanNetworks();
    if (networks == 0) {
        html += "<p style='text-align:center;color:#666'>未找到網路</p>";
    } else {
        for (int i = 0; i < networks && i < 10; i++) {
            String ssid = WiFi.SSID(i);
            int32_t rssi = WiFi.RSSI(i);
            bool encrypted = (WiFi.encryptionType(i) != ENC_TYPE_NONE);
            
            // Signal strength indicator
            String signalIcon = "●●●●";
            String lockIcon = encrypted ? " [加密]" : " [開放]";
            
            if (rssi > -50) signalIcon = "●●●●";      // Excellent
            else if (rssi > -65) signalIcon = "●●●○"; // Good
            else if (rssi > -80) signalIcon = "●●○○"; // Fair
            else signalIcon = "●○○○";                  // Weak
            
            html += "<div class='network-item' onclick='document.getElementById(\"ssid\").value=\"" + ssid + "\"'>";
            html += signalIcon + " " + ssid + " (" + String(rssi) + " dBm)" + lockIcon;
            html += "</div>";
        }
    }
    
    html += "<hr style='margin:20px 0'>";
    html += "<form action='/connect' method='post'>";
    html += "<p><input type='text' id='ssid' name='ssid' placeholder='網路名稱 (SSID)' required></p>";
    html += "<p><input type='password' name='password' placeholder='密碼 (如需要)'></p>";
    html += "<p><button type='submit' class='btn'>連接網路</button></p>";
    html += "</form>";
    
    html += "<div style='text-align:center;margin-top:20px'>";
    html += "<a href='/scan' class='btn btn-secondary'>重新掃描</a> ";
    html += "<a href='/info' class='btn btn-secondary'>設備資訊</a> ";
    html += "<a href='/reset' class='btn btn-secondary'>重置設備</a>";
    html += "</div>";
    
    if (!_customHTML.isEmpty()) {
        html += "<hr style='margin:20px 0'>" + _customHTML;
    }
    
    html += "</div></body></html>";
    
    _server->send(200, "text/html; charset=utf-8", html);
}

void ConfigPortal::handleScan() {
    _server->sendHeader("Location", "/");
    _server->send(302, "text/plain", "");
}

void ConfigPortal::handleConnect() {
    String ssid = _server->arg("ssid");
    String password = _server->arg("password");
    
    if (ssid.length() == 0) {
        _server->send(400, "text/html; charset=utf-8", "<h1>錯誤</h1><p>需要輸入網路名稱</p><a href='/'>返回</a>");
        return;
    }
    
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<title>連線中...</title>";
    html += "<meta http-equiv='refresh' content='10;url=/result'>";
    html += "</head><body><h1>正在連線到 " + ssid + "...</h1>";
    html += "<p>請等待...</p></body></html>";
    
    _server->send(200, "text/html; charset=utf-8", html);
    
    // Trigger callback
    if (_onConnect) {
        _onConnect(ssid, password);
    }
}

void ConfigPortal::handleInfo() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<title>設備資訊</title></head><body>";
    html += "<h1>設備資訊</h1>";
    html += "<p><strong>晶片 ID:</strong> " + String(rp2040.hwrand32(), HEX) + "</p>";
    html += "<p><strong>可用記憶體:</strong> " + String(rp2040.getFreeHeap()) + " bytes</p>";
    html += "<p><strong>運行時間:</strong> " + String(millis() / 1000) + " 秒</p>";
    html += "<p><strong>AP IP:</strong> " + _apIP.toString() + "</p>";
    html += "<br><a href='/'>返回</a>";
    html += "</body></html>";
    
    _server->send(200, "text/html; charset=utf-8", html);
}

void ConfigPortal::handleReset() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<title>重置中</title></head><body>";
    html += "<h1>重置中...</h1>";
    html += "<p>設備將在 3 秒後重新啟動。</p>";
    html += "</body></html>";
    
    _server->send(200, "text/html; charset=utf-8", html);
    delay(2000);
    
    if (_onReset) {
        _onReset();
    }
}

void ConfigPortal::handleNotFound() {
    String uri = _server->uri();
    
    // Special handling for common captive portal detection URLs
    if (uri.equals("/hotspot-detect.html") || 
        uri.equals("/library/test/success.html") ||
        uri.equals("/captive") ||
        uri.equals("/generate_204")) {
        handleRoot();
        return;
    }
    
    // For all other requests, redirect to root with proper headers
    _server->sendHeader("Location", "http://" + _apIP.toString() + "/");
    _server->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    _server->sendHeader("Pragma", "no-cache");
    _server->sendHeader("Expires", "-1");
    _server->send(302, "text/plain", "Redirecting to captive portal");
}
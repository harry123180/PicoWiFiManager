# PicoWiFiManager

[中文版 (Chinese)](README_CN.md) | [English Version](README.md)

## 📶 專為 Raspberry Pi Pico 2 W 設計的強大 WiFi 管理庫

**PicoWiFiManager** 是專為 **Raspberry Pi Pico 2 W** 設計和優化的綜合性 WiFi 連接管理庫。提供無縫的 WiFi 連接、強制門戶支援、持久化存儲和先進的雙核心功能。

## ✨ 主要功能

- **🔌 自動連接管理**：自動連接到已保存的 WiFi 網路，失敗時回退到配置門戶
- **🌐 強制門戶**：用戶友好的網頁界面，支援多設備兼容（iPhone、Android、Windows）
- **💾 持久化存儲**：基於 EEPROM 的憑證存儲，具有數據完整性保護
- **⚡ 雙核心支援**：充分利用 Pico 2 W 的雙核心架構以實現最佳性能
- **📱 手機友好**：針對智慧手機和平板電腦優化的響應式設計
- **🔧 高級配置**：靜態 IP、自定義 DNS、超時設定等
- **🛡️ 強大的錯誤處理**：自動重連、損壞檢測和恢復
- **📊 全面的診斷**：詳細的狀態監控和除錯工具
- **🚀 Pico 優化**：專為 RP2350 + CYW43439 WiFi 晶片而建
- **🔍 網路掃描器**：高級 WiFi 網路掃描和篩選

## 🏗️ 架構

```
PicoWiFiManager/
├── PicoWiFiManager.h      # 主類別接口
├── PicoWiFiManager.cpp    # 核心實現  
├── ConfigPortal.h         # 網頁配置界面
├── StorageManager.h       # Flash 存儲管理
├── NetworkScanner.h       # WiFi 掃描和篩選
└── examples/
    ├── Basic/             # 簡單使用範例
    ├── Advanced/          # 功能演示
    └── DualCore/          # 雙核心利用
```

## 📚 範例概述

### 🟢 Basic 範例 - 簡單集成
**適合初學者和簡單專案**
- 基本的 WiFi 連接管理
- 自動連接到已保存的網路
- 失敗時回退到配置門戶
- 簡單的狀態監控

**使用場景**：IoT 感測器、簡單開關、基本自動化

### 🔵 Advanced 範例 - 全功能
**適合複雜專案的完整功能**
- 自定義配置參數
- 靜態 IP 設定與 DNS 配置
- 詳細的回調處理
- 串列埠命令界面
- 網路掃描功能
- 全面的診斷和狀態報告

**使用場景**：智慧家居控制器、網頁服務器、數據記錄系統

### 🟣 DualCore 範例 - 高性能
**針對性能關鍵應用優化**
- **Core 0**：專門處理 WiFi 管理和網路操作
- **Core 1**：應用邏輯和感測器處理
- 核心間通信與互斥鎖保護
- 獨立任務管理
- 系統資源監控

**使用場景**：即時控制系統、高頻數據採集、感測器密集型應用

## 🚀 快速開始

### 安裝

1. 下載庫作為 ZIP 檔案
2. 在 Arduino IDE 中：`草圖` → `匯入程式庫` → `匯入 .ZIP 程式庫`
3. 選擇 PicoWiFiManager ZIP 檔案
4. 庫將出現在範例中：`檔案` → `範例` → `PicoWiFiManager`

### 基本使用

```cpp
#include "PicoWiFiManager.h"

PicoWiFiManager wifiManager;

void setup() {
    Serial.begin(115200);
    
    // 初始化並連接
    wifiManager.begin();
    wifiManager.setDeviceName("MyPico2W");
    
    if (wifiManager.autoConnect()) {
        Serial.println("WiFi 已連接！");
    }
}

void loop() {
    wifiManager.loop(); // 處理 WiFi 事件
    // 你的應用程式碼
}
```

## 🏗️ 專案整合指南

### 選擇合適的範例

| 專案類型 | 推薦範例 | 原因 |
|---------|---------|------|
| IoT 感測器 | **Basic** | 簡單、低功耗 |
| 智慧家居控制 | **Advanced** | 需要靜態 IP 和詳細控制 |
| 數據採集 | **DualCore** | 高頻率感測器處理 |
| 網頁服務器 | **Advanced** | 需要完整網路功能 |
| 即時控制 | **DualCore** | 需要穩定的即時響應 |
| 簡單開關控制 | **Basic** | 基本功能足夠 |

### 整合步驟

1. **選擇合適的範例**作為起點
2. **複製基本結構**到你的專案
3. **替換應用邏輯**部分為你的功能
4. **保持 WiFi 管理**部分不變
5. **根據需要調整**配置參數

## 📖 API 參考

### 核心方法
- `begin()` - 初始化 WiFi 管理器
- `autoConnect()` - 嘗試自動連接
- `startConfigPortal()` - 啟動配置門戶
- `loop()` - 處理 WiFi 事件（在主迴圈中呼叫）

### 配置
- `setDeviceName(name)` - 設定設備/AP 名稱
- `setTimeout(seconds)` - 設定門戶超時
- `setConfig(config)` - 應用自定義配置

### 狀態和資訊
- `isConnected()` - 檢查連接狀態
- `getSSID()` - 獲取已連接的網路名稱
- `getLocalIP()` - 獲取分配的 IP 地址
- `getRSSI()` - 獲取信號強度
- `printDiagnostics()` - 列印詳細診斷

## 🌐 強制門戶功能

- **多設備兼容性**：支援 iPhone、Android、Windows 等
- **自動檢測**：設備會自動顯示配置頁面
- **網路掃描**：顯示可用的 WiFi 網路和信號強度
- **用戶友好界面**：為手機設備優化的乾淨響應式設計
- **UTF-8 支援**：完全支援國際字符

### 多設備兼容性展示

配置門戶針對所有設備進行了響應式設計優化：

<div align="center">

| iOS 設備 | Android 設備 |
|:--------:|:------------:|
| ![iOS 配置門戶](image/iOSView.jpg) | ![Android 配置門戶](image/AndroidView.jpg) |
| **iPhone/iPad 界面** | **Android 界面** |

</div>

✅ **測試通過的設備**：iPhone、iPad、Android 手機/平板、Windows PC、Mac、Linux 瀏覽器

## ⚙️ 高級配置

```cpp
PicoWiFiConfig config;
strcpy(config.deviceName, "MyAdvancedPico");
strcpy(config.apPassword, "mypassword123");
config.configPortalTimeout = 600;  // 10 分鐘
config.connectTimeout = 20;        // 20 秒
config.useStaticIP = true;
config.staticIP = IPAddress(192, 168, 1, 100);
config.gateway = IPAddress(192, 168, 1, 1);
config.subnet = IPAddress(255, 255, 255, 0);
config.autoReconnect = true;

wifiManager.setConfig(config);
```

## 🔄 雙核心支援

利用 Pico 的雙核心架構：

```cpp
PicoWiFiManager wifiManager;

void setup() {
    wifiManager.begin();
    wifiManager.enableDualCore(true);
    
    // Core 0: 自動 WiFi 管理
    // Core 1: 你的應用程式碼
    multicore_launch_core1(core1_task);
}

void core1_task() {
    while (true) {
        // 你的密集應用邏輯
        // WiFi 在 Core 0 上管理
        processSensors();
        runAlgorithms();
        sleep_ms(100);
    }
}

void loop() {
    // Core 0: 處理 WiFi 事件
    wifiManager.loop();
}
```

## 💾 存儲管理

具有損壞恢復的持久化存儲：

```cpp
// 存儲會自動處理，但你可以訪問它：
StorageManager storage;
storage.begin();

// 保存自定義 WiFi 憑證
storage.saveWiFiCredentials("MyNetwork", "password123");

// 載入憑證
WiFiCredentials creds;
if (storage.loadWiFiCredentials(creds)) {
    Serial.println("已載入：" + String(creds.ssid));
}

// 存儲診斷
storage.printDiagnostics();
storage.performIntegrityCheck();
```

## 🛠️ 硬體需求

- **Raspberry Pi Pico 2 W**（RP2350 與 CYW43439 WiFi 晶片）
- **Arduino-Pico Core** by Earle F. Philhower
- 最少 2MB 快閃記憶體（用於庫和你的應用）

## 🛠️ 硬體設定

### 所需硬體
- Raspberry Pi Pico 2 W
- 可選：GPIO 2 上的重置按鈕
- 可選：狀態 LED（預設使用內建 LED）

### Arduino IDE 設定

1. 安裝 **Arduino-Pico** 核心：
   - 添加網址：`https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json`
   - 安裝：「Raspberry Pi Pico/RP2040」by Earle F. Philhower, III

2. 選擇開發板：
   - **開發板**：「Raspberry Pi Pico 2 W」或「Generic RP2350」
   - **CPU 速度**：150 MHz（推薦）
   - **快閃記憶體大小**：4MB

3. 安裝 PicoWiFiManager：
   - 複製庫到 `Arduino/libraries/PicoWiFiManager/`

## 🐛 故障排除

### 常見問題

**WiFi 無法連接：**
```cpp
// 啟用除錯輸出
wifiManager.enableDebug(true);
wifiManager.printDiagnostics();

// 檢查已保存的憑證
StorageManager storage;
WiFiCredentials creds;
if (storage.loadWiFiCredentials(creds)) {
    Serial.println("已保存的 SSID：" + String(creds.ssid));
}
```

**配置門戶無法訪問：**
- 驗證 AP 已創建：在 WiFi 網路中尋找你的設備名稱
- 檢查 IP 地址：應該是 `192.168.4.1`
- 嘗試不同的瀏覽器或清除快取
- 確保設備名稱和密碼正確

## 📊 性能

### 記憶體使用
- **基本庫**：~8KB Flash，~2KB RAM
- **帶網頁門戶**：~15KB Flash，~4KB RAM  
- **全功能**：~25KB Flash，~6KB RAM

### 網路性能
- **掃描時間**：20 個網路約 2-3 秒
- **連接時間**：典型 3-5 秒
- **門戶響應**：<200ms 頁面載入
- **自動重連**：斷線後 <10 秒

## 📄 許可證

本專案採用 MIT 許可證：

```
MIT License

Copyright (c) 2024 harry123180

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

## 👨‍💻 作者與聯絡方式

**作者**：harry123180  
**Email**：harry123180@gmail.com

歡迎聯絡我提出問題、建議或貢獻！您的反饋和貢獻總是受歡迎的。

## 🤝 貢獻

我們歡迎貢獻！請隨時在 GitHub 上提交問題、功能請求或拉取請求。

## 🙏 致謝

- **Raspberry Pi Foundation** 提供 Pico 2 W 平台
- **Earle F. Philhower, III** 提供優秀的 Arduino-Pico 核心
- **Arduino Community** 提供基礎庫
- **ESP WiFiManager** 專案提供靈感（儘管這是完全重寫）

---

**由 harry123180 用 ❤️ 專為 Raspberry Pi Pico 2 W 打造**
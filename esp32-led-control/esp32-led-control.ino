// ==================== ESP32-C3 LED WebSocket 控制器 ====================
// 功能：純 WebSocket 控制 GPIO 10 的 LED（不需 MQTT Server）
// 搭配檔案：
//   - 前端頁面：web/esp32-led-control.html
//   - 後端伺服器：server/websocket-server.js (Port: 8080)
//
// 需要安裝的 Arduino 函式庫：
//   1. WebSockets (作者：Markus Sattler / Links2004)
//   2. ArduinoJson (作者：Benoit Blanchon)
// =====================================================================

#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

// ==================== WiFi 設定 ====================
const char* WIFI_SSID = "Techart_MESH";      // 修改為你的 WiFi 名稱
const char* WIFI_PASS = "035715131@a";      // 修改為你的 WiFi 密碼

// ==================== 網路設定 (固定 IP) ====================
// 若想改用 DHCP 自動取得 IP，請將下行註解掉
#define USE_STATIC_IP

#ifdef USE_STATIC_IP
IPAddress local_IP(192, 168, 50, 205);        // ESP32-C3 的固定 IP
IPAddress gateway(192, 168, 50, 1);           // 路由器閘道
IPAddress subnet(255, 255, 255, 0);            // 子網路遮罩
IPAddress primaryDNS(8, 8, 8, 8);              // DNS 伺服器
#endif

// ==================== WebSocket 伺服器設定 ====================
// 對應 server/websocket-server.js 的位址與端口
const char* WS_HOST = "192.168.50.116";       // WebSocket 伺服器 IP
const int   WS_PORT = 8081;                    // WebSocket 伺服器 Port
const char* WS_PATH = "/";                     // WebSocket 路徑

// ==================== 硬體設定 ====================
const int LED_PIN = 8;                        // LED 接在 ESP32-C3 的 GPIO 8

// ==================== 全域變數 ====================
WebSocketsClient webSocket;
bool currentLedState = false;                  // 目前 LED 狀態 (true = ON, false = OFF)

// 函式原型宣告
void connectWiFi();
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length);
void handleIncomingJson(const char* jsonStr);
void setLedState(bool isOn);
void sendLedStatus();

// ==================== Setup ====================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n==========================================");
  Serial.println("   ESP32-C3 純 WebSocket LED 控制器 (GPIO 10)");
  Serial.println("==========================================");

  // 設定 GPIO 10 為輸出並預設關閉
  pinMode(LED_PIN, OUTPUT);
  setLedState(false);

  // 連接 WiFi
  connectWiFi();

  // 設定並啟動 WebSocket 連線
  Serial.print("正在連接 WebSocket 伺服器: ws://");
  Serial.print(WS_HOST);
  Serial.print(":");
  Serial.println(WS_PORT);

  webSocket.begin(WS_HOST, WS_PORT, WS_PATH);
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);        // 斷線時每 5 秒自動重連
}

// ==================== Loop ====================
void loop() {
  // 確保 WiFi 保持連線
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("⚠️ WiFi 斷線，嘗試重新連接...");
    connectWiFi();
  }

  // 處理 WebSocket 收發與連線維護
  webSocket.loop();
}

// ==================== 控制 GPIO 10 LED ====================
void setLedState(bool isOn) {
  currentLedState = isOn;
  digitalWrite(LED_PIN, isOn ? HIGH : LOW);

  Serial.print("💡 GPIO ");
  Serial.print(LED_PIN);
  Serial.print(" LED 狀態: ");
  Serial.println(isOn ? "ON (HIGH)" : "OFF (LOW)");
}

// ==================== 回報 LED 狀態給 WebSocket 伺服器 ====================
void sendLedStatus() {
  StaticJsonDocument<128> doc;
  doc["type"] = "led_status";
  JsonObject payload = doc.createNestedObject("payload");
  payload["GPIO8"] = currentLedState ? "on" : "off";
  payload["GPIO10"] = currentLedState ? "on" : "off";
  payload["GPIO23"] = currentLedState ? "on" : "off";

  String output;
  serializeJson(doc, output);
  webSocket.sendTXT(output);

  Serial.print("📤 [WebSocket] 已回報狀態: ");
  Serial.println(output);
}

// ==================== WebSocket 事件處理 ====================
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.println("🔌 [WebSocket] 已斷線，等待重新連線...");
      break;

    case WStype_CONNECTED:
      Serial.print("✅ [WebSocket] 已連線至伺服器！路徑: ");
      Serial.println((char*)payload);
      sendLedStatus();  // 連線成功時主動回報目前 LED 狀態
      break;

    case WStype_TEXT:
      Serial.print("📥 [WebSocket] 收到訊息: ");
      Serial.println((char*)payload);
      handleIncomingJson((const char*)payload);
      break;

    case WStype_ERROR:
      Serial.println("❌ [WebSocket] 發生連線錯誤");
      break;

    default:
      break;
  }
}

// ==================== 解析 WebSocket JSON 訊息 ====================
void handleIncomingJson(const char* jsonStr) {
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, jsonStr);

  if (error) {
    Serial.print("❌ JSON 解析失敗: ");
    Serial.println(error.c_str());
    return;
  }

  const char* msgType = doc["type"];
  if (!msgType) return;

  String typeStr = String(msgType);

  // 1. 收到伺服器歡迎訊息
  if (typeStr == "welcome") {
    Serial.print("📨 伺服器訊息: ");
    Serial.println(doc["message"].as<const char*>());
    return;
  }

  // 2. 收到 websocket-server.js 廣播的 LED 控制指令
  // 格式: {"type":"led_command_sent","payload":{"GPIO23":"on"}}
  // 或直接控制格式: {"type":"led_control","payload":{"GPIO23":"on"}}
  if (typeStr == "led_command_sent" || typeStr == "led_control") {
    JsonObject payloadObj = doc["payload"];
    if (!payloadObj.isNull()) {
      const char* cmd = nullptr;
      if (payloadObj.containsKey("GPIO8")) {
        cmd = payloadObj["GPIO8"];
      } else if (payloadObj.containsKey("GPIO10")) {
        cmd = payloadObj["GPIO10"];
      } else if (payloadObj.containsKey("GPIO23")) {
        cmd = payloadObj["GPIO23"];
      } else if (payloadObj.containsKey("state")) {
        cmd = payloadObj["state"];
      }

      if (cmd != nullptr) {
        String cmdStr = String(cmd);
        cmdStr.toLowerCase();
        setLedState(cmdStr == "on" || cmdStr == "1" || cmdStr == "true");
        sendLedStatus();
      }
    }
    return;
  }

  // 3. 收到搖晃事件廣播 (shake_event) 時自動點亮 LED
  if (typeStr == "shake_event") {
    Serial.println("📳 偵測到搖晃事件，開啟 LED！");
    setLedState(true);
    sendLedStatus();
    return;
  }
}

// ==================== 連接 WiFi ====================
void connectWiFi() {
  Serial.print("正在連接 WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);

#ifdef USE_STATIC_IP
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS)) {
    Serial.println("⚠️ 固定 IP 設定失敗，改用 DHCP");
  } else {
    Serial.print("✅ 固定 IP 設定成功: ");
    Serial.println(local_IP);
  }
#endif

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi 連接成功！");
    Serial.print("IP 位址: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n❌ WiFi 連接逾時，將持續重試...");
  }
}

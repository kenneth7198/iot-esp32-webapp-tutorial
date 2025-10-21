#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>  // 需要安裝 ArduinoJson 函式庫

// ==================== 模式選擇 ====================
// 選擇工作模式（取消註解其中一個）
// #define GROUP_MODE          // 群組模式：esp32-group1/sensor/touch （可分組多群組比賽用）
#define INDEPENDENT_MODE    // 獨立模式：esp32/device-01/touch（9人比賽用）

// ==================== WiFi 設定 ====================
const char* WIFI_SSID = "NetArt";
const char* WIFI_PASS = "1qaz2wsx";

// ==================== 設備設定 ====================
// 重要：每個 ESP32 必須設定不同的設備編號（01-09）
const char* DEVICE_NUMBER = "02";  //修改這裡！01, 02, 03, ... 09

#ifdef GROUP_MODE
// 群組模式設定
const char* GROUP_NAME = "group1";            // 群組名稱：group1...（這保留先不用改）
#endif

// ==================== 網路設定 ====================
// 固定 IP 設定（可選，建議設定以避免 DHCP 延遲）
#define USE_STATIC_IP       // 取消註解以使用固定 IP

#ifdef USE_STATIC_IP
IPAddress local_IP(192, 168, 100, 201);       // ESP32 的固定 IP (110, 120, 130... 根據自己組別修改)
IPAddress gateway(192, 168, 100, 1);          // 路由器閘道
IPAddress subnet(255, 255, 255, 0);           // 子網路遮罩
IPAddress primaryDNS(8, 8, 8, 8);             // DNS 伺服器
#endif

// ==================== MQTT 設定 ====================
const char* MQTT_HOST = "192.168.100.200";    // MQTT Broker IP
const int   MQTT_PORT = 1883;                 // MQTT Port

// ==================== 硬體設定 ====================
const int TOUCH_SENSOR_PIN = 12;              // 電容觸摸感測器接在 GPIO 12
const int TOUCH_THRESHOLD = 40;               // 觸摸靈敏度（使用內建觸摸時）

// ==================== 進階設定 ====================
const unsigned long SEND_INTERVAL = 100;      // 檢查間隔 (毫秒)
const unsigned long DEBOUNCE_TIME = 100;      // 防抖時間 (毫秒)

// ==================== 自動產生的變數 ====================
String DEVICE_ID;                             // 設備 ID
String TOPIC_TOUCH;                           // MQTT 主題
// ===============================================

WiFiClient espClient;
PubSubClient mqtt(espClient);
unsigned long lastSendTime = 0;
unsigned long lastPublishTime = 0;
int lastTouchState = HIGH;  // 記錄上次觸摸狀態

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n========================================");
  Serial.println("ESP32 觸摸感測器 MQTT 發送器 v2.0");
  Serial.println("========================================");
  
  // 根據模式動態產生設備 ID 和主題
#ifdef GROUP_MODE
  // 群組模式：esp32-group1-01 → esp32-group1/sensor/touch
  DEVICE_ID = String("esp32-") + GROUP_NAME + "-" + DEVICE_NUMBER;
  TOPIC_TOUCH = String("esp32-") + GROUP_NAME + "/sensor/touch";
  Serial.println("模式: 群組模式 (GROUP_MODE)");
  Serial.print("群組名稱: ");
  Serial.println(GROUP_NAME);
#endif

#ifdef INDEPENDENT_MODE
  // 獨立模式：esp32-device-01 → esp32/device-01/touch
  DEVICE_ID = String("esp32-device-") + DEVICE_NUMBER;
  TOPIC_TOUCH = String("esp32/device-") + DEVICE_NUMBER + "/touch";
  Serial.println("模式: 獨立模式 (INDEPENDENT_MODE)");
  Serial.println("用途: rhythm-game.html 9人比賽");
#endif

  Serial.print("設備編號: ");
  Serial.println(DEVICE_NUMBER);
  Serial.print("設備 ID: ");
  Serial.println(DEVICE_ID);
  Serial.print("發布主題: ");
  Serial.println(TOPIC_TOUCH);
  Serial.println("========================================");
  
  // 設定 GPIO 12 為數位輸入
  pinMode(TOUCH_SENSOR_PIN, INPUT_PULLUP);
  
  Serial.print("觸摸感測器腳位: GPIO ");
  Serial.println(TOUCH_SENSOR_PIN);
  Serial.println();
  
  // 測試讀取
  int testValue = digitalRead(TOUCH_SENSOR_PIN);
  Serial.print("初始狀態: ");
  Serial.println(testValue ? "未觸摸 (HIGH)" : "觸摸中 (LOW)");
  Serial.println();

  // 連接 WiFi
  connectWiFi();
  
  // 設定 MQTT
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  
  // 連接 MQTT Broker
  connectMQTT();
  
  Serial.println("初始化完成，開始監測觸摸感測器...\n");
}

void loop() {
  // 確保 WiFi 和 MQTT 保持連線
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }
  
  if (!mqtt.connected()) {
    connectMQTT();
  }
  
  mqtt.loop();  // 處理 MQTT 訊息
  
  // 每隔指定時間檢查觸摸感測器
  unsigned long now = millis();
  if (now - lastSendTime >= SEND_INTERVAL) {
    lastSendTime = now;
    sendTouchData();
  }
}

// 連接 WiFi
void connectWiFi() {
  Serial.print("正在連接 WiFi: ");
  Serial.println(WIFI_SSID);
  
  WiFi.mode(WIFI_STA);
  
#ifdef USE_STATIC_IP
  // 設定固定 IP（必須在 WiFi.begin 之前）
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS)) {
    Serial.println("固定 IP 設定失敗！");
  } else {
    Serial.println("固定 IP 設定成功");
  }
#else
  Serial.println("使用 DHCP 自動取得 IP");
#endif
  
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n WiFi 連接成功！");
    Serial.print("IP 位址: ");
    Serial.println(WiFi.localIP());
#ifdef USE_STATIC_IP
    Serial.print("閘道: ");
    Serial.println(gateway);
#endif
    Serial.print("訊號強度 (RSSI): ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  } else {
    Serial.println("\n WiFi 連接失敗！");
    Serial.println("請檢查 SSID 和密碼");
  }
}

// 連接 MQTT Broker
void connectMQTT() {
  Serial.print("正在連接 MQTT Broker (");
  Serial.print(MQTT_HOST);
  Serial.print(":");
  Serial.print(MQTT_PORT);
  Serial.println(")...");
  
  int attempts = 0;
  while (!mqtt.connected() && attempts < 5) {
    Serial.print(".");
    
    if (mqtt.connect(DEVICE_ID.c_str())) {
      Serial.println("\n MQTT 連接成功！");
      Serial.print("設備 ID: ");
      Serial.println(DEVICE_ID);
      Serial.print("發布主題: ");
      Serial.println(TOPIC_TOUCH);
      Serial.println("========================================\n");
      Serial.println("系統就緒，開始監測觸摸感測器...\n");
      break;
    } else {
      Serial.print("\n MQTT 連接失敗，錯誤代碼: ");
      Serial.println(mqtt.state());
      Serial.println("5 秒後重試...");
      delay(5000);
      attempts++;
    }
  }
  
  if (!mqtt.connected()) {
    Serial.println(" 無法連接到 MQTT Broker，請檢查網路設定");
  }
}

// 讀取並發送觸摸感測器數據（JSON 格式）
void sendTouchData() {
  // 使用 digitalRead 讀取 GPIO 12（數位輸入）
  int touchState = digitalRead(TOUCH_SENSOR_PIN);
  
  // 只在狀態改變時發送（減少不必要的訊息）
  if (touchState != lastTouchState) {
    // 防抖處理
    unsigned long currentTime = millis();
    if (currentTime - lastPublishTime < DEBOUNCE_TIME) {
      return;  // 忽略過快的觸發
    }
    
    lastTouchState = touchState;
    lastPublishTime = currentTime;
    
    // 準備 JSON 訊息
    int touchValue;
    String status;
    String emoji;
    
    if (touchState == LOW) {
      // 觸摸感測器被觸發（觸摸模組輸出 LOW 相當於被觸發）
      touchValue = 1;  // 觸摸
      status = "觸摸";
    } else {
      // 未觸摸
      touchValue = 0;  // 釋放
      status = "釋放";
    }
    
    // 使用 ArduinoJson 建立 JSON 格式
    StaticJsonDocument<200> doc;
    doc["touch"] = touchValue;
    doc["device"] = DEVICE_ID;
    doc["timestamp"] = millis();
    
    // 序列化 JSON
    String jsonMessage;
    serializeJson(doc, jsonMessage);
    
    // 發布到 MQTT Broker
    bool success = mqtt.publish(TOPIC_TOUCH.c_str(), jsonMessage.c_str());
    
    // 在序列監控器顯示
    if (success) {
      Serial.print("[");
      Serial.print(TOPIC_TOUCH);
      Serial.print("] ");
      Serial.print(jsonMessage);
      Serial.print(" ");
      Serial.println(status);
    } else {
      Serial.println("發送失敗");
    }
  }
}
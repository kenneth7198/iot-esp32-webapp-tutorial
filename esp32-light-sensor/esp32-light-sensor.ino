// ==================== ESP32 光敏電阻感應器（萬聖節鬼屋版）====================
// 功能：讀取光敏電阻數值（手電筒照亮），並發布到 MQTT broker
// 用途：用手電筒照亮光敏電阻，觸發小精靈顯示在鬼屋迷宮中

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// ==================== 設定區 ====================
// WiFi 設定
const char* WIFI_SSID = "NetArt";
const char* WIFI_PASS = "1qaz2wsx";

// 固定 IP 設定（必須設定）
IPAddress local_IP(192, 168, 100, 201);       // ESP32 的固定 IP（device-01 使用 .201，device-02 使用 .202，依此類推）
IPAddress gateway(192, 168, 100, 1);          // 路由器閘道
IPAddress subnet(255, 255, 255, 0);           // 子網路遮罩
IPAddress primaryDNS(8, 8, 8, 8);             // DNS 伺服器

// MQTT Broker 設定
const char* MQTT_HOST = "192.168.100.200";    // MQTT Broker IP（伺服器 IP）
const int   MQTT_PORT = 1883;                 // MQTT Port
const char* DEVICE_ID = "esp32-device-01";    // 設備 ID（device-01 到 device-09）

// 硬體設定
const int LIGHT_SENSOR_PIN = 34;              // 光敏電阻接在 GPIO 34 (ADC1)

// 光線閾值設定
const int LIGHT_THRESHOLD = 3000;             // 光線閾值（超過此值表示手電筒照亮，0-4095）
const int DARK_THRESHOLD = 1000;              // 黑暗閾值（低於此值表示黑暗，用於消抖）

// 發送間隔
const unsigned long SEND_INTERVAL = 100;      // 每 0.1 秒發送一次（毫秒）

// MQTT 主題
String topicLight;                            // 動態生成的主題，例如：esp32/device-01/light
// ===============================================

WiFiClient espClient;
PubSubClient mqtt(espClient);
unsigned long lastSendTime = 0;
bool isActive = false;  // 當前激活狀態

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== ESP32 光敏電阻感應器（萬聖節鬼屋版）===");
  Serial.print("設備 ID: ");
  Serial.println(DEVICE_ID);
  
  // 動態生成 MQTT 主題
  topicLight = "esp32/" + String(DEVICE_ID) + "/light";
  
  // 設定 ADC 解析度和衰減
  analogReadResolution(12);           // 12-bit 解析度 (0-4095)
  analogSetAttenuation(ADC_11db);     // 設定衰減，讀取 0-3.3V 全範圍
  
  // 連接 WiFi
  connectWiFi();
  
  // 設定 MQTT
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  
  // 連接 MQTT Broker
  connectMQTT();
  
  Serial.println("初始化完成，開始監測光線...");
  Serial.print("光線閾值: ");
  Serial.println(LIGHT_THRESHOLD);
  Serial.print("黑暗閾值: ");
  Serial.println(DARK_THRESHOLD);
  Serial.println("👻 用手電筒照亮光敏電阻來喚醒你的小精靈！\n");
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
  
  // 每隔指定時間發送光敏電阻數據
  unsigned long now = millis();
  if (now - lastSendTime >= SEND_INTERVAL) {
    lastSendTime = now;
    sendLightData();
  }
}

// 連接 WiFi（使用固定 IP）
void connectWiFi() {
  Serial.print("正在連接 WiFi: ");
  Serial.println(WIFI_SSID);
  
  WiFi.mode(WIFI_STA);
  
  // 設定固定 IP（必須在 WiFi.begin 之前）
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS)) {
    Serial.println("固定 IP 設定失敗！");
  } else {
    Serial.println("固定 IP 設定成功");
  }
  
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWiFi 連接成功！");
  Serial.print("IP 位址: ");
  Serial.println(WiFi.localIP());
  Serial.print("閘道: ");
  Serial.println(gateway);
  Serial.print("訊號強度 (RSSI): ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
}

// 連接 MQTT Broker
void connectMQTT() {
  Serial.print("正在連接 MQTT Broker...");
  
  while (!mqtt.connected()) {
    Serial.print(".");
    
    if (mqtt.connect(DEVICE_ID)) {
      Serial.println("\nMQTT 連接成功！");
      Serial.print("設備 ID: ");
      Serial.println(DEVICE_ID);
      Serial.print("發布主題: ");
      Serial.println(topicLight);
    } else {
      Serial.print("\nMQTT 連接失敗，錯誤代碼: ");
      Serial.println(mqtt.state());
      Serial.println("5 秒後重試...");
      delay(5000);
    }
  }
}

// 讀取並發送光敏電阻數據
void sendLightData() {
  // 讀取光敏電阻 ADC 值 (0-4095)
  int lightValue = analogRead(LIGHT_SENSOR_PIN);
  
  // 判斷是否觸發（使用遲滯比較避免抖動）
  bool newActive;
  if (lightValue > LIGHT_THRESHOLD) {
    newActive = true;  // 光線強，激活
  } else if (lightValue < DARK_THRESHOLD) {
    newActive = false;  // 光線弱，停用
  } else {
    newActive = isActive;  // 在中間區域保持當前狀態
  }
  
  // 檢查狀態變化
  bool stateChanged = (newActive != isActive);
  isActive = newActive;
  
  // 建立 JSON 格式資料
  StaticJsonDocument<128> doc;
  doc["device"] = DEVICE_ID;
  doc["light"] = lightValue;
  doc["active"] = isActive;
  doc["timestamp"] = millis();
  
  // 序列化為字串
  String message;
  serializeJson(doc, message);
  
  // 發布到 MQTT Broker
  bool success = mqtt.publish(topicLight.c_str(), message.c_str());
  
  // 在序列監控器顯示（只在狀態變化時顯示詳細訊息）
  if (stateChanged) {
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━");
    if (isActive) {
      Serial.println("小精靈甦醒了！光線充足！");
    } else {
      Serial.println("小精靈隱藏了！光線不足！");
    }
    Serial.print("光線值: ");
    Serial.println(lightValue);
    Serial.print("狀態: ");
    Serial.println(isActive ? "激活" : "隱藏");
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━");
  }
  
  // 持續顯示當前數值（較簡潔）
  if (success) {
    Serial.print("LightValue:");
    Serial.print(lightValue);
    Serial.print(" | ");
    Serial.println(isActive ? "激活" : "隱藏");
  } else {
    Serial.println("發送失敗");
  }
}

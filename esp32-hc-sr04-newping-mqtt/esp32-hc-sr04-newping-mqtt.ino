/*
 * ESP32 HC-SR04 with NewPing 程式庫 + WiFi + MQTT
 * 更穩定的測量版本
 * 
 * 需要安裝的程式庫：
 * 1. NewPing (Arduino IDE → 管理程式庫)
 * 2. PubSubClient
 * 3. ArduinoJson
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <NewPing.h>

// ==================== WiFi 設定 ====================
const char* WIFI_SSID = "NetArt";
const char* WIFI_PASS = "1qaz2wsx";

// ==================== 設備設定 ====================
const char* DEVICE_NUMBER = "01";

// ==================== MQTT 設定 ====================
const char* MQTT_HOST = "192.168.100.200";
const int   MQTT_PORT = 1883;

// ==================== 硬體設定 ====================
// 嘗試多組腳位，自動選擇能用的
#define TRIG_PIN  18        // 或改成 32
#define ECHO_PIN  19        // 或改成 33
#define MAX_DISTANCE 400    // 最大測量距離 (cm)
#define MIN_DISTANCE 2      // 最小測量距離 (cm)

// ==================== 進階設定 ====================
const unsigned long SEND_INTERVAL = 100;      // 發送間隔 (毫秒)
const int MEASURE_SAMPLES = 5;                // 每次測量取樣次數

// ==================== 固定 IP ====================
IPAddress local_IP(192, 168, 100, 211);
IPAddress gateway(192, 168, 100, 1);
IPAddress subnet(255, 255, 255, 0);

// 建立物件
NewPing sonar(TRIG_PIN, ECHO_PIN, MAX_DISTANCE);
WiFiClient espClient;
PubSubClient mqtt(espClient);

String DEVICE_ID;
String TOPIC_DISTANCE;
unsigned long lastSendTime = 0;

// 統計變數
unsigned long totalMeasures = 0;
unsigned long failedMeasures = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n════════════════════════════════════════");
  Serial.println("ESP32 HC-SR04 (NewPing) MQTT 發送器");
  Serial.println("════════════════════════════════════════");
  
  // 設定設備 ID 和主題
  DEVICE_ID = String("esp32-device-") + DEVICE_NUMBER;
  TOPIC_DISTANCE = String("esp32/device-") + DEVICE_NUMBER + "/distance";
  
  Serial.print("設備 ID: ");
  Serial.println(DEVICE_ID);
  Serial.print("發布主題: ");
  Serial.println(TOPIC_DISTANCE);
  Serial.print("使用腳位: GPIO ");
  Serial.print(TRIG_PIN);
  Serial.print(" (Trig) / GPIO ");
  Serial.print(ECHO_PIN);
  Serial.println(" (Echo)");
  Serial.println("使用 NewPing 程式庫 ✅");
  Serial.println();
  
  // 測試感測器
  Serial.println("測試感測器連接...");
  for (int i = 0; i < 3; i++) {
    unsigned int testDist = sonar.ping_cm();
    Serial.print("  測試 #");
    Serial.print(i + 1);
    Serial.print(": ");
    if (testDist == 0) {
      Serial.println("❌ 無讀取");
    } else {
      Serial.print(testDist);
      Serial.println(" cm ✅");
    }
    delay(100);
  }
  Serial.println();
  
  // 連接 WiFi
  connectWiFi();
  
  // 設定 MQTT
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  connectMQTT();
  
  Serial.println("初始化完成，開始測量...\n");
}

void loop() {
  // 確保連線
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }
  
  if (!mqtt.connected()) {
    connectMQTT();
  }
  
  mqtt.loop();
  
  // 定時發送數據
  unsigned long now = millis();
  if (now - lastSendTime >= SEND_INTERVAL) {
    lastSendTime = now;
    sendDistanceData();
  }
}

void connectWiFi() {
  Serial.print("連接 WiFi: ");
  Serial.print(WIFI_SSID);
  Serial.print("...");
  
  WiFi.config(local_IP, gateway, subnet);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(" 成功！");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  }
}

void connectMQTT() {
  Serial.print("連接 MQTT Broker...");
  
  int attempts = 0;
  while (!mqtt.connected() && attempts < 5) {
    if (mqtt.connect(DEVICE_ID.c_str())) {
      Serial.println(" 成功！");
      break;
    } else {
      Serial.print(".");
      delay(1000);
      attempts++;
    }
  }
}

void sendDistanceData() {
  // 使用 NewPing 的中位數模式（取多次測量的中位數，更準確）
  unsigned int distance = sonar.ping_median(MEASURE_SAMPLES);
  
  // 轉換為 cm（NewPing 的 ping_median 返回微秒）
  distance = sonar.convert_cm(distance);
  
  totalMeasures++;
  
  // 如果讀取失敗，不發送
  if (distance == 0) {
    failedMeasures++;
    return;
  }
  
  // 驗證範圍
  bool valid = (distance >= MIN_DISTANCE && distance <= MAX_DISTANCE);
  
  // 準備 JSON
  StaticJsonDocument<256> doc;
  doc["device"] = DEVICE_ID;
  doc["distance"] = distance;
  doc["valid"] = valid;
  doc["unit"] = "cm";
  doc["timestamp"] = millis();
  
  String jsonMessage;
  serializeJson(doc, jsonMessage);
  
  // 發布
  mqtt.publish(TOPIC_DISTANCE.c_str(), jsonMessage.c_str());
  
  // 每 50 次顯示統計
  if (totalMeasures % 50 == 0) {
    float failRate = (failedMeasures * 100.0) / totalMeasures;
    Serial.print("📊 統計 - 總測量: ");
    Serial.print(totalMeasures);
    Serial.print(" | 失敗: ");
    Serial.print(failedMeasures);
    Serial.print(" | 失敗率: ");
    Serial.print(failRate, 1);
    Serial.print("% | 最新距離: ");
    Serial.print(distance);
    Serial.println(" cm");
  }
}

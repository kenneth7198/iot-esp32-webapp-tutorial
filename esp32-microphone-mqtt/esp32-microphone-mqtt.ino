/*
 * ESP32 麥克風控制 LED + MQTT
 * 連接到 192.168.100.200 的 MQTT Server
 * 
 * 硬體連接:
 * - 麥克風模組 (類比輸出) -> GPIO 34 (ADC1_CH6)
 * - LED -> GPIO 23 (可 PWM 輸出)
 * 
 * MQTT Topics:
 * - 發布: esp32/microphone/level (音量數值)
 * - 發布: esp32/microphone/status (ON/OFF 狀態)
 */

#include <WiFi.h>
#include <PubSubClient.h>

// ==================== WiFi 設定 ====================
const char* ssid = "XXXXX";           // 修改為你的 WiFi 名稱
const char* password = "XXXXXX";   // 修改為你的 WiFi 密碼

// ==================== 固定 IP ====================
IPAddress local_IP(192, 168, 100, 211);
IPAddress gateway(192, 168, 100, 1);
IPAddress subnet(255, 255, 255, 0);

// ==================== MQTT 設定 ====================
const char* mqtt_server = "192.168.100.200";
const int mqtt_port = 1883;
const char* mqtt_client_id = "esp32-microphone-01";

// MQTT Topics
const char* TOPIC_MIC_LEVEL = "esp32/microphone/level";
const char* TOPIC_MIC_STATUS = "esp32/microphone/status";

// ==================== 硬體腳位 ====================
const int MIC_PIN = 34;        // 麥克風類比輸入 (ADC1_CH6)
const int LED_PIN = 23;        // LED 輸出腳位

// ==================== PWM 設定 ====================
const int PWM_CHANNEL = 0;     // PWM 通道
const int PWM_FREQ = 5000;     // PWM 頻率 5kHz
const int PWM_RESOLUTION = 8;  // 8-bit 解析度 (0-255)

// ==================== 全域變數 ====================
WiFiClient espClient;
PubSubClient mqttClient(espClient);

int micValue = 0;              // 麥克風讀值
int mappedValue = 0;           // 映射後的值 (0-255)
const int THRESHOLD = 2500;    // 觸發閾值 (ESP32 ADC 是 12-bit: 0-4095)

unsigned long lastPublishTime = 0;
const unsigned long PUBLISH_INTERVAL = 100;  // 每 100ms 發布一次

// ==================== WiFi 連接 ====================
void connectWiFi() {
  Serial.println();
  Serial.print("連接 WiFi: ");
  Serial.println(ssid);
  
  // 設定固定 IP
  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("⚠️ 固定 IP 設定失敗");
  } else {
    Serial.println("✅ 固定 IP 已設定: 192.168.100.211");
  }
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("✅ WiFi 已連接");
    Serial.print("IP 位址: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println("❌ WiFi 連接失敗");
  }
}

// ==================== MQTT 連接 ====================
void connectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("連接 MQTT Server...");
    
    if (mqttClient.connect(mqtt_client_id)) {
      Serial.println(" ✅ 已連接");
      
      // 發送上線訊息
      String statusMsg = "{\"device\":\"" + String(mqtt_client_id) + "\",\"status\":\"online\"}";
      mqttClient.publish(TOPIC_MIC_STATUS, statusMsg.c_str());
      
    } else {
      Serial.print(" ❌ 失敗, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" 5秒後重試...");
      delay(5000);
    }
  }
}

// ==================== 發布麥克風數據 ====================
void publishMicData() {
  if (!mqttClient.connected()) {
    return;
  }
  
  // 建立 JSON 格式數據
  String payload = "{";
  payload += "\"device\":\"" + String(mqtt_client_id) + "\",";
  payload += "\"micValue\":" + String(micValue) + ",";
  payload += "\"mappedValue\":" + String(mappedValue) + ",";
  payload += "\"ledBrightness\":" + String(mappedValue) + ",";
  payload += "\"isLoud\":" + String(micValue > THRESHOLD ? "true" : "false") + ",";
  payload += "\"timestamp\":" + String(millis());
  payload += "}";
  
  // 發布到 MQTT
  mqttClient.publish(TOPIC_MIC_LEVEL, payload.c_str());
  
  // 如果超過閾值，發布狀態
  if (micValue > THRESHOLD) {
    String statusMsg = "{\"device\":\"" + String(mqtt_client_id) + "\",\"status\":\"LOUD\"}";
    mqttClient.publish(TOPIC_MIC_STATUS, statusMsg.c_str());
  }
}

// ==================== Setup ====================
void setup() {
  Serial.begin(115200);
  Serial.println("\n\n");
  Serial.println("╔══════════════════════════════════════╗");
  Serial.println("║   ESP32 麥克風 + LED + MQTT 控制     ║");
  Serial.println("╚══════════════════════════════════════╝");
  
  // 初始化 LED PWM (使用新版 API)
  ledcAttach(LED_PIN, PWM_FREQ, PWM_RESOLUTION);
  ledcWrite(LED_PIN, 0);  // 初始關閉
  
  Serial.println("✅ LED PWM 初始化完成");
  
  // 連接 WiFi
  connectWiFi();
  
  // 設定 MQTT
  mqttClient.setServer(mqtt_server, mqtt_port);
  
  // 測試 LED
  Serial.println("🔦 測試 LED...");
  for (int i = 0; i <= 255; i += 5) {
    ledcWrite(LED_PIN, i);
    delay(10);
  }
  for (int i = 255; i >= 0; i -= 5) {
    ledcWrite(LED_PIN, i);
    delay(10);
  }
  Serial.println("✅ LED 測試完成");
  
  Serial.println("\n🎤 開始監測麥克風...\n");
}

// ==================== Loop ====================
void loop() {
  // 檢查 WiFi 連接
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("⚠️ WiFi 斷線，重新連接...");
    connectWiFi();
  }
  
  // 檢查 MQTT 連接
  if (!mqttClient.connected()) {
    connectMQTT();
  }
  mqttClient.loop();
  
  // 讀取麥克風數值 (ESP32 ADC 是 12-bit: 0-4095)
  micValue = analogRead(MIC_PIN);
  
  // 映射到 0-255 (LED PWM 範圍)
  mappedValue = map(micValue, 0, 4095, 0, 255);
  
  // 輸出到 LED
  ledcWrite(LED_PIN, mappedValue);
  
  // 序列埠輸出
  Serial.print("🎤 麥克風: ");
  Serial.print(micValue);
  Serial.print(" | LED: ");
  Serial.print(mappedValue);
  
  // 檢查是否超過閾值
  if (micValue > THRESHOLD) {
    Serial.print(" | 🔊 大聲！");
    ledcWrite(LED_PIN, 255);  // LED 全亮
  }
  Serial.println();
  
  // 定期發布到 MQTT
  unsigned long currentTime = millis();
  if (currentTime - lastPublishTime >= PUBLISH_INTERVAL) {
    publishMicData();
    lastPublishTime = currentTime;
  }
  
  delay(20);  // 與原程式相同的延遲
}

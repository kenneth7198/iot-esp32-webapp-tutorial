// ==================== ESP32 按鈕觸發視覺藝術 ====================
// 功能：讀取按鈕狀態並發布到 MQTT broker
// 用途：按下實體按鈕，觸發網頁上的生成藝術效果

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// ==================== 設定區 ====================
// WiFi 設定
const char* WIFI_SSID = "NetArt";
const char* WIFI_PASS = "1qaz2wsx";

// 固定 IP 設定（可選，建議設定以確保穩定連線）
IPAddress local_IP(192, 168, 100, 201);       // ESP32 的固定 IP（根據自己的ESP32作調整）
IPAddress gateway(192, 168, 100, 1);          // 路由器閘道
IPAddress subnet(255, 255, 255, 0);           // 子網路遮罩
IPAddress primaryDNS(8, 8, 8, 8);             // DNS 伺服器

// MQTT Broker 設定
const char* MQTT_HOST = "192.168.100.200";    // MQTT Broker IP（伺服器 IP）
const int   MQTT_PORT = 1883;                 // MQTT Port
const char* DEVICE_ID = "esp32-art-01";       // 設備 ID

// 硬體設定
const int BUTTON_PIN = 22;                    // 按鈕接在 GPIO 22
const int LED_PIN = 23;                       // LED 接在 GPIO 23

// 防彈跳設定
const unsigned long DEBOUNCE_DELAY = 50;      // 防彈跳延遲（毫秒）

// MQTT 主題
String topicButton;                           // 動態生成的主題，例如：art/button/esp32-art-01
// ===============================================

// 變數
int lastButtonState = LOW;
int buttonState = LOW;
unsigned long lastDebounceTime = 0;

WiFiClient espClient;
PubSubClient mqtt(espClient);

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== ESP32 按鈕觸發視覺藝術 ===");
  Serial.print("設備 ID: ");
  Serial.println(DEVICE_ID);
  
  // 動態生成 MQTT 主題
  topicButton = "art/button/" + String(DEVICE_ID);
  
  // 設定 GPIO
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);
  
  // 連接 WiFi
  connectWiFi();
  
  // 設定 MQTT
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  
  // 連接 MQTT Broker
  connectMQTT();
  
  Serial.println("初始化完成，等待按鈕觸發...");
  Serial.println("按下按鈕以觸發藝術效果！\n");
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
      Serial.println(topicButton);
    } else {
      Serial.print("\nMQTT 連接失敗，錯誤代碼: ");
      Serial.println(mqtt.state());
      Serial.println("5 秒後重試...");
      delay(5000);
    }
  }
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
  
  // 讀取按鈕狀態（帶防彈跳）
  int reading = digitalRead(BUTTON_PIN);
  
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  
  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (reading != buttonState) {
      buttonState = reading;
      
      // 按鈕狀態改變時發送 MQTT 訊息
      if (buttonState == HIGH) {
        Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━");
        Serial.println("按鈕按下 - 觸發藝術效果！");
        digitalWrite(LED_PIN, HIGH);
        
        // 建立 JSON 格式資料
        StaticJsonDocument<128> doc;
        doc["device"] = DEVICE_ID;
        doc["pressed"] = true;
        doc["timestamp"] = millis();
        
        // 序列化為字串
        String message;
        serializeJson(doc, message);
        
        // 發布到 MQTT Broker
        bool success = mqtt.publish(topicButton.c_str(), message.c_str());
        
        if (success) {
          Serial.print("發送成功: ");
          Serial.println(message);
        } else {
          Serial.println("發送失敗");
        }
        Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━");
        
      } else {
        Serial.println("按鈕釋放");
        digitalWrite(LED_PIN, LOW);
        
        // 建立 JSON 格式資料
        StaticJsonDocument<128> doc;
        doc["device"] = DEVICE_ID;
        doc["pressed"] = false;
        doc["timestamp"] = millis();
        
        // 序列化為字串
        String message;
        serializeJson(doc, message);
        
        // 發布到 MQTT Broker
        mqtt.publish(topicButton.c_str(), message.c_str());
      }
    }
  }
  
  lastButtonState = reading;
  delay(10);
}

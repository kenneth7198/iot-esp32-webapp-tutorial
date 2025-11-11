#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>  // 需要安裝 ArduinoJson 函式庫
#include <NewPing.h>      // 需要安裝 NewPing 函式庫

// ==================== 模式選擇 ====================
// 選擇工作模式（取消註解其中一個）
// #define GROUP_MODE          // 群組模式：esp32-group1/sensor/distance
#define INDEPENDENT_MODE    // 獨立模式：esp32/device-01/distance

// ==================== WiFi 設定 ====================
const char* WIFI_SSID = "NetArt";
const char* WIFI_PASS = "1qaz2wsx";

// ==================== 設備設定 ====================
// 重要：每個 ESP32 必須設定不同的設備編號（01-09）
const char* DEVICE_NUMBER = "01";  // 修改這裡！01, 02, 03, ... 09

#ifdef GROUP_MODE
// 群組模式設定
const char* GROUP_NAME = "group1";            // 群組名稱：group1...
#endif

// ==================== 網路設定 ====================
// 固定 IP 設定（可選，建議設定以避免 DHCP 延遲）
#define USE_STATIC_IP       // 取消註解以使用固定 IP

#ifdef USE_STATIC_IP
IPAddress local_IP(192, 168, 100, 211);       // ESP32 的固定 IP (211, 212, 213... 根據設備修改)
IPAddress gateway(192, 168, 100, 1);          // 路由器閘道
IPAddress subnet(255, 255, 255, 0);           // 子網路遮罩
IPAddress primaryDNS(8, 8, 8, 8);             // DNS 伺服器
#endif

// ==================== MQTT 設定 ====================
const char* MQTT_HOST = "192.168.100.200";    // MQTT Broker IP
const int   MQTT_PORT = 1883;                 // MQTT Port

// ==================== 硬體設定 ====================
// GPIO 腳位（GPIO 12/14）✅ NewPing 測試通過
const int TRIG_PIN = 12;                      // HC-SR04 Trig 腳位
const int ECHO_PIN = 14;                      // HC-SR04 Echo 腳位
const int MAX_DISTANCE = 400;                 // 最大測量距離 (cm)
const int MIN_DISTANCE = 2;                   // 最小測量距離 (cm)

// ==================== 進階設定 ====================
const unsigned long SEND_INTERVAL = 100;      // 發送間隔 (毫秒)
const int MEASURE_SAMPLES = 1;                // 改為單次測量（更即時）

// 建立 NewPing 物件
NewPing sonar(TRIG_PIN, ECHO_PIN, MAX_DISTANCE);

// ==================== 自動產生的變數 ====================
String DEVICE_ID;                             // 設備 ID
String TOPIC_DISTANCE;                        // MQTT 主題
// ===============================================

WiFiClient espClient;
PubSubClient mqtt(espClient);
unsigned long lastSendTime = 0;

// 統計變數
unsigned long totalMeasures = 0;
unsigned long failedMeasures = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n========================================");
  Serial.println("ESP32 HC-SR04 超音波感測器 MQTT 發送器");
  Serial.println("========================================");
  
  // 根據模式動態產生設備 ID 和主題
#ifdef GROUP_MODE
  // 群組模式：esp32-group1-01 → esp32-group1/sensor/distance
  DEVICE_ID = String("esp32-") + GROUP_NAME + "-" + DEVICE_NUMBER;
  TOPIC_DISTANCE = String("esp32-") + GROUP_NAME + "/sensor/distance";
  Serial.println("模式: 群組模式 (GROUP_MODE)");
  Serial.print("群組名稱: ");
  Serial.println(GROUP_NAME);
#endif

#ifdef INDEPENDENT_MODE
  // 獨立模式：esp32-device-01 → esp32/device-01/distance
  DEVICE_ID = String("esp32-device-") + DEVICE_NUMBER;
  TOPIC_DISTANCE = String("esp32/device-") + DEVICE_NUMBER + "/distance";
  Serial.println("模式: 獨立模式 (INDEPENDENT_MODE)");
#endif

  Serial.print("設備編號: ");
  Serial.println(DEVICE_NUMBER);
  Serial.print("設備 ID: ");
  Serial.println(DEVICE_ID);
  Serial.print("發布主題: ");
  Serial.println(TOPIC_DISTANCE);
  Serial.println("========================================");
  
  Serial.print("Trig 腳位: GPIO ");
  Serial.println(TRIG_PIN);
  Serial.print("Echo 腳位: GPIO ");
  Serial.println(ECHO_PIN);
  Serial.println("✅ 使用 NewPing 程式庫");
  Serial.print("測量模式: 中位數 (");
  Serial.print(MEASURE_SAMPLES);
  Serial.println(" 次取樣)");
  
  // 測試感測器
  Serial.println("\n測試感測器連接...");
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
  
  Serial.print("\n測量範圍: ");
  Serial.print(MIN_DISTANCE);
  Serial.print(" - ");
  Serial.print(MAX_DISTANCE);
  Serial.println(" cm");
  Serial.println();

  // 連接 WiFi
  connectWiFi();
  
  // 設定 MQTT
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  
  // 連接 MQTT Broker
  connectMQTT();
  
  Serial.println("初始化完成，開始測量距離...\n");
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
  
  // 每隔指定時間發送距離數據
  unsigned long now = millis();
  if (now - lastSendTime >= SEND_INTERVAL) {
    lastSendTime = now;
    sendDistanceData();
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
    Serial.println("\nWiFi 連接成功！");
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
    Serial.println("\nWiFi 連接失敗！");
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
  Serial.print("設備 ID: ");
  Serial.println(DEVICE_ID);
  
  int attempts = 0;
  while (!mqtt.connected() && attempts < 5) {
    Serial.print("嘗試連接 #");
    Serial.print(attempts + 1);
    Serial.print("...");
    
    // 使用簡單的連接方式（只需要設備 ID）
    if (mqtt.connect(DEVICE_ID.c_str())) {
      Serial.println(" 成功！");
      Serial.print("發布主題: ");
      Serial.println(TOPIC_DISTANCE);
      Serial.println("========================================\n");
      Serial.println("系統就緒，開始測量距離...\n");
      break;
    } else {
      int state = mqtt.state();
      Serial.print(" 失敗！\n錯誤代碼: ");
      Serial.print(state);
      Serial.print(" - ");
      
      // 詳細錯誤訊息
      switch(state) {
        case -4:
          Serial.println("MQTT_CONNECTION_TIMEOUT - 連接超時");
          break;
        case -3:
          Serial.println("MQTT_CONNECTION_LOST - 連接中斷");
          break;
        case -2:
          Serial.println("MQTT_CONNECT_FAILED - 無法連接到伺服器");
          Serial.println("請檢查：");
          Serial.println("  1. Mosquitto 是否正在運行？");
          Serial.println("  2. Mosquitto 是否監聽 0.0.0.0:1883？");
          Serial.println("  3. 防火牆是否阻擋 1883 端口？");
          break;
        case -1:
          Serial.println("MQTT_DISCONNECTED - 已斷線");
          break;
        case 1:
          Serial.println("MQTT_CONNECT_BAD_PROTOCOL - 協議版本錯誤");
          break;
        case 2:
          Serial.println("MQTT_CONNECT_BAD_CLIENT_ID - 客戶端 ID 被拒絕");
          break;
        case 3:
          Serial.println("MQTT_CONNECT_UNAVAILABLE - 伺服器不可用");
          break;
        case 4:
          Serial.println("MQTT_CONNECT_BAD_CREDENTIALS - 憑證錯誤");
          break;
        case 5:
          Serial.println("MQTT_CONNECT_UNAUTHORIZED - 未授權");
          break;
        default:
          Serial.println("未知錯誤");
      }
      
      if (attempts < 4) {
        Serial.println("3 秒後重試...\n");
        delay(3000);
      }
      attempts++;
    }
  }
  
  if (!mqtt.connected()) {
    Serial.println("\n❌ 無法連接到 MQTT Broker");
    Serial.println("請確認：");
    Serial.println("  1. WiFi 已連接");
    Serial.println("  2. Mosquitto 正在運行");
    Serial.println("  3. mosquitto.conf 設定為 'listener 1883 0.0.0.0'");
    Serial.println("  4. 執行: lsof -i :1883 檢查端口");
  }
}

// 測量距離（使用 NewPing 程式庫 - 中位數模式）
long measureDistance() {
  // 使用 NewPing 的中位數模式（取多次測量的中位數，過濾異常值）
  unsigned int uS = sonar.ping_median(MEASURE_SAMPLES);
  
  // 轉換為 cm
  unsigned int distance = sonar.convert_cm(uS);
  
  // 統計
  totalMeasures++;
  if (distance == 0) {
    failedMeasures++;
  }
  
  // 每 50 次顯示統計
  if (totalMeasures % 50 == 0) {
    float failRate = (failedMeasures * 100.0) / totalMeasures;
    Serial.print("� 統計 #");
    Serial.print(totalMeasures);
    Serial.print(" - 失敗: ");
    Serial.print(failedMeasures);
    Serial.print(" (");
    Serial.print(failRate, 1);
    Serial.print("%)");
    
    if (distance > 0) {
      Serial.print(" | 最新距離: ");
      Serial.print(distance);
      Serial.println(" cm ✅");
    } else {
      Serial.println(" | 最新: 無讀取 ❌");
    }
  }
  
  return distance;
}

// 讀取並發送距離數據（JSON 格式）
void sendDistanceData() {
  // 測量距離（NewPing 已經做了中位數平滑處理）
  long distance = measureDistance();
  
  // 如果讀取失敗（distance = 0），不發送 MQTT 訊息
  if (distance == 0) {
    // 靜默失敗，不發送無效數據
    return;
  }
  
  // 驗證距離是否在有效範圍內
  bool valid = (distance >= MIN_DISTANCE && distance <= MAX_DISTANCE);
  
  // 準備 JSON 訊息
  StaticJsonDocument<256> doc;
  doc["device"] = DEVICE_ID;
  doc["distance"] = distance;
  doc["valid"] = valid;
  doc["unit"] = "cm";
  doc["timestamp"] = millis();
  
  // 序列化 JSON
  String jsonMessage;
  serializeJson(doc, jsonMessage);
  
  // 發布到 MQTT Broker
  bool success = mqtt.publish(TOPIC_DISTANCE.c_str(), jsonMessage.c_str());
  
  // 在序列監控器顯示
  if (success) {
    // 每 20 次顯示一次詳細資訊
    static int printCount = 0;
    printCount++;
    
    if (printCount % 20 == 0 || !valid) {
      Serial.print("[");
      Serial.print(TOPIC_DISTANCE);
      Serial.print("] ");
      
      if (valid) {
        Serial.print("✅ ");
        Serial.print(distance);
        Serial.print(" cm");
      } else {
        Serial.print("⚠️ Out of range - ");
        Serial.print(distance);
        Serial.print(" cm (duration 可能為 0)");
      }
      
      Serial.println();
    }
  } else {
    Serial.println("❌ 發送失敗");
  }
}

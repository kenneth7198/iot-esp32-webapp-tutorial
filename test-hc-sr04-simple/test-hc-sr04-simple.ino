/*
 * HC-SR04 簡單測試程式
 * 用於診斷 HC-SR04 超音波感測器是否正常工作
 * 不需要 WiFi 或 MQTT，只測試感測器本身
 */

// 腳位定義（使用診斷測試驗證過的 GPIO）
const int TRIG_PIN = 32;  // Trig 腳位（GPIO 32）✅ 已驗證
const int ECHO_PIN = 33;  // Echo 腳位（GPIO 33）✅ 已驗證

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== HC-SR04 簡單測試程式 ===");
  Serial.println("腳位配置:");
  Serial.println("  Trig: GPIO 32 ✅");
  Serial.println("  Echo: GPIO 33 ✅");
  Serial.println("=============================\n");
  
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  // 確保 Trig 腳位初始為 LOW
  digitalWrite(TRIG_PIN, LOW);
  delay(2000);
}

void loop() {
  // 發送 10us 觸發脈衝
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  // 測量回波時間
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);  // 30ms timeout
  
  // 詳細診斷資訊
  Serial.print("原始持續時間: ");
  Serial.print(duration);
  Serial.print(" us");
  
  if (duration == 0) {
    Serial.println(" ❌ 沒有收到回波信號！");
    Serial.println("可能原因:");
    Serial.println("  1. 感測器未正確供電 (需要 5V)");
    Serial.println("  2. Echo 腳位連接錯誤");
    Serial.println("  3. 感測器損壞");
    Serial.println("  4. 障礙物太近（<2cm）或太遠（>400cm）");
  } else {
    // 計算距離（公式: duration * 0.034 / 2）
    float distance = duration * 0.034 / 2;
    
    Serial.print(" | 距離: ");
    Serial.print(distance);
    Serial.print(" cm");
    
    if (distance < 2) {
      Serial.println(" ⚠️ 物體太近");
    } else if (distance > 400) {
      Serial.println(" ⚠️ 超出範圍");
    } else {
      Serial.println(" ✅ 正常");
    }
  }
  
  delay(500);  // 每 0.5 秒測量一次
}

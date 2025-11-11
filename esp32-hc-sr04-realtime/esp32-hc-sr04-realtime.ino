/*
 * ESP32 HC-SR04 即時測試
 * 每次測量都顯示，看數值是否真的會變化
 */

#include <NewPing.h>

const int TRIG_PIN = 18;
const int ECHO_PIN = 19;
const int MAX_DISTANCE = 400;

NewPing sonar(TRIG_PIN, ECHO_PIN, MAX_DISTANCE);

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n╔═══════════════════════════════════════╗");
  Serial.println("║  HC-SR04 即時測試                    ║");
  Serial.println("║  每次測量都顯示，觀察數值變化        ║");
  Serial.println("╚═══════════════════════════════════════╝");
  Serial.println("Trig: GPIO 18 | Echo: GPIO 19");
  Serial.println();
  Serial.println("請移動手掌，觀察數值是否改變...");
  Serial.println("─────────────────────────────────────────\n");
}

void loop() {
  // 單次測量
  unsigned int distance = sonar.ping_cm();
  
  // 獲取當前時間
  unsigned long now = millis();
  
  // 每次都顯示
  Serial.print("[");
  Serial.print(now / 1000);
  Serial.print("s] ");
  
  if (distance == 0) {
    Serial.println("❌ 無讀取（超出範圍或無障礙物）");
  } else {
    Serial.print("📏 ");
    Serial.print(distance);
    Serial.print(" cm");
    
    // 根據距離顯示提示
    if (distance < 10) {
      Serial.println(" 🔴 非常近！（撈魚距離）");
    } else if (distance < 30) {
      Serial.println(" 🟡 近");
    } else if (distance < 50) {
      Serial.println(" 🟢 中等");
    } else {
      Serial.println(" 🔵 遠");
    }
  }
  
  delay(200);  // 0.2 秒測量一次
}

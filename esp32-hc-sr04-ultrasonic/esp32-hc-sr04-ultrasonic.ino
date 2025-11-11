/*
 * ESP32 HC-SR04 測試 - 使用 Ultrasonic.h 程式庫
 * GPIO 4 (Trig) / GPIO 5 (Echo)
 */

#include <Ultrasonic.h>

// 建立 Ultrasonic 物件 (Trig, Echo)
Ultrasonic ultrasonic(12, 14);

void setup() {
  Serial.begin(115200);  
  delay(2000);
  
  Serial.println("\n╔═══════════════════════════════════════╗");
  Serial.println("║  HC-SR04 測試 - Ultrasonic.h         ║");
  Serial.println("╚═══════════════════════════════════════╝");
  Serial.println("Trig: GPIO 4");
  Serial.println("Echo: GPIO 5");
  Serial.println();
  Serial.println("請移動手掌，觀察數值變化...");
  Serial.println("─────────────────────────────────────────\n");
}

void loop() {
  // 讀取距離 (cm)
  long distance = ultrasonic.read();
  
  // 獲取時間
  unsigned long now = millis();
  
  // 顯示結果
  Serial.print("[");
  Serial.print(now / 1000);
  Serial.print("s] ");
  
  if (distance <= 0 || distance > 400) {
    Serial.println("❌ 超出範圍");
  } else {
    Serial.print("📏 ");
    Serial.print(distance);
    Serial.print(" cm");
    
    // 根據距離顯示提示
    if (distance < 10) {
      Serial.println(" 🔴 撈魚距離！");
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

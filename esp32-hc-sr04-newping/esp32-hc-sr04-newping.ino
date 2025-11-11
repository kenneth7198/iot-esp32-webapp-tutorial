/*
 * ESP32 HC-SR04 測試 - 使用 NewPing 程式庫
 * 更穩定、更可靠的版本
 * 
 * 安裝方法：
 * Arduino IDE → 工具 → 管理程式庫 → 搜尋 "NewPing" → 安裝
 */

#include <NewPing.h>

// 腳位定義（測試多組腳位）
#define TRIG_PIN_1  18
#define ECHO_PIN_1  19
#define TRIG_PIN_2  32
#define ECHO_PIN_2  33
#define MAX_DISTANCE 400  // 最大測量距離 (cm)

// 建立兩個感測器物件
NewPing sonar1(TRIG_PIN_1, ECHO_PIN_1, MAX_DISTANCE);
NewPing sonar2(TRIG_PIN_2, ECHO_PIN_2, MAX_DISTANCE);

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n╔═══════════════════════════════════════╗");
  Serial.println("║  HC-SR04 測試 - NewPing 程式庫      ║");
  Serial.println("╚═══════════════════════════════════════╝\n");
  Serial.println("測試兩組腳位：");
  Serial.println("  1. GPIO 18/19");
  Serial.println("  2. GPIO 32/33");
  Serial.println();
  delay(1000);
}

void loop() {
  Serial.println("─────────────────────────────────────");
  
  // 測試 GPIO 18/19
  Serial.print("📍 GPIO 18/19: ");
  unsigned int distance1 = sonar1.ping_cm();
  if (distance1 == 0) {
    Serial.println("❌ 無讀取");
  } else {
    Serial.print(distance1);
    Serial.println(" cm ✅");
  }
  
  delay(50);  // 兩次測量間隔
  
  // 測試 GPIO 32/33
  Serial.print("📍 GPIO 32/33: ");
  unsigned int distance2 = sonar2.ping_cm();
  if (distance2 == 0) {
    Serial.println("❌ 無讀取");
  } else {
    Serial.print(distance2);
    Serial.println(" cm ✅");
  }
  
  Serial.println();
  delay(500);
}

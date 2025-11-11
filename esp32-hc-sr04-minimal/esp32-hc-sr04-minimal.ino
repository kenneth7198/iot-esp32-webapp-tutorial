/*
 * ESP32 HC-SR04 最簡單測試 - 多腳位版本
 * 自動測試多組腳位，找出能工作的組合
 */

// 測試腳位組合
struct PinPair {
  int trig;
  int echo;
  const char* name;
};

PinPair pins[] = {
  {32, 33, "GPIO 32/33"},
  {18, 19, "GPIO 18/19"},
  {16, 17, "GPIO 16/17"},
  {27, 14, "GPIO 27/14"},
  {21, 22, "GPIO 21/22"}
};

int currentPin = 0;
const int numPins = sizeof(pins) / sizeof(pins[0]);

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n╔═══════════════════════════════╗");
  Serial.println("║  HC-SR04 自動腳位測試        ║");
  Serial.println("╚═══════════════════════════════╝\n");
  
  testCurrentPin();
}

void loop() {
  int trig = pins[currentPin].trig;
  int echo = pins[currentPin].echo;
  
  // 測量 3 次
  int successCount = 0;
  for (int i = 0; i < 3; i++) {
    digitalWrite(trig, LOW);
    delayMicroseconds(2);
    digitalWrite(trig, HIGH);
    delayMicroseconds(10);
    digitalWrite(trig, LOW);
    
    long duration = pulseIn(echo, HIGH, 30000);
    
    Serial.print("  測試 #");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(duration);
    Serial.print(" us");
    
    if (duration == 0) {
      Serial.println(" ❌");
    } else {
      float distance = duration * 0.034 / 2;
      Serial.print(" | ");
      Serial.print(distance);
      Serial.println(" cm ✅");
      successCount++;
    }
    
    delay(300);
  }
  
  // 評估結果
  Serial.print("\n結果: ");
  if (successCount == 3) {
    Serial.println("✅ 完美！這組腳位可以使用！");
    Serial.println("請記住這組腳位並更新主程式。");
    Serial.println("\n按 Enter 繼續測試下一組...\n");
  } else if (successCount > 0) {
    Serial.print("⚠️ 部分成功 (");
    Serial.print(successCount);
    Serial.println("/3)，接線可能不穩定");
    Serial.println("按 Enter 繼續測試下一組...\n");
  } else {
    Serial.println("❌ 失敗，測試下一組腳位...\n");
    delay(1000);
    currentPin = (currentPin + 1) % numPins;
    if (currentPin == 0) {
      Serial.println("\n⚠️ 所有腳位都測試完畢但都失敗！");
      Serial.println("可能問題：");
      Serial.println("  1. HC-SR04 沒接 5V 電源");
      Serial.println("  2. 接線錯誤或鬆脫");
      Serial.println("  3. HC-SR04 損壞");
      Serial.println("\n重新開始測試...\n");
      delay(2000);
    }
    testCurrentPin();
    return;
  }
  
  // 等待使用者輸入
  while (!Serial.available()) {
    delay(100);
  }
  while (Serial.available()) {
    Serial.read();
  }
  
  currentPin = (currentPin + 1) % numPins;
  testCurrentPin();
}

void testCurrentPin() {
  Serial.println("════════════════════════════════");
  Serial.print("測試腳位組合 ");
  Serial.print(currentPin + 1);
  Serial.print("/");
  Serial.println(numPins);
  Serial.print("📍 ");
  Serial.println(pins[currentPin].name);
  Serial.print("   Trig: GPIO ");
  Serial.println(pins[currentPin].trig);
  Serial.print("   Echo: GPIO ");
  Serial.println(pins[currentPin].echo);
  Serial.println("════════════════════════════════\n");
  
  pinMode(pins[currentPin].trig, OUTPUT);
  pinMode(pins[currentPin].echo, INPUT);
  digitalWrite(pins[currentPin].trig, LOW);
  delay(100);
}

/*
 * ESP32 HC-SR04 終極診斷程式
 * 包含電源測試和更多腳位組合
 */

// 擴展的腳位測試組合
struct PinPair {
  int trig;
  int echo;
  const char* name;
};

PinPair pins[] = {
  {32, 33, "GPIO 32/33 (ADC1)"},
  {18, 19, "GPIO 18/19 (SPI)"},
  {16, 17, "GPIO 16/17 (UART2)"},
  {27, 14, "GPIO 27/14"},
  {21, 22, "GPIO 21/22 (I2C)"},
  {23, 5, "GPIO 23/5"},
  {13, 15, "GPIO 13/15"},
  {4, 2, "GPIO 4/2"},
  {25, 26, "GPIO 25/26 (DAC)"}
};

int currentPin = 0;
const int numPins = sizeof(pins) / sizeof(pins[0]);

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n╔═══════════════════════════════════════╗");
  Serial.println("║  ESP32 HC-SR04 終極診斷程式         ║");
  Serial.println("╚═══════════════════════════════════════╝\n");
  
  Serial.println("⚡ 電源診斷：");
  Serial.println("請用三用電表檢查：");
  Serial.println("  HC-SR04 VCC 到 GND 電壓 = ??? V");
  Serial.println("  ✅ 應該是 4.5V - 5.5V");
  Serial.println("  ❌ 如果是 3.3V → 接線錯誤！");
  Serial.println("  ❌ 如果是 0V → 沒接電源！");
  Serial.println();
  
  Serial.print("將測試 ");
  Serial.print(numPins);
  Serial.println(" 組腳位組合...\n");
  
  delay(2000);
  testCurrentPin();
}

void loop() {
  int trig = pins[currentPin].trig;
  int echo = pins[currentPin].echo;
  
  // 測量 5 次（增加測試次數）
  int successCount = 0;
  long totalDuration = 0;
  
  for (int i = 0; i < 5; i++) {
    // 確保 Trig 為 LOW
    digitalWrite(trig, LOW);
    delayMicroseconds(2);
    
    // 發送觸發脈衝
    digitalWrite(trig, HIGH);
    delayMicroseconds(10);
    digitalWrite(trig, LOW);
    
    // 讀取 Echo
    long duration = pulseIn(echo, HIGH, 30000);
    
    Serial.print("  #");
    Serial.print(i + 1);
    Serial.print(": ");
    
    if (duration == 0) {
      Serial.println("0 us ❌");
    } else {
      float distance = duration * 0.034 / 2;
      Serial.print(duration);
      Serial.print(" us → ");
      Serial.print(distance);
      Serial.println(" cm ✅");
      successCount++;
      totalDuration += duration;
    }
    
    delay(200);
  }
  
  // 評估結果
  Serial.println("\n─────────────────────────────────");
  Serial.print("成功率: ");
  Serial.print(successCount);
  Serial.print("/5 (");
  Serial.print(successCount * 20);
  Serial.println("%)");
  
  if (successCount >= 4) {
    Serial.println("\n🎉 找到了！這組腳位可以使用！");
    Serial.print("平均 Duration: ");
    Serial.print(totalDuration / successCount);
    Serial.println(" us");
    Serial.println("\n請更新主程式使用這組腳位：");
    Serial.print("  const int TRIG_PIN = ");
    Serial.print(trig);
    Serial.println(";");
    Serial.print("  const int ECHO_PIN = ");
    Serial.print(echo);
    Serial.println(";");
    Serial.println("\n輸入 'n' + Enter 繼續測試其他腳位");
    Serial.println("或重新上傳主程式使用這組腳位");
  } else if (successCount > 0) {
    Serial.println("\n⚠️ 部分成功，但不穩定");
    Serial.println("可能原因：");
    Serial.println("  1. 電源不穩定（USB 供電不足）");
    Serial.println("  2. 接線接觸不良");
    Serial.println("  3. 麵包板接觸不良");
  } else {
    Serial.println("\n❌ 完全失敗");
  }
  
  Serial.println("\n3 秒後測試下一組...\n");
  delay(3000);
  
  // 下一組腳位
  currentPin++;
  if (currentPin >= numPins) {
    Serial.println("\n╔═══════════════════════════════════════╗");
    Serial.println("║  所有腳位測試完畢                   ║");
    Serial.println("╚═══════════════════════════════════════╝\n");
    
    Serial.println("⚠️ 所有腳位都無法正常工作！");
    Serial.println("\n硬體檢查清單：");
    Serial.println("□ HC-SR04 的 VCC 接到 ESP32 的 5V (或 VIN)");
    Serial.println("□ HC-SR04 的 GND 接到 ESP32 的 GND");
    Serial.println("□ 所有接線牢固（重新插拔測試）");
    Serial.println("□ 用三用電表測量 VCC 到 GND 電壓 = 4.5-5.5V");
    Serial.println("□ HC-SR04 沒有物理損壞");
    Serial.println("□ 嘗試更換 HC-SR04 感測器");
    Serial.println("\n建議：");
    Serial.println("1. 用外部 5V 電源供電給 HC-SR04");
    Serial.println("2. 檢查 ESP32 的 5V 輸出是否正常");
    Serial.println("3. 嘗試使用 Arduino（5V 系統）測試感測器");
    Serial.println("\n10 秒後重新測試...\n");
    
    delay(10000);
    currentPin = 0;
  }
  
  testCurrentPin();
}

void testCurrentPin() {
  Serial.println("════════════════════════════════════════");
  Serial.print("📍 測試組合 ");
  Serial.print(currentPin + 1);
  Serial.print("/");
  Serial.print(numPins);
  Serial.print(": ");
  Serial.println(pins[currentPin].name);
  Serial.print("   Trig: GPIO ");
  Serial.println(pins[currentPin].trig);
  Serial.print("   Echo: GPIO ");
  Serial.println(pins[currentPin].echo);
  Serial.println("════════════════════════════════════════");
  
  // 設定腳位
  pinMode(pins[currentPin].trig, OUTPUT);
  pinMode(pins[currentPin].echo, INPUT);
  
  // 初始化 Trig
  digitalWrite(pins[currentPin].trig, LOW);
  delay(50);
  
  // 檢查 Echo 初始狀態
  int echoState = digitalRead(pins[currentPin].echo);
  Serial.print("Echo 初始狀態: ");
  Serial.println(echoState == LOW ? "LOW ✅" : "HIGH ⚠️");
  Serial.println();
}

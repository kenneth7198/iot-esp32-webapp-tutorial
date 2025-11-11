/*
 * ESP32 HC-SR04 超強診斷程式
 * 用於找出 HC-SR04 無法工作的根本原因
 */

// 測試多組腳位（依次嘗試）
struct PinPair {
  int trig;
  int echo;
  const char* name;
};

PinPair pinConfigs[] = {
  {25, 26, "GPIO 25/26 (建議)"},
  {32, 33, "GPIO 32/33 (ADC)"},
  {27, 14, "GPIO 27/14"},
  {16, 17, "GPIO 16/17 (RX2/TX2)"},
  {18, 19, "GPIO 18/19 (SPI)"}
};

int currentConfig = 0;
const int NUM_CONFIGS = sizeof(pinConfigs) / sizeof(pinConfigs[0]);

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║  ESP32 HC-SR04 超強診斷程式          ║");
  Serial.println("╚════════════════════════════════════════╝\n");
  
  Serial.println("📋 將測試以下腳位組合:");
  for (int i = 0; i < NUM_CONFIGS; i++) {
    Serial.print("  ");
    Serial.print(i + 1);
    Serial.print(". ");
    Serial.print(pinConfigs[i].name);
    Serial.print(" - Trig: GPIO ");
    Serial.print(pinConfigs[i].trig);
    Serial.print(", Echo: GPIO ");
    Serial.println(pinConfigs[i].echo);
  }
  Serial.println();
  
  // 開始測試第一組腳位
  testCurrentConfig();
}

void loop() {
  // 測量 5 次
  for (int i = 0; i < 5; i++) {
    long duration = measureDistance();
    
    Serial.print("📏 測試 #");
    Serial.print(i + 1);
    Serial.print(": Duration = ");
    Serial.print(duration);
    Serial.print(" us");
    
    if (duration == 0) {
      Serial.println(" ❌ 無回應");
    } else {
      float distance = duration * 0.034 / 2;
      Serial.print(" | 距離 = ");
      Serial.print(distance);
      Serial.print(" cm");
      
      if (distance < 2 || distance > 400) {
        Serial.println(" ⚠️ 超出範圍");
      } else {
        Serial.println(" ✅ 正常");
      }
    }
    
    delay(300);
  }
  
  // 檢查是否所有測量都失敗
  Serial.println("\n⏸️  5 次測量完成");
  Serial.println("請選擇:");
  Serial.println("  1. 按任意鍵 + Enter = 繼續測試此腳位");
  Serial.println("  2. 輸入 'n' + Enter = 測試下一組腳位");
  Serial.println("  3. 輸入數字 (1-5) + Enter = 跳到指定腳位組合");
  
  // 等待使用者輸入
  while (!Serial.available()) {
    delay(100);
  }
  
  String input = Serial.readStringUntil('\n');
  input.trim();
  
  if (input == "n" || input == "N") {
    currentConfig = (currentConfig + 1) % NUM_CONFIGS;
    testCurrentConfig();
  } else if (input.length() == 1 && input[0] >= '1' && input[0] <= '5') {
    int selected = input[0] - '1';
    if (selected < NUM_CONFIGS) {
      currentConfig = selected;
      testCurrentConfig();
    }
  } else {
    Serial.println("繼續測試目前腳位...\n");
  }
}

void testCurrentConfig() {
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.print("║  測試腳位組合 ");
  Serial.print(currentConfig + 1);
  Serial.println("/5                   ║");
  Serial.println("╚════════════════════════════════════════╝");
  
  Serial.print("📍 ");
  Serial.println(pinConfigs[currentConfig].name);
  Serial.print("   Trig: GPIO ");
  Serial.println(pinConfigs[currentConfig].trig);
  Serial.print("   Echo: GPIO ");
  Serial.println(pinConfigs[currentConfig].echo);
  Serial.println();
  
  // 設定腳位
  pinMode(pinConfigs[currentConfig].trig, OUTPUT);
  pinMode(pinConfigs[currentConfig].echo, INPUT);
  
  // 初始化 Trig
  digitalWrite(pinConfigs[currentConfig].trig, LOW);
  delay(100);
  
  // 測試 Echo 腳位狀態
  Serial.print("🔌 Echo 腳位初始狀態: ");
  int echoState = digitalRead(pinConfigs[currentConfig].echo);
  Serial.print(echoState);
  Serial.println(echoState == LOW ? " (LOW - 正常)" : " (HIGH - 可能有問題)");
  
  // 測試 Trig 腳位
  Serial.print("🔌 測試 Trig 腳位...");
  digitalWrite(pinConfigs[currentConfig].trig, HIGH);
  delay(10);
  digitalWrite(pinConfigs[currentConfig].trig, LOW);
  Serial.println(" 完成");
  
  Serial.println("\n開始距離測量:");
  Serial.println("─────────────────────────────────────────");
}

long measureDistance() {
  int trig = pinConfigs[currentConfig].trig;
  int echo = pinConfigs[currentConfig].echo;
  
  // 確保 Trig 為 LOW
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  
  // 發送 10us 脈衝
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  
  // 讀取 Echo（30ms timeout）
  long duration = pulseIn(echo, HIGH, 30000);
  
  return duration;
}

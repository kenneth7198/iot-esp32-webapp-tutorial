/*
 * ESP32 HC-SR04 純測試（無 WiFi）
 * 測試 GPIO 32/33（之前診斷程式成功的腳位）
 */

const int TRIG_PIN = 32;
const int ECHO_PIN = 33;

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n╔═════════════════════════════════════╗");
  Serial.println("║  HC-SR04 純測試（無 WiFi）        ║");
  Serial.println("║  GPIO 32/33（診斷程式成功腳位）  ║");
  Serial.println("╚═════════════════════════════════════╝");
  Serial.println("Trig: GPIO 32");
  Serial.println("Echo: GPIO 33");
  Serial.println();
  
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);
  delay(100);
  
  Serial.print("Echo 初始狀態: ");
  Serial.println(digitalRead(ECHO_PIN) == LOW ? "LOW ✅" : "HIGH ⚠️");
  Serial.println("\n開始測量...\n");
}

void loop() {
  // 重試機制
  long duration = 0;
  for (int retry = 0; retry < 3; retry++) {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    
    duration = pulseIn(ECHO_PIN, HIGH, 30000);
    
    if (duration > 0) break;
    delayMicroseconds(100);
  }
  
  Serial.print("Duration: ");
  Serial.print(duration);
  Serial.print(" us");
  
  if (duration == 0) {
    Serial.println(" ❌");
  } else {
    float distance = duration * 0.034 / 2;
    Serial.print(" | ");
    Serial.print(distance);
    Serial.println(" cm ✅");
  }
  
  delay(300);
}

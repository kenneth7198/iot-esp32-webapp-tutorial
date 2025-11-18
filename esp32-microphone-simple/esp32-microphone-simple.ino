/*
 * ESP32 麥克風控制 LED (簡化版)
 * 純感測器版本 - 無網路功能
 * 
 * 硬體連接:
 * - 麥克風模組 (類比輸出) -> GPIO 34 (ADC1_CH6)
 * - LED -> GPIO 23 (可 PWM 輸出)
 * 
 * 功能:
 * - 讀取麥克風音量
 * - 根據音量調整 LED 亮度
 * - 超過閾值時 LED 全亮
 */

// ==================== 硬體腳位 ====================
const int MIC_PIN = 34;        // 麥克風類比輸入 (ADC1_CH6)
const int LED_PIN = 23;        // LED 輸出腳位

// ==================== PWM 設定 ====================
const int PWM_FREQ = 5000;     // PWM 頻率 5kHz
const int PWM_RESOLUTION = 8;  // 8-bit 解析度 (0-255)

// ==================== 感測器變數 ====================
int micValue = 0;              // 麥克風讀值
int mappedValue = 0;           // 映射後的值 (0-255)
const int THRESHOLD = 2500;    // 觸發閾值 (ESP32 ADC 是 12-bit: 0-4095)

// ==================== Setup ====================
void setup() {
  Serial.begin(115200);
  Serial.println("\n\n");
  Serial.println("╔══════════════════════════════════════╗");
  Serial.println("║   ESP32 麥克風 + LED 控制 (簡化版)  ║");
  Serial.println("╚══════════════════════════════════════╝");
  
  // 初始化 LED PWM
  ledcAttach(LED_PIN, PWM_FREQ, PWM_RESOLUTION);
  ledcWrite(LED_PIN, 0);  // 初始關閉
  
  Serial.println("✅ LED PWM 初始化完成");
  
  // 測試 LED
  Serial.println("🔦 測試 LED...");
  for (int i = 0; i <= 255; i += 5) {
    ledcWrite(LED_PIN, i);
    delay(10);
  }
  for (int i = 255; i >= 0; i -= 5) {
    ledcWrite(LED_PIN, i);
    delay(10);
  }
  ledcWrite(LED_PIN, 0);
  Serial.println("✅ LED 測試完成");
  
  Serial.println("\n🎤 開始監測麥克風...\n");
}

// ==================== Loop ====================
void loop() {
  // 讀取麥克風數值 (ESP32 ADC 是 12-bit: 0-4095)
  micValue = analogRead(MIC_PIN);
  
  // 映射到 0-255 (LED PWM 範圍)
  mappedValue = map(micValue, 0, 4095, 0, 255);
  
  // 輸出到 LED
  ledcWrite(LED_PIN, mappedValue);
  
  // 序列埠輸出
  Serial.print("🎤 麥克風: ");
  Serial.print(micValue);
  Serial.print(" | LED 亮度: ");
  Serial.print(mappedValue);
  
  // 檢查是否超過閾值
  if (micValue > THRESHOLD) {
    Serial.print(" | 🔊 大聲！");
    ledcWrite(LED_PIN, 255);  // LED 全亮
  }
  Serial.println();
  
  delay(20);  // 延遲 20ms
}


#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"   
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


#define PIN_SW1 2
#define PIN_SW2 3
#define PIN_SW3 4
#define PIN_LED 13

// OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Sensor
MAX30105 particleSensor;

// App state
enum RunState { IDLE, SAMPLING };
RunState state = IDLE;

// Button debounce
struct Btn {
  uint8_t pin;
  bool lastStable{HIGH};
  bool lastRead{HIGH};
  unsigned long lastChangeMs{0};
  bool pressedEdge{false};
  unsigned long pressedMs{0};
} btn1{PIN_SW1}, btn2{PIN_SW2}, btn3{PIN_SW3};

const unsigned long DEBOUNCE_MS = 25;
const unsigned long LONG_PRESS_MS = 700;

// Display modes
enum ViewMode { VIEW_MAIN, VIEW_SCOPE, VIEW_INFO };
ViewMode view = VIEW_MAIN;

// SpO2/HR buffers per Maxim example
const int32_t BUFFER_LEN = 100;   // ~4 seconds at 25Hz (I’ll use ~25Hz)
uint32_t irBuffer[BUFFER_LEN];
uint32_t redBuffer[BUFFER_LEN];
int32_t spo2;            // SPO2 value
int8_t validSPO2;        // Indicator to show if SPO2 calculation is valid
int32_t heartRate;       // Heart rate value
int8_t validHeartRate;   // Indicator to show if heart rate calculation is valid

// For scope view
#define SCOPE_W 120
#define SCOPE_H 28
uint8_t scopeX = 0;

void drawHeader(const char* title) {
  display.fillRect(0, 0, SCREEN_WIDTH, 10, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setCursor(2, 1);
  display.print(title);
  display.setTextColor(SSD1306_WHITE);
}

void showIdle() {
  display.clearDisplay();
  drawHeader("DIY SpO2 - IDLE");
  display.setCursor(0, 14);
  display.println("Press SW1: Start/Stop");
  display.println("Press SW2: Change View");
  display.println("Press SW3: Reset/Cal");
  display.setCursor(0, 44);
  display.println("Place finger on sensor.");
  display.display();
}

void showMain() {
  display.clearDisplay();
  drawHeader("Live Readings");
  display.setCursor(0, 14);
  display.print("SpO2: ");
  if (validSPO2) { display.print(spo2); display.println(" %"); }
  else           { display.println("-- %"); }

  display.print("BPM : ");
  if (validHeartRate) { display.println(heartRate); }
  else                { display.println("--"); }

  display.setCursor(0, 34);
  display.println("SW1=Stop  SW2=Mode  SW3=Reset");

  // Status line
  display.setCursor(0, 52);
  display.print("Sensor: ");
  display.println((validSPO2 && validHeartRate) ? "OK" : "Adjust finger...");
  display.display();
}

void showInfo() {
  display.clearDisplay();
  drawHeader("Info");
  display.setCursor(0, 14);
  display.println("MAX30102 + SSD1306");
  display.println("Algo: Maxim (SparkFun)");
  display.println("Not medical device");
  display.setCursor(0, 44);
  display.println("SW2: Next  SW1: Stop");
  display.display();
}

void showScope(uint16_t sample) {
  display.clearDisplay();
  drawHeader("IR Scope");
  // Scope area
  int x0 = 4, y0 = 14;
  display.drawRect(x0-1, y0-1, SCOPE_W+2, SCOPE_H+2, SSD1306_WHITE);

  // Map sample to scope height (rough)
  static uint16_t minV = 65535, maxV = 0;
  if (sample < minV) minV = sample;
  if (sample > maxV) maxV = sample;
  uint16_t minR = minV, maxR = maxV;
  if (maxR == minR) maxR = minR + 1;

  uint8_t y = y0 + SCOPE_H - (uint8_t)((sample - minR) * (uint32_t)SCOPE_H / (maxR - minR));
  
  display.drawFastVLine(x0 + scopeX, y0, SCOPE_H, SSD1306_BLACK);
  display.drawPixel(x0 + scopeX, y, SSD1306_WHITE);

  scopeX = (scopeX + 1) % SCOPE_W;
  if (scopeX == 0) {
    
    display.fillRect(x0, y0, SCOPE_W, SCOPE_H, SSD1306_BLACK);
    minV = 65535; maxV = 0;
  }

  display.setCursor(0, 46);
  display.print("SpO2:");
  if (validSPO2) { display.print(spo2); display.print("%  "); }
  else           { display.print("--%  "); }
  display.print("BPM:");
  if (validHeartRate) display.print(heartRate); else display.print("--");

  display.display();
}

// Debounce + edge/long-press
void updateBtn(Btn& b) {
  bool r = digitalRead(b.pin);
  unsigned long now = millis();

  if (r != b.lastRead) { b.lastRead = r; b.lastChangeMs = now; }

  if ((now - b.lastChangeMs) > DEBOUNCE_MS && r != b.lastStable) {
    b.lastStable = r;
    if (r == LOW) {               // pressed (using INPUT_PULLUP)
      b.pressedEdge = true;
      b.pressedMs = now;
    }
  }

  // clear edge after I read it elsewhere
}

bool btnPressed(Btn& b) {
  bool e = b.pressedEdge;
  b.pressedEdge = false;
  return e;
}

bool btnLongHeld(Btn& b) {
  return (b.lastStable == LOW) && (millis() - b.pressedMs > LONG_PRESS_MS);
}

void sensorConfigure() {
  
  byte ledBrightness = 0x24; // 0x02..0xFF
  byte sampleAverage = 4;    // 1,2,4,8,16,32
  byte ledMode = 2;          // 2 = Red + IR
  byte sampleRate = 100;     // 50, 100, 200, 400, 800, 1000, 1600, 3200
  int pulseWidth = 411;      // 69, 118, 215, 411 (higher = wider)
  int adcRange = 16384;      // 2048, 4096, 8192, 16384

  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);
  particleSensor.setPulseAmplitudeGreen(0); // Not used
}

bool sensorBegin() {
  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) return false;
  sensorConfigure();
  return true;
}

// Fill initial buffers (blocking) for first SPO2 solve
bool primeBuffers() {
  int32_t num = BUFFER_LEN;
  for (int i = 0; i < num; i++) {
    while (!particleSensor.available()) particleSensor.check(); // wait for FIFO
    redBuffer[i] = particleSensor.getRed();
    irBuffer[i]  = particleSensor.getIR();
    particleSensor.nextSample();
  }
  // Initial calculation
  maxim_heart_rate_and_oxygen_saturation(irBuffer, num, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);
  return true;
}

void setup() {
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);

  pinMode(PIN_SW1, INPUT_PULLUP);
  pinMode(PIN_SW2, INPUT_PULLUP);
  pinMode(PIN_SW3, INPUT_PULLUP);

  Serial.begin(115200);
  Wire.begin();

  // OLED init
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    // If OLED fails, continue with serial only
  }
  display.clearDisplay();
  drawHeader("DIY SpO2");
  display.setCursor(0, 14);
  display.println("Init...");
  display.display();

  // Sensor init
  if (!sensorBegin()) {
    display.clearDisplay();
    drawHeader("Sensor Error");
    display.setCursor(0, 14);
    display.println("MAX30102 not found");
    display.println("Check Vcc/GND/SDA/SCL");
    display.display();
  } else {
    display.setCursor(0, 24);
    display.println("Sensor OK");
    display.display();
  }

  showIdle();
}

void startSampling() {
  if (!sensorBegin()) return;
  primeBuffers();
  state = SAMPLING;
  digitalWrite(PIN_LED, HIGH);
  scopeX = 0;
}

void stopSampling() {
  state = IDLE;
  digitalWrite(PIN_LED, LOW);
  showIdle();
}

void resetCal() {

  primeBuffers();
}

void loop() {
  // Update buttons
  updateBtn(btn1); updateBtn(btn2); updateBtn(btn3);

  // Global controls
  if (btnPressed(btn1)) {
    if (state == IDLE) startSampling(); else stopSampling();
  }
  if (state == SAMPLING && btnPressed(btn2)) {
    view = (ViewMode)((view + 1) % 3);
  }
  if (state == SAMPLING && (btnPressed(btn3) || btnLongHeld(btn3))) {
    resetCal();
  }

  if (state == IDLE) {
    delay(10);
    return;
  }


  const int32_t SEG = 25;
  for (int i = 0; i < BUFFER_LEN - SEG; i++) {
    redBuffer[i] = redBuffer[i + SEG];
    irBuffer[i]  = irBuffer[i + SEG];
  }
  for (int i = BUFFER_LEN - SEG; i < BUFFER_LEN; i++) {
    while (!particleSensor.available()) particleSensor.check();
    redBuffer[i] = particleSensor.getRed();
    irBuffer[i]  = particleSensor.getIR();
    particleSensor.nextSample();
  }

  maxim_heart_rate_and_oxygen_saturation(irBuffer, BUFFER_LEN, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);

  // Render
  switch (view) {
    case VIEW_MAIN:  showMain(); break;
    case VIEW_SCOPE: showScope((uint16_t)(irBuffer[BUFFER_LEN - 1] >> 8)); break;
    case VIEW_INFO:  showInfo(); break;
  }
}

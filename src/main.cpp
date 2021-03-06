#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/Picopixel.h>
#include <Fonts/FreeSansBold9pt7b.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
// Declaration for SSD1306 display connected using software SPI (default case):
/*
#define OLED_MOSI  D7
#define OLED_CLK   D5
#define OLED_DC    D2
#define OLED_CS    D8
#define OLED_RESET D3
*/
#define OLED_MOSI  18
#define OLED_CLK   19
#define OLED_DC    16
#define OLED_CS    23
#define OLED_RESET 17
/*
orange-white: 5V (from motor controller)
orange:       GND (from motor controller)
green-white:  EL
green:        Signal
blue-white:   V/R
blue:         Z/F
brown-white:  3.3V (from ESP devboard)
brown:        TMP36 analog signal
*/
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT,
  OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

/*
#define POT_PIN A0
#define PWM_PIN D4
#define RPM_PIN D1
*/
#define POT_PIN 36
#define PWM_PIN 26
#define RPM_PIN 27
#define DAC_PIN 25

int prev_x = 0;
int speed = 16; // pixels per second
int prev_timestamp = -1;
int dir = 1;
volatile bool update_text = true;
volatile float rpm = 0.0;
volatile int pot = 0;
volatile int prev_pot = 0;
volatile int rpm_count = 0;
int update_rpm_count = 0;
int rpm_timestamp = 0;


void updateText();
void drawAnimation();
int readPot();
ICACHE_RAM_ATTR void rpmISR();


void setup() {
  Serial.begin(115200);
  Serial.println(F("DREJA setup"));
  
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(POT_PIN, INPUT);
  pinMode(RPM_PIN, INPUT);//INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(RPM_PIN), rpmISR, FALLING);
  pinMode(PWM_PIN, OUTPUT);
  //analogWriteFreq(10000);
  ledcSetup(0, 10000, 12);
  ledcAttachPin(PWM_PIN, 0);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  //display.setRotation(1);
  display.setFont(&Picopixel);
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.clearDisplay();

  for (int n=0; n<4; n++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
  }
  Serial.println(F("Setup complete"));
}

void loop() {
  if (update_text) {
    update_text = false;
    display.clearDisplay();
    updateText();
  }

  pot = readPot();
  if (pot != prev_pot) {
    Serial.print("Pot: ");
    Serial.println(pot);
    //analogWrite(PWM_PIN, pot);
    ledcWrite(0, pot);
    update_text = true;
    prev_pot = pot;
  }

  if (++update_rpm_count >= 20) {
    float time = float(millis() - rpm_timestamp) / 1000.0;
    rpm = ((float)(60*rpm_count)) / 45.0 * 1.0/time; // find out actual numbers for hoverboard signal
    rpm_timestamp = millis();
    rpm_count = 0;
    update_rpm_count = 0;
    update_text = true;
  }

  drawAnimation();
  display.display();
  delay(50);
}

void updateText() {
  display.setFont(&Picopixel);
  display.setCursor(0,5);
  display.println(F("DREJA"));
  display.drawLine(0,7,128,7,SSD1306_WHITE);
  display.setCursor(0,14);

  display.println(F("RPM"));
  display.println();
  display.setFont(&FreeSansBold9pt7b);
  display.print(rpm, 0);  

  display.setFont(&Picopixel);
  display.println();
  display.println(F("POT"));
  display.println();
  display.setFont(&FreeSansBold9pt7b);
  display.print(pot);
  display.setFont(&Picopixel);
  display.println();
}

void drawAnimation() {
  int now = millis();
  if (prev_timestamp < 0) {
    prev_timestamp = millis();
  }
  int step = (now - prev_timestamp) * speed / 1000;
  int x = prev_x + step*dir;
  if (x >= display.width()) { // right bounce
    dir = -1;
    x = prev_x + step*dir;
  }
  else if (x < 0) { // left bounce
    dir = 1;
    x = prev_x + step*dir;
  }
  display.drawPixel(prev_x, display.height()-1, SSD1306_BLACK);
  display.drawPixel(x, display.height()-1, SSD1306_WHITE);
  prev_x = x;
  prev_timestamp = now;
  
}

int readPot() {
  int val = 0;
  for(int i = 0; i < 10; i++) {
    val += analogRead(A0);
    delay(5);
  }
  return val / 10;
}

void rpmISR() {
  rpm_count ++;
}
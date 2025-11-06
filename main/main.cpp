#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const int battery1Pin = A0;
const int battery2Pin = A1;
const int battery3Pin = A2;
const int battery4Pin = A3;
const int tempPin = A4;
const int relayPin = 8;
const int redLEDPin = 13;
const int greenLEDPin = 12;
const int ovfltPin = 9;
const int uvfltPin = 10;
const int balEnablePin = 11;

LiquidCrystal_I2C lcd(0x27, 16, 2);

const float overVoltageThreshold = 14.5;
const float underVoltageThreshold = 10.5;
const float overTemperatureThreshold = 45.0;
const float voltageDividerRatio = 11.0;

float batt[4];
float temperatureC;
unsigned long previousMillis = 0;
bool charging = false;

void setup() {
  pinMode(relayPin, OUTPUT);
  pinMode(redLEDPin, OUTPUT);
  pinMode(greenLEDPin, OUTPUT);
  pinMode(ovfltPin, INPUT_PULLUP);
  pinMode(uvfltPin, INPUT_PULLUP);
  pinMode(balEnablePin, OUTPUT);
  pinMode(tempPin, INPUT);

  digitalWrite(relayPin, LOW);
  digitalWrite(redLEDPin, LOW);
  digitalWrite(greenLEDPin, HIGH);

  Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("System Initializing");
  delay(2000);
  lcd.clear();
}

void loop() {
  readInputs();
  faultHandling();
  controlCharging();
  activeBalancing();
  displayData();
}

void readInputs() {
  batt[0] = analogRead(battery1Pin) * (5.0 / 1023.0) * voltageDividerRatio;
  batt[1] = analogRead(battery2Pin) * (5.0 / 1023.0) * voltageDividerRatio;
  batt[2] = analogRead(battery3Pin) * (5.0 / 1023.0) * voltageDividerRatio;
  batt[3] = analogRead(battery4Pin) * (5.0 / 1023.0) * voltageDividerRatio;
  float sensorVoltage = analogRead(tempPin) * (5.0 / 1023.0);
  temperatureC = sensorVoltage * 100.0;
}

void controlCharging() {
  unsigned long currentMillis = millis();
  const unsigned long interval = 5000;

  if (!charging && currentMillis - previousMillis >= interval) {
    digitalWrite(relayPin, HIGH);
    charging = true;
    previousMillis = currentMillis;
    Serial.println("Charging: ON");
  }

  if (charging && currentMillis - previousMillis >= interval) {
    digitalWrite(relayPin, LOW);
    charging = false;
    previousMillis = currentMillis;
    Serial.println("Charging: OFF (measuring OCV)");
  }
}

void activeBalancing() {
  bool ovFault = digitalRead(ovfltPin) == LOW;
  bool uvFault = digitalRead(uvfltPin) == LOW;

  if (!ovFault && !uvFault) {
    digitalWrite(balEnablePin, HIGH);
  } else {
    digitalWrite(balEnablePin, LOW);
  }
}

void displayData() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("B1:");
  lcd.print(batt[0], 1);
  lcd.print(" B2:");
  lcd.print(batt[1], 1);
  lcd.setCursor(0, 1);
  lcd.print("B3:");
  lcd.print(batt[2], 1);
  lcd.print(" B4:");
  lcd.print(batt[3], 1);

  Serial.print("Voltages: ");
  for (int i = 0; i < 4; i++) {
    Serial.print(batt[i], 2);
    Serial.print("V ");
  }
  Serial.print("Temp:");
  Serial.print(temperatureC);
  Serial.println("C");
}

void faultHandling() {
  bool fault = false;
  for (int i = 0; i < 4; i++) {
    if (batt[i] > overVoltageThreshold || batt[i] < underVoltageThreshold)
      fault = true;
  }
  if (temperatureC > overTemperatureThreshold)
    fault = true;

  if (fault) {
    digitalWrite(relayPin, LOW);
    digitalWrite(redLEDPin, HIGH);
    digitalWrite(greenLEDPin, LOW);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("FAULT DETECTED!");
    lcd.setCursor(0, 1);
    lcd.print("Charging Stopped");
    Serial.println("FAULT DETECTED! Charger OFF.");
    while (1);
  } else {
    digitalWrite(redLEDPin, LOW);
    digitalWrite(greenLEDPin, HIGH);
  }
}

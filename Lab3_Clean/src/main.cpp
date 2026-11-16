#include <Arduino.h>

#define SPEAKER_PIN PB0

// Candidate GPIO pins used on RT-Spark / STM32F407 boards for onboard I2C sensors
const uint8_t pins[] = {
  PA0, PA1, PA2, PA3, PA4, PA5, PA6, PA7, PA8, PA11, PA12, PA15,
  PB0, PB1, PB3, PB4, PB5, PB6, PB7, PB8, PB9, PB10, PB11, PB12, PB13, PB14, PB15,
  PC0, PC1, PC2, PC3, PC4, PC5, PC6, PC7, PC8, PC9, PC10, PC11, PC12, PC13,
  PD0, PD1, PD2, PD3, PD4, PD5, PD6, PD7, PD11, PD12, PD13, PD14, PD15,
  PE0, PE1, PE2, PE3, PE4, PE5, PE6, PF0, PF1, PG0, PG1
};
const char* pinNames[] = {
  "PA0","PA1","PA2","PA3","PA4","PA5","PA6","PA7","PA8","PA11","PA12","PA15",
  "PB0","PB1","PB3","PB4","PB5","PB6","PB7","PB8","PB9","PB10","PB11","PB12","PB13","PB14","PB15",
  "PC0","PC1","PC2","PC3","PC4","PC5","PC6","PC7","PC8","PC9","PC10","PC11","PC12","PC13",
  "PD0","PD1","PD2","PD3","PD4","PD5","PD6","PD7","PD11","PD12","PD13","PD14","PD15",
  "PE0","PE1","PE2","PE3","PE4","PE5","PE6","PF0","PF1","PG0","PG1"
};
const int NUM_PINS = sizeof(pins) / sizeof(pins[0]);

uint8_t sdaPin = PC1;
uint8_t sclPin = PC0;
uint8_t devAddr = 0x68;
bool found = false;

void i2cDelay() { delayMicroseconds(4); }
void sdaHigh() { pinMode(sdaPin, INPUT_PULLUP); }
void sdaLow()  { pinMode(sdaPin, OUTPUT); digitalWrite(sdaPin, LOW); }
void sclHigh() { pinMode(sclPin, INPUT_PULLUP); }
void sclLow()  { pinMode(sclPin, OUTPUT); digitalWrite(sclPin, LOW); }

void i2cStart() { sdaHigh(); sclHigh(); i2cDelay(); sdaLow(); i2cDelay(); sclLow(); i2cDelay(); }
void i2cStop()  { sdaLow(); sclLow(); i2cDelay(); sclHigh(); i2cDelay(); sdaHigh(); i2cDelay(); }

bool i2cWriteByte(uint8_t b) {
  for (int i = 7; i >= 0; i--) {
    if ((b >> i) & 1) sdaHigh(); else sdaLow();
    i2cDelay(); sclHigh(); i2cDelay(); sclLow(); i2cDelay();
  }
  sdaHigh(); i2cDelay(); sclHigh(); i2cDelay();
  bool ack = (digitalRead(sdaPin) == LOW);
  sclLow(); i2cDelay();
  return ack;
}

uint8_t i2cReadByte(bool ack) {
  uint8_t b = 0;
  sdaHigh();
  for (int i = 7; i >= 0; i--) {
    i2cDelay(); sclHigh(); i2cDelay();
    if (digitalRead(sdaPin)) b |= (1 << i);
    sclLow();
  }
  if (ack) sdaLow(); else sdaHigh();
  i2cDelay(); sclHigh(); i2cDelay(); sclLow(); i2cDelay();
  sdaHigh();
  return b;
}

void writeReg(uint8_t reg, uint8_t val) {
  i2cStart(); i2cWriteByte(devAddr << 1); i2cWriteByte(reg); i2cWriteByte(val); i2cStop();
}

int16_t read16(uint8_t reg) {
  i2cStart(); i2cWriteByte(devAddr << 1); i2cWriteByte(reg);
  i2cStart(); i2cWriteByte((devAddr << 1) | 1);
  uint8_t hi = i2cReadByte(true);
  uint8_t lo = i2cReadByte(false);
  i2cStop();
  return (int16_t)((hi << 8) | lo);
}

void setup() {
  pinMode(SPEAKER_PIN, OUTPUT);
  digitalWrite(SPEAKER_PIN, LOW);
  Serial.begin(115200);
  delay(500);

  // Scan all pin pairs for an I2C ACK at 0x68 or 0x69
  for (int s = 0; s < NUM_PINS && !found; s++) {
    for (int c = 0; c < NUM_PINS && !found; c++) {
      if (s == c) continue;
      sdaPin = pins[s];
      sclPin = pins[c];
      sdaHigh(); sclHigh();
      // Only test pairs where both lines idle HIGH (have pull-ups)
      if (digitalRead(sdaPin) == HIGH && digitalRead(sclPin) == HIGH) {
        for (uint8_t addr = 0x68; addr <= 0x69; addr++) {
          i2cStart();
          bool ack = i2cWriteByte(addr << 1);
          i2cStop();
          if (ack) {
            devAddr = addr;
            found = true;
            Serial.print("{\"found_sda\":\""); Serial.print(pinNames[s]);
            Serial.print("\",\"found_scl\":\""); Serial.print(pinNames[c]);
            Serial.print("\",\"addr\":\"0x"); Serial.print(addr, HEX);
            Serial.println("\"}");
            break;
          }
        }
      }
      pinMode(sdaPin, INPUT);
      pinMode(sclPin, INPUT);
    }
  }

  if (found) {
    sdaHigh(); sclHigh();
    writeReg(0x6B, 0x01); // Wake up IMU
    delay(100);
  }
}

void loop() {
  if (!found) {
    Serial.println("{\"status\":\"no_i2c_imu_found\"}");
    delay(1000);
    return;
  }

  float ax = read16(0x3B) / 16384.0;
  float ay = read16(0x3D) / 16384.0;
  float az = read16(0x3F) / 16384.0;
  float gx = read16(0x43) / 131.0;
  float gy = read16(0x45) / 131.0;
  float gz = read16(0x47) / 131.0;

  Serial.print("{\"ax\":"); Serial.print(ax, 4);
  Serial.print(",\"ay\":"); Serial.print(ay, 4);
  Serial.print(",\"az\":"); Serial.print(az, 4);
  Serial.print(",\"gx\":"); Serial.print(gx, 4);
  Serial.print(",\"gy\":"); Serial.print(gy, 4);
  Serial.print(",\"gz\":"); Serial.print(gz, 4);
  Serial.println("}");

  delay(200);
}
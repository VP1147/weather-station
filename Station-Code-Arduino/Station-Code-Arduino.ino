#include "Adafruit_Sensor.h"
#include "Adafruit_AM2320.h"
#include <Adafruit_BMP085.h>
#include "RTClib.h"


Adafruit_AM2320 am2320 = Adafruit_AM2320();
Adafruit_BMP085 bmp;
RTC_PCF8563 rtc;

// If true, RTC captures and uses compilation time
// Used if battery if off or switched
bool time_capture = true;

void setup() {
  Serial.begin(9600);
  am2320.begin();
  bmp.begin();
  if(time_capture == true) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    rtc.start();
  }
  rtc.begin();
}

void loop() {
  read_rtc();
  read_am2320();
  read_bmp180();

  Serial.println();
  delay(1000);
}

float read_am2320() {
  float t, h;
  if (am2320.readTemperatureAndHumidity(&t, &h)) {
    Serial.print("T1: "); Serial.print(t); Serial.print(" C  ");
    Serial.print("H: "); Serial.print(h); Serial.print(" %  ");
  } else {
    Serial.println(">>> AM2320 : READ ERROR!");
  }
  return t, h;
}

float read_bmp180() {
  float t = bmp.readTemperature();
  float p = bmp.readPressure();
  Serial.print("T2: "); Serial.print(t); Serial.print(" C  ");
  Serial.print("P: "); Serial.print(p/100); Serial.print(" hPa  ");
}

float read_rtc() {
  DateTime now = rtc.now();
  Serial.print(now.timestamp(DateTime::TIMESTAMP_DATE));
  Serial.print("  ");
  Serial.print(now.timestamp(DateTime::TIMESTAMP_TIME));
  Serial.print("  ");
}
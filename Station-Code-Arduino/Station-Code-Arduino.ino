#include "Adafruit_Sensor.h"
#include "Adafruit_AM2320.h"
#include <Adafruit_BMP085.h>
#include "RTClib.h"
#include <SPI.h>
#include <SD.h>

const int chipSelect = 10;
File dataFile;

Adafruit_AM2320 am2320 = Adafruit_AM2320();
Adafruit_BMP085 bmp;
RTC_PCF8563 rtc;

// If true, RTC captures and uses compilation time
// Used if battery if off or switched
bool time_capture = true;

volatile byte revolutions;
float rpmilli;
float speed;
unsigned long timeold=0 ;

void setup() {
  Serial.begin(9600);
  Serial.println(">> Iniciando ...");
  am2320.begin();
  bmp.begin();
  attachInterrupt(digitalPinToInterrupt(2), rpm_fun, RISING);
  revolutions = 0;
  rpmilli = 0;
  timeold = 0;
  Serial.println(">> Sensores iniciados");
  rtc.begin();
  if(time_capture == true) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    rtc.start();
    Serial.print(">> Relogio definido para "); read_rtc(); Serial.println();
  }
  Serial.print(">> Relogio iniciado em "); read_rtc(); Serial.println();
  if (!SD.begin(chipSelect)) {
    Serial.println(">> Falha ao iniciar o cartao SD");
    return;
  }
  else { Serial.println(">> Cartao SD iniciado"); }
    dataFile = SD.open("data.csv", FILE_WRITE);
  if (dataFile) {
    dataFile.println("Data, Hora, T1 (C), H (%RH), T2 (C), P, Vcc (mV)");
    dataFile.close();
  }
}

void loop() {
  Serial.print(">> Calculando a partir de "); read_rtc(); Serial.println(); 
  calc_average();
  Serial.println();
  delay(1);
}

void rpm_fun()
{
    revolutions++;
}

float read_am2320() {
  float t, h;
  am2320.readTemperatureAndHumidity(&t, &h);
  return t, h;
}

float read_bmp180() {
  float t = bmp.readTemperature();
  float p = bmp.readPressure();
  return t, p;
}

float read_rtc() {
  DateTime now = rtc.now();
  Serial.print(now.timestamp(DateTime::TIMESTAMP_DATE));
  Serial.print("  ");
  Serial.print(now.timestamp(DateTime::TIMESTAMP_TIME));
  Serial.print("  ");
}

float ReadVcc() {
  // From https://gist.github.com/ProfJayHam/7792782
  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
  #if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
    ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
    ADMUX = _BV(MUX5) | _BV(MUX0);
  #elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
    ADMUX = _BV(MUX3) | _BV(MUX2);
  #else
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #endif  
 
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA,ADSC)); // measuring
 
  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH  
  uint8_t high = ADCH; // unlocks both
 
  long result = (high<<8) | low;
 
  result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  return result; // Vcc in millivolts
}

void writeDataToCSV(String data) {
  dataFile = SD.open("data.csv", FILE_WRITE);
  if (dataFile) {
    dataFile.println(data);
    dataFile.close();
  }
}

float calc_average() {
  int interval = 10;                      // Interval between measurements (sec)
  int period = 15*60;                        // Measuremment period (sec)
  float t1, h, t2, p, vcc;
  float t1_tot=0, h_tot=0, t2_tot=0, p_tot=0, vcc_tot=0;
  int n=0; 
  for(int i=0; i<period; i+=interval) {
    // Read data from each sensor
    am2320.readTemperatureAndHumidity(&t1, &h); t1_tot += float(t1); h_tot += float(h);
    t2 = bmp.readTemperature(); t2_tot += t2;
    p = bmp.readPressure() / 100; p_tot += p;          // hPa
    vcc = analogRead(0)*4.09/1023.0*ReadVcc(); vcc_tot += float(vcc);
    read_rtc();

    // calculate the revolutions per milli(second)
    rpmilli = ((float)revolutions)/(millis()-timeold);

    timeold = millis();
    revolutions = 0;
    // Calculando a velocidade (m/s)
    speed = rpmilli * 1000 * (2 * 3.1416 * 0.085);

    n++;
    delay(interval * 1000);             // millisecs
    Serial.print(n); Serial.print(" ");
    Serial.print("T1: "); Serial.print(t1); Serial.print(" C  ");
    Serial.print("H: "); Serial.print(h); Serial.print(" %  ");
    Serial.print("T2: "); Serial.print(t2); Serial.print(" C  ");
    Serial.print("P: "); Serial.print(p); Serial.print(" hPa  ");
    Serial.print("Vcc: "); Serial.print(vcc); Serial.print(" mV  ");
    Serial.print("Wnd (NC): "); Serial.print(speed); Serial.print(" m/s");
    Serial.println();

  }
  float t1_med = t1_tot/n; float h_med = h_tot/n;
  float t2_med = t2_tot/n; float p_med = p_tot/n; float vcc_med = vcc_tot/n;

  Serial.println("Media ate "); read_rtc(); Serial.println(":");
  Serial.print("T1: "); Serial.print(t1_med); Serial.print(" C  ");
  Serial.print("H: "); Serial.print(h_med); Serial.print(" %  ");
  Serial.print("T2: "); Serial.print(t2_med); Serial.print(" C  ");
  Serial.print("P: "); Serial.print((p_med)); Serial.print(" hPa  ");
  Serial.print("Vcc: "); Serial.print(vcc_med); Serial.print(" mV");


  DateTime now = rtc.now();
  String data = now.timestamp(DateTime::TIMESTAMP_DATE) + "," + now.timestamp(DateTime::TIMESTAMP_TIME)
  + "," + String(t1_med) + "," + String(h_med) + "," + String(t2_med)
  + "," + String(p_med) + "," + String(vcc_med);
  writeDataToCSV(data);

  return t1_med, h_med, t2_med, p_med, vcc_med;
}
#include "Adafruit_Sensor.h"
#include "Adafruit_AM2320.h"
#include <Adafruit_BMP085.h>

#include <GSMSimHTTP.h>

Adafruit_AM2320 am2320 = Adafruit_AM2320();
Adafruit_BMP085 bmp;

#define RESET_PIN 10 // you can use any pin.

// Configure HTTP GSM Sim
GSMSimHTTP http(Serial1, RESET_PIN);


// Variables
float t1, h, t2, p, p_inhg;

// Anemometer variables
volatile byte revolutions;
float rpmilli;
float speed;
unsigned long timeold=0;

void setup() {
  Serial1.begin(115200);
  delay(1000);
  Serial.begin(115200);
  delay(1000);
  pinMode(2, OUTPUT);

  digitalWrite(2, LOW);
  delay(100);
  digitalWrite(2, HIGH);
  delay(100);
  digitalWrite(2, LOW);
  delay(100);
  digitalWrite(2, HIGH);
  delay(100);

  Serial.println(">> Iniciando SIM ...");
  // Init module...
  http.init(); // use for init module. Use it if you dont have any valid reason.

  Serial.print("Set Phone Function... ");
  Serial.println(http.setPhoneFunc(1));
  delay(1000);
  Serial.println(http.isSimInserted());

  Serial.print("is Module Registered to Network?... ");
  Serial.println(http.isRegistered());
  delay(1000);

  Serial.print("Signal Quality... ");
  Serial.println(http.signalQuality());
  delay(1000);

  Serial.print("Operator Name... ");
  Serial.println(http.operatorNameFromSim());
  //delay(1000);

  //Serial.print("GPRS Init... ");
  //http.gprsInit("timbrasil.br", "tim", "tim"); 
  //delay(1000);

  Serial.print("Connect GPRS... ");
  Serial.println(http.connect());
  Serial.println(http.isConnected());
  //delay(1000);

  Serial.print("Get IP Address... ");
  Serial.println(http.getIP());
  delay(1000);
  Serial.println(">> SIM iniciado");

  Serial.println(">> Iniciando sensores...");
  am2320.begin();
  bmp.begin();
  attachInterrupt(digitalPinToInterrupt(2), rpm_fun, RISING);
  revolutions = 0;
  rpmilli = 0;
  timeold = 0;
  Serial.println(">> Sensores iniciados");

  digitalWrite(2, LOW);
  delay(100);
  digitalWrite(2, HIGH);
  delay(100);
  digitalWrite(2, LOW);
  delay(100);
  digitalWrite(2, HIGH);
  delay(100);
}

void loop() {
  digitalWrite(2, LOW);
  // Read data from each sensor
  am2320.readTemperatureAndHumidity(&t1, &h);
  t2 = bmp.readTemperature();
  p = bmp.readPressure() / 100;               // hPa
  p_inhg = (p)/ 33.86;

  // calculate the revolutions per milli(second)
  rpmilli = ((float)revolutions)/(millis()-timeold);

  timeold = millis();
  revolutions = 0;
  // Calculando a velocidade (m/s)
  speed = rpmilli * 1000 * (2 * 3.1416 * 0.085);
  // Aguardar 5 segundos entre as medidas
  delay(5 * 1000);
  Serial.print("T1: "); Serial.print(t1); Serial.print(" C  ");
  Serial.print("H: "); Serial.print(h); Serial.print(" %  ");
  Serial.print("T2: "); Serial.print(t2); Serial.print(" C  ");
  Serial.print("P: "); Serial.print(p); Serial.print(" hPa  ");
  Serial.print("Wnd (NC): "); Serial.print(speed); Serial.print(" m/s");
  Serial.println();

  float tf = (t1 * (9/5)) + 32;
  String tempf = String(tf); String humidity = String(h); String pressure = String(p_inhg);
  Serial.println(">> Sending data to server...");
  wu_update(tempf, humidity, pressure);
  Serial.println();
  digitalWrite(2, HIGH);
  delay(1000);
}

void rpm_fun()
{
    revolutions++;
}

void wu_update(String tempf, String humid, String pressure) {
  String cmd = "ID=";
  cmd += "ICAMPO409";
  cmd += "&PASSWORD=";
  cmd += "c9lXNQkd";
  cmd += "&tempf=";
  cmd += tempf;
  cmd += "&humidity=";
  cmd += humid;
  cmd += "&baromin=";
  cmd += pressure;
  cmd += "&dateutc=now&action=updateraw";
  delay(500);
  Serial.println(cmd);
  Serial.println(http.postWithSSL("https://rtupdate.wunderground.com/weatherstation/updateweatherstation.php", 
  cmd, "application/x-www-form-urlencoded", true));
}
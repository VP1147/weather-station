/* 
 *  ARDUINO Wireless Weather Station
 sends sensor data to wunderground.com
 
https://www.instructables.com/id/Arduino-Uno-Wireless-Weather-Station-Wundergroundc/

 last edit 2021 02 20 
 

1) www.cactus.io  Davis Anemometer  http://cactus.io/hookups/weather/anemometer/davis/hookup-arduino-to-davis-anemometer

2) Davis 6410 anemometer manual http://www.davisnet.com/product_documents/weather/spec_sheets/6410_SS.pdf

3) Adafruit BME280 Driver (Barometric Pressure Sensor) library  https://github.com/adafruit/Adafruit_BME280_Library

4) ML8511 UV Sensor Hookup Guide   https://learn.sparkfun.com/tutorials/ml8511-uv-sensor-hookup-guide

5) Arduino Library for Maxim Temperature Integrated Circuits 
    DS18B20
    DS18S20 - Please note there appears to be an issue with this series.
    DS1822
    DS1820
    MAX31820
 https://github.com/milesburton/Arduino-Temperature-Control-Library

6) Library for Dallas/Maxim 1-Wire Chips https://github.com/PaulStoffregen/OneWire

7) Wunderground  (Personal Weather Station Upload Protocol) http://wiki.wunderground.com/index.php/PWS_-_Upload_Protocol

 */

#include "TimerOne.h"          // https://github.com/PaulStoffregen/TimerOne  Timer Interrupt set to 2 second for read sensors
#include <math.h>
#include <Adafruit_Sensor.h>   // https://github.com/adafruit/Adafruit_Sensor
#include <Adafruit_BME280.h>   // https://github.com/adafruit/Adafruit_BME280_Library
#include <OneWire.h>           // https://github.com/PaulStoffregen/OneWire
#include <DallasTemperature.h> // https://github.com/milesburton/Arduino-Temperature-Control-Library
#include <SPI.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include <stdlib.h>




#define WindSensorPin (3)          // The pin location of the anemometer sensor
#define WindVanePin (A0)           // The pin the wind vane sensor is connected to
#define VaneOffset 0;              // define the anemometer offset from magnetic north
#define GREEN_LED 13               // indication led sending data to Wunderground 
Adafruit_BME280 bme ;              // I2C  BME280 sensor
#define BME280_ADDRESS (0x76)      // SDO conect to GND address 0x76 or  CSB +3.3V address 0x77  
#define RainSensorPin 2            // The pin rain REED-ILS sensor
#define ONE_WIRE_BUS 4             // The pin temperature  DS18B20 sensor
#define Bucket_Size 0.02           // rain bucket size inches  ( 0.5mm)

#define DEBUG 0 

volatile unsigned long tipCount1h;     // bucket tip counter used in interrupt routine
volatile unsigned long tipCount24h;    // bucket tip counter used in interrupt routine  
volatile unsigned long contactTime;    // Timer to manage any contact bounce in interrupt routine

                        
char server [] = "rtupdate.wunderground.com";        
char WEBPAGE [] = "GET /weatherstation/updateweatherstation.php?";
char ID [] = "XXXXXXK";        //wunderground weather station ID            
String PASSWORD  = "xxxxxx";   // wunderground weather station password     

int VaneValue;           // raw analog value from wind vane
int Direction;           // translated 0 - 360 direction
int CalDirection;        // converted value with offset applied
int LastValue;

int UVOUT = A3; //Output from the ML8511 sensor
int REF_3V3 = A2; //3.3V power on the Arduino board


float tempc;             // Temp celsius DS18B20
float tempc_min= 100;    // Minimum temperature C
float tempc_max;         // Maximum temperature C
float temp2c;            // Temp celsius  BMP280
float humidity;          // Humidity BMP280
float tempf=0;           // Temp farenheit BMP280
float dewptf=0;          // dew point tempf
float windSpeed;         // Wind speed (mph)
float wind_min = 100;     // Minimum wind speed (mph)
float wind_avg;          // 10 minutes average wind speed ( mph)
float windgustmph = 0;   // Maximum Wind gust speed( mph)
float barompa;           // Preasure pascal Pa
float baromin;           // Preasure inches of mercury InHg
float baromhpa;          // Preasure hectopascal hPa
float rain1h = 0;        // Rain inches over the past hour
float rain24h = 0;       // Rain inches over the past 24 hours
float uvIntensity = 0;   // UV 
float UVmax = 0;         // maximum UV intensity over the past 10 minutes
int solar  = 0;          // solarradiation - [W/m^2]
float altitudepws = 30.00;  //Local  altitude of the PWS to get relative pressure  meters (m)




volatile bool IsSampleRequired;       // this is set true every 2.5s. Get wind speed
volatile unsigned int  TimerCount;    // used to determine 2.5sec timer count
volatile unsigned long Rotations;     // cup rotation counter used in interrupt routine
volatile unsigned long ContactBounceTime;  // Timer to avoid contact bounce in interrupt routine

unsigned int counter = 0;
unsigned int raincount1h = 0;
unsigned int raincount24h = 0;

 

SoftwareSerial esp8266Module(10, 11); // RX, TX ESP8266

// Setup a oneWire instance to communicate with any OneWire devices 
// (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
 
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature DS18B20(&oneWire);
 

//******************************************************* MAIN PROGRAM START ************************************************

void setup() {
  
  LastValue = 0;
  IsSampleRequired = false;
  TimerCount = 0;
  Rotations = 0;   // Set Rotations to 0 ready for calculations
  tipCount1h = 0;
  tipCount24h = 0;
  
{
  Serial.begin(115200);
  esp8266Module.begin(115200);
  esp8266Module.println("AT+RST");
  pinMode(GREEN_LED,OUTPUT);
  pinMode(RainSensorPin, INPUT);
  pinMode(UVOUT, INPUT);
  pinMode(REF_3V3, INPUT);
  bme.begin(0x76);
  DS18B20.begin();
  delay(1000);
  
}
  pinMode(WindSensorPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(WindSensorPin), isr_rotation, FALLING);
  attachInterrupt(digitalPinToInterrupt(RainSensorPin), isr_rg, FALLING);
  sei();// Enable Interrupts
  Serial.print("Start Weather Station: ");
  Serial.println(ID);
  delay(3000);
  // Setup the timer intterupt
  Timer1.initialize(500000); // Timer interrupt every 2.5 seconds
  Timer1.attachInterrupt(isr_timer);

}


void loop()
{
 read_data();             // read different sensors data from analog and digital pins of arduino
 getWindSpeed();
 isr_timer();
 isr_rotation();
 getWindDirection();
 getRain();
 if(DEBUG){
 print_data();}            // print in serial monitor
 wunderground();          // sends data to wunderground


}
//-------------------------------------------------------------------------------------------------------------
////////////////////////////////////READ THE DATA FROM SENSORS/////////////////////////////////////////////////
//-------------------------------------------------------------------------------------------------------------
 void read_data(void)
 {
    DS18B20.requestTemperatures(); // Send the command to get temperatures from DS18B20 sensor
     
     tempc = DS18B20.getTempCByIndex(0);

    if (tempc > tempc_max) {
     tempc_max = tempc;
}
    if (tempc_min > tempc ) {
     tempc_min = tempc;
}
    tempc = (tempc_min + tempc_max)*0.5;  // average wind temperature C per 10 minutes   

     
     temp2c = bme.readTemperature();
     humidity = bme.readHumidity();
    
     tempf =  (tempc * 9.0)/ 5.0 + 32.0;   
     dewptf = (dewPoint(tempf, bme.readHumidity()));
     
     barompa = bme.readPressure();
     baromhpa = (barompa / 100)+(altitudepws * 0.12);  
     baromin = (baromhpa)/ 33.86;
     
     int uvLevel = averageAnalogRead(UVOUT);
     int refLevel = averageAnalogRead(REF_3V3);
     //Use the 3.3V power pin as a reference to get a very accurate output value from sensor
     float outputVoltage = 3.3 / refLevel * uvLevel;
     uvIntensity = mapfloat(outputVoltage, 0.99, 2.8, 0.0, 15.0);
     if  ( uvIntensity < 0.4 ){ 
      uvIntensity = 0; 
      }
     solar = uvIntensity * 100;
     if (uvIntensity > UVmax) {
     UVmax = uvIntensity; 
     }
     UV_convert();
  }

//-------------------------------------------------------------------------------------------------------------
////////////////////////////////////Get wind speed  /////////////////////////////////////////////////////////
//-------------------------------------------------------------------------------------------------------------
void getWindSpeed(void)
  {
  // Only update the display if change greater than 45 degrees. 
  if(abs(CalDirection - LastValue) > 5)
  { 
     LastValue = CalDirection;
  }
  if(IsSampleRequired)
  {
     Rotations = 0; // Set Rotations count to 0 ready for calculations
     //sei(); // Enables interrupts
     delay (25000); // Wait 25 seconds to average wind speed
     //cli(); // Disable interrupts
     
     /* convert to mp/h using the formula V=P(2.25/T)
      V = P(2.25/25) = P * 0.9       V - speed in mph,  P - pulses per sample period, T - sample period in seconds */
     windSpeed = Rotations * 0.18; // 0.18
     Rotations = 0;   // Reset count for next sample
     
     IsSampleRequired = false; 
  }
 if (windSpeed > windgustmph) {
     windgustmph = windSpeed;
}
 if (wind_min > windSpeed ) {
     wind_min = windSpeed;
}

 wind_avg = (windgustmph + wind_min) * 0.5;   // average wind speed mph per 10 minutes   
 
}
//-------------------------------------------------------------------------------------------------------------
////////////////////////////////////ISR timer//////////////////////////////////////////////////////////////////
//-------------------------------------------------------------------------------------------------------------
// isr routine fr timer interrupt
void isr_timer(void) 
{
  
  TimerCount++;
  
  if(TimerCount == 5)
  {
    IsSampleRequired = true;
    TimerCount = 0;
  }
}
// This is the function that the interrupt calls to increment the rotation count
//-------------------------------------------------------------------------------------------------------------
////////////////////////////////////ISR rotation//////////////////////////////////////////////////////////////
//-------------------------------------------------------------------------------------------------------------
void isr_rotation(void)   
{
  if ((millis() - ContactBounceTime) > 15 ) {  // debounce the switch contact.
    Rotations++;
    ContactBounceTime = millis();
  }
}
// Convert MPH to Knots
float getKnots(float speed) {
   return speed * 0.868976;          //knots 0.868976;
}
// Convert MPH to m/s
float getms(float speed) {
   return speed * 0.44704;           //metric m/s 0.44704;;
}
// Get Wind Direction
//-------------------------------------------------------------------------------------------------------------
/////////////////////////////////// Wind direction ////////////////////////////////////////////////////////////
//-------------------------------------------------------------------------------------------------------------
void getWindDirection(void) 
{
   VaneValue = analogRead(WindVanePin);
   Direction = map(VaneValue, 0, 1023, 0, 360);
   CalDirection = Direction + VaneOffset;
   
   if(CalDirection > 360)
     CalDirection = CalDirection - 360;
     
   if(CalDirection < 0)
     CalDirection = CalDirection + 360;
}

// Converts compass direction to heading
void getHeading(int direction) {
    if(direction < 22)
      Serial.print("N");
    else if (direction < 67)
      Serial.print(" NE");
    else if (direction < 112)
      Serial.print("E");
    else if (direction < 157)
      Serial.print(" SE");
    else if (direction < 202)
      Serial.print(" S");
    else if (direction < 247)
      Serial.print(" SW");
    else if (direction < 292)
      Serial.print(" W");
    else if (direction < 337)
      Serial.print(" NW");
    else
      Serial.print(" N");  
}

// converts wind speed to wind strength
void getWindStrength(float speed)
{
  if(speed < 1)
    Serial.println("Calm");
  else if(speed >= 1 && speed < 3)
    Serial.println("Light Air");
  else if(speed >= 3 && speed < 7)
    Serial.println("Light Breeze");
  else if(speed >= 7 && speed < 12)
    Serial.println("Gentle Breeze");
  else if(speed >= 12 && speed < 18)
    Serial.println("Moderate Breeze");
  else if(speed >= 18 && speed < 24)
    Serial.println("Fresh Breeze");
  else if(speed >= 24 && speed < 31)
    Serial.println("Strong Breeze");
  else if(speed >= 31 && speed < 38)
    Serial.println("High wind");
  else if(speed >= 38 && speed < 46)
    Serial.println("Fresh Gale");
  else if(speed >= 46 && speed < 54)
    Serial.println("Strong Gale");
  else if(speed >= 54 && speed < 63)
    Serial.println("Storm");
  else if(speed >= 63 && speed < 72)
    Serial.println("Violent storm"); 
  else if(speed >= 72 && speed)
    Serial.println("Hurricane");    
}

//------------------------------------------------------------------------------------------------------------
///////////////////////////////// Get Rain data //////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------------------------------------

  void getRain(void)
   {
// we only display the tip count when it has been incremented by the sensor
   cli();         //Disable interrupts
  
      rain1h = tipCount1h * Bucket_Size;
      rain24h = tipCount24h * Bucket_Size;
  
      if (raincount1h >= 120)   
      { 
// Im using 120 count for the 30 second loop time wunderground goes by 1 hour intervals (30 seconds x 120)  = 3600 seconds / 60 = 60 minutes = 1h

    tipCount1h = 0;
    raincount1h = 0;
}
 if (raincount24h >= 2879) 
      {    
    tipCount24h = 0;
    raincount24h = 0;
}
   sei();         //Enables interrupts
   raincount1h++;
   raincount24h++;
}


// Interrrupt handler routine that is triggered when the W174 detects rain   
void isr_rg() {

  if((millis() - contactTime) > 15 ) { // debounce of sensor signal 
    tipCount1h++;
    tipCount24h++;
    contactTime = millis(); 
  } 
} 

//------------------------------------------------------------------------------------------------------------
/////////////////////////////////PRINT DATA IN SERIAL MONITOR/////////////////////////////////////////////////
//------------------------------------------------------------------------------------------------------------
  void print_data(void) 
 {
    delay(100);
    Serial.println(" ");

    Serial.print("DS18B20 = ");
    Serial.print(tempc);
    Serial.println(" C  ");

    Serial.print(F("BME280 = "));
    Serial.print(temp2c);
    Serial.print(" C  ");
    Serial.print(tempf); 
    Serial.print(" F  ");
    Serial.print(humidity); 
    Serial.print(" %  ");
    Serial.print(baromhpa);
    Serial.print(" hPa  ");
    Serial.print(baromin);
    Serial.println(" inHg  ");
    //Serial.print(F("BME280 Approx altitude = "));
    // Serial.print(bme.readAltitude(997.6)); // this should be adjusted to your local forcase http://www.mide.com/pages/air-pressure-at-altitude-calculator
    // Serial.println(" m");
    
    Serial.print("ML8511 = ");
    Serial.print("Solar = "); Serial.print(solar); Serial.print(" W/m^2  ");  
    Serial.print("  UV = ");  Serial.println(uvIntensity);
    


    Serial.print("WindSpeed = ");
    Serial.print(windSpeed);
    Serial.print(" mph  ");
    Serial.print(getKnots(windSpeed));
    Serial.print(" knots  ");
    Serial.print(getms(windSpeed));
    Serial.println(" m/s  ");


    Serial.print("Wind_min = ");  //Minimum wind speed
    Serial.print(wind_min);
    Serial.print(" mph ");
    Serial.print("WindAvg = "); //Average wind speed
    Serial.print(wind_avg);
    Serial.print(" mph ");
    Serial.print("WindGust = "); //Maximum wind gust speed
    Serial.print(windgustmph);
    Serial.println(" mph ");




    Serial.print("Wind_Direction = ");   
    getHeading(CalDirection);
    Serial.println(" ");  
    Serial.print("WindStrength = ");
    getWindStrength(windSpeed);
  
    Serial.print("Rain Tip Count = ");
    Serial.println(tipCount24h);
    Serial.print("Precip. Rate = ");
    Serial.print(rain1h); Serial.println(" in");
    Serial.print("Precip Accum. Total: ");
    Serial.print(rain24h); Serial.println(" in");
    
 
    
 }

//-------------------------------------------------------------------
// Following function sends sensor data to wunderground.com
//-------------------------------------------------------------------

void wunderground(void)
{
 String cmd1 = "AT+CIPSTART=\"TCP\",\"";
  cmd1 += "rtupdate.wunderground.com"; // wunderground
  cmd1 += "\",80";
  esp8266Module.println(cmd1);

  if(esp8266Module.find("Error")){
    Serial.println("AT+CIPSTART error");
    return;
  }
  
  String cmd = "GET /weatherstation/updateweatherstation.php?ID=";
  cmd += ID;
  cmd += "&PASSWORD=";
  cmd += PASSWORD;
  cmd += "&dateutc=now&winddir=";
  cmd += CalDirection;
  cmd += "&windspeedmph=";
  cmd += wind_avg;    
  cmd += "&windgustmph=";
  cmd += windgustmph;
  cmd += "&tempf=";
  cmd += tempf;
  cmd += "&dewptf=";
  cmd += dewptf;
  cmd += "&humidity=";
  cmd += humidity;
  cmd += "&baromin=";
  cmd += baromin;
  cmd += "&solarradiation=";
  cmd += solar;
  cmd += "&UV=";
  cmd += UVmax;
  cmd += "&rainin=";
  cmd += rain1h;
  cmd += "&dailyrainin=";
  cmd += rain24h;
  cmd += "&softwaretype=Arduino-ESP8266&action=updateraw&realtime=1&rtfreq=30";   // &softwaretype=Arduino%20UNO%20version1
  cmd += "/ HTTP/1.1\r\nHost: rtupdate.wunderground.com:80\r\nConnection: close\r\n\r\n";

  cmd1 = "AT+CIPSEND=";
  cmd1 += String(cmd.length());
  esp8266Module.println(cmd1);
  if(esp8266Module.find(">")){
    esp8266Module.print(cmd);
    digitalWrite(GREEN_LED,HIGH);  // Green led is turn on then sending data to Wunderground
    Serial.println("Data send OK");
    delay(1000);
    digitalWrite(GREEN_LED,LOW);
      }
  else{
    esp8266Module.println("AT+CIPCLOSE");
    Serial.println("Connection closed");
    Serial.println(" ");  
  }
  delay(2500);
  Counter();
}


//------------------------------------------------------------------------------------------------------------
/////////////////////////////////////////////// Counter /////////////////////////////////////////////////////
//------------------------------------------------------------------------------------------------------------

void Counter()
{
if (counter >= 21){

windgustmph = 0;
wind_avg = 0;
wind_min = 100;
tempc_min = 100;
tempc_max = 0;
UVmax = 0;
counter = 0;
}
counter++;

}


//------------------------------------------------------------------------------------------------------------
///////////////////////////////// Dew point calculation//////////////////////////////////////////////////////
//------------------------------------------------------------------------------------------------------------

double dewPoint(double tempf, double humidity)
{
        double RATIO = 373.15 / (273.15 + tempf);  // RATIO was originally named A0, possibly confusing in Arduino context
        double SUM = -7.90298 * (RATIO - 1);
        SUM += 5.02808 * log10(RATIO);
        SUM += -1.3816e-7 * (pow(10, (11.344 * (1 - 1/RATIO ))) - 1) ;
        SUM += 8.1328e-3 * (pow(10, (-3.49149 * (RATIO - 1))) - 1) ;
        SUM += log10(1013.246);
        double VP = pow(10, SUM - 3) * humidity;
        double T = log(VP/0.61078);   // temp var
        return (241.88 * T) / (17.558 - T);
}

//Takes an average of readings on a given pin
//Returns the average
int averageAnalogRead(int pinToRead)
{
  byte numberOfReadings = 8;
  unsigned int runningValue = 0; 

  for(int x = 0 ; x < numberOfReadings ; x++)
    runningValue += analogRead(pinToRead);
  runningValue /= numberOfReadings;

  return(runningValue);  
}

//The Arduino Map function but for floats
//From: http://forum.arduino.cc/index.php?topic=3922.0
float mapfloat(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}



//------------------------------------------------------------------------------------------------------------
/////////////////////////////////////////// UV_convert //////////////////////////////////////////////////////
//------------------------------------------------------------------------------------------------------------
void UV_convert()

{
  if(UVmax < 0.9)
   UVmax = 0;
  else if(UVmax >= 0.9 && UVmax < 1.75)
   UVmax = 1;  
  else if(UVmax >= 1.75 && UVmax < 2.4)
   UVmax = 2;
  else if(UVmax >= 2.4 && UVmax < 3.4)
   UVmax = 3;
  else if(UVmax >= 3.4 && UVmax < 4.4)
   UVmax = 4;
  else if(UVmax >= 4.4 && UVmax < 5.4)
   UVmax = 5;
  else if(UVmax >= 5.4 && UVmax < 6.4)
   UVmax = 6;
  else if(UVmax >= 6.4 && UVmax < 7.4)
   UVmax = 7;
  else if(UVmax >= 7.4 && UVmax < 8.4)
   UVmax = 8;
  else if(UVmax >= 8.4 && UVmax < 9.4)
   UVmax = 9;
  else if(UVmax >= 9.4 && UVmax < 10.4)
   UVmax = 10;
  else if(UVmax >= 10.4 && UVmax < 11.4)
   UVmax = 11;
  else if(UVmax >= 11.4 && UVmax < 12.4)
   UVmax = 12;
  else if(UVmax >= 12.4 && UVmax < 13.4)
   UVmax = 13;
  else if(UVmax >= 13.4 && UVmax < 14.4)
   UVmax = 14;
  else if(UVmax >= 14.4 )
   UVmax = 15;
}

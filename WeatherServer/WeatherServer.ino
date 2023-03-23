// Import required libraries

//#include <ESPAsyncWebServer.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

#define LM32 A0

String ID = "ICAMPO246";          // wunderground weather station ID            
String PASSWORD  = "ep99Fx3Q";    // wunderground weather station password   
String wu_sha1 = "E0:2A:E3:39:87:EB:E5:C6:AD:34:D5:08:1C:5F:12:5C:5E:4D:2E:26";

// Variables
String temp = "";
String tempf = "";
String humid = "";
String hic = "";

// Timer variables
unsigned long lastTime = 0;  
unsigned long timerDelay = 2000;

// Replace with your network credentials
const char* ssid = "VIPERNET_ROSALI";
const char* password = "@db10buster@";


// Create AsyncWebServer object on port 80
//AsyncWebServer server(80);

HTTPClient http;
WiFiClient client;

/*
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <style>
    html {
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
    h2 { font-size: 3.0rem; }
    p { font-size: 3.0rem; }
    .units { font-size: 1.2rem; }
    .ds-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 15px;
    }
  </style>
</head>
<body>
  <h2>Nano Weather Station</h2>
  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 
    <span class="ds-labels">Temperature: </span> 
    <span id="temp">%TEMP%</span>
    <sup class="units">&deg;C</sup>
  </p>
  <p>
    <i class="fas fa-cloud-rain" style="color:#059e8a;"></i> 
    <span class="ds-labels">Humidity: </span> 
    <span id="humid">%HUMID%</span>
    <sup class="units">&percnt;</sup>
  </p>
  <p>
    <i class="fas fa-thermometer" style="color:#059e8a;"></i> 
    <span class="ds-labels">Heat Index: </span> 
    <span id="hic">%HIC%</span>
    <sup class="units">&deg;C</sup>
  </p>
</body>
<script>
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temp").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temp", true);
  xhttp.send();
  }, 2000) ;
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("humid").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/humid", true);
  xhttp.send();
  
  }, 2000) ;
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("hic").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/hic", true);
  xhttp.send();
}, 2000) ;
</script>
</html>)rawliteral";

// Replaces placeholder with DHT values
String processor(const String& var){
  if(var == "TEMP"){
    return temp.c_str();
  }
  else if(var == "HUMID"){
    return humid.c_str();
  }
  else if(var == "HIC"){
    return hic.c_str();
  }
  return String();
}
*/
void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);
  pinMode(D5, OUTPUT);
  digitalWrite(D5, LOW);

  Serial.print("Start Weather Station: ");
  Serial.println(ID);
  
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);

  /* // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/temp", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", temp.c_str());
  });
  server.on("/humid", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", humid.c_str());
  });
  server.on("/hic", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", hic.c_str());
  });
  // Start server
  server.begin();
  */
}

void wunderground(String tempf) {
  String cmd = "http://rtupdate.wunderground.com/weatherstation/updateweatherstation.php?ID=";
  cmd += ID;
  cmd += "&PASSWORD=";
  cmd += PASSWORD;
  cmd += "&tempf=";
  cmd += tempf;
  //cmd += "&humidity=";
  //cmd += humid;
  cmd += "&dateutc=now&action=updateraw";
  Serial.println(cmd);
  http.begin(client, cmd);
  int httpCode = http.GET();
  if(httpCode > 0) {
    Serial.println(httpCode);
    if(httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      digitalWrite(D5, HIGH); delay(100); digitalWrite(D5, LOW);
    }
  }
    else {
      Serial.println(http.errorToString(httpCode));
      }
  http.end();
}

float ReadLM32(int port) {
  float tension = (float(analogRead(port)) * 5) / 1023;
  float temp = tension / 0.010; // 10 mV
  return temp;
}
void loop(){
  if ((millis() - lastTime) > timerDelay) {
    float t = ReadLM32(LM32);
    float tf = (t * (9/5)) + 32;
    //float h = 0;
    temp = t; tempf = tf; //humid = h;
    wunderground(tempf);
    lastTime = millis();
  }  
}

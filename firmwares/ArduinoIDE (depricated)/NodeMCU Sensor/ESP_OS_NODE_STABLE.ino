//Includes
#include "DHT.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiClient.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>


//Constant
#define LIGHTPIN A0
#define DHTPIN 4
#define DHTTYPE DHT11
#define LED_BUILTIN 16
// #define BATT_PIN 0


//Variables
String ssid   = "";
String pasw   = "";
String hwId   = "";
String url    = "http://dev.steelants.cz/vasek/home/api.php                      ";

//Settings
long sleepTime = 4;  //in minutes
String apSSID = "Node Meter";

//Aliasses
#if defined(DHTPIN)
  DHT dht(DHTPIN, DHTTYPE);
#endif
ESP8266WebServer webServer(80);

void setup() {
  Serial.begin(9600);
  EEPROM.begin(512);
  delay(10);
  Serial.println();
  Serial.println();

  #if defined(LIGHTPIN)
    pinMode(LIGHTPIN, INPUT);
  #endif
  #if defined(BATT_PIN)
    pinMode(BATT_PIN, INPUT);
  #endif
  
  Serial.println("HW: " + String(hwId));
  sleepTime = sleepTime * 60000;
  dht.begin();
}

void loop() {
  // WI-FI CONECTING
  Serial.println("WIFI SSID: " + String(ssid));
  Serial.println("SLEEP INTERVAL: "+String(sleepTime));
  
  WiFi.begin(ssid, pasw);
  checkConnection();

  Serial.println("CONECTED TO WIFI");
  Serial.println("IP: " + WiFi.localIP().toString());
  
  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "text/plain");  //Specify content-type header
  
  StaticJsonDocument<1024> sendContent;
  sendContent["token"] = hwId;
  
  #if defined(DHTPIN)
    sendContent["values"]["temp"]["value"] = getTemperature();
    sendContent["values"]["temp"]["unit"] = "C";
    sendContent["values"]["humi"]["value"] = getHumidity();
    sendContent["values"]["humi"]["unit"] = "%";
  #endif
  
  #if defined(LIGHTPIN)
    sendContent["values"]["light"]["value"] = getLight();
    sendContent["values"]["light"]["unit"] = "";
  #endif
  
  #if defined(BATT_PIN)
    sendContent["values"]["battery"]["value"] = getBattery();
    sendContent["values"]["battery"]["unit"] = "";
  #endif
  
  String requestJson = "";
  serializeJson(sendContent, requestJson);
  Serial.println("JSON: " + requestJson);
  
  int httpCode = http.POST(requestJson);
  String payload = http.getString();  //Get the response payload
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, payload);
  
  String hostname = doc["device"]["hostname"];
  sleepTime = doc["device"]["sleepTime"];
  WiFi.hostname(hostname);
  
  Serial.println("HTTP CODE: " + String(httpCode) + ""); //Print HTTP return code
  Serial.println("HTTP BODY: " + String(payload) + "");  //Print request response payload
  http.end();  //Close connection
  Serial.println("DISCONECTED FROM WIFI");
  WiFi.disconnect();
  
  Serial.println("GOING TO SLEEP FOR " + String(sleepTime));
  ESP.deepSleep((sleepTime * 60) * 1000000, RF_DEFAULT);  // 20e6 is 20 microseconds
  delay(1000);
}

#if defined(DHTPIN)
  float getTemperature() {
    float t = dht.readTemperature();
    if (isnan(t)) {
      Serial.println( "Failed to read temperature from sensor!") ;
      return 999;
    }
    return t;
  }
  
  float getHumidity() {
    float h = dht.readHumidity();
    if (isnan(h)) {
      Serial.println("Failed to read humidity from sensor!");
      return 999;
    }
    return h;
  }
#endif

#if defined(LIGHTPIN)
  int getLight() {
    int l = analogRead(LIGHTPIN);
    return l;
    if (l > 1000) {
      return 1;
    } else {
      return 0;
    }
    Serial.println("Failed to read light from sensor!");
    return 999;
  }
#endif

#if defined(BATT_PIN)
  float getBattery() {
    float l = analogRead(BATT_PIN);
    float volts = 0;
    
    int sampleNumper = 100;
    for (int x = 0; x <= sampleNumper; x++) {
      volts = volts + (((l * 3.22265625) *2) /1000);
    }
    return volts/100;
  }
#endif
  

bool checkConnection() {
  int count = 0;
  Serial.print("Waiting for Wi-Fi connection");
  while ( count < 30 ) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println();
      Serial.println("Connected!");
      return (true);
    }
    delay(500);
    Serial.print(".");
    count++;
  }
  Serial.println("Timed out.");
  return false;
}

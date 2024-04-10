#include <stdio.h>
#include "MPU9250.h"
#include "Math.h"
#include "BluetoothSerial.h"
#include <TinyGPS++.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>



#define GPS_BAUDRATE 9600  // The default baudrate of NEO-6M is 9600
#define time_offset 7200  // define a clock offset of 7200 seconds (2 hour) ==> UTC + 2

JsonDocument doc;
TinyGPSPlus gps;  // the TinyGPS++ object
String output;
MPU9250 IMU(Wire,0x68);


int status;
const int ledPin = 32;

const char* ssid = "MERCUSYS_49A8";  // Enter SSID here
const char* password = "27042003";  // Enter WiFi password here
const char* serverName = "http://192.168.1.160:8000/esp_data/";  // Server address
unsigned long lastTime = 0;
// Timer set to 10 minutes (600000)
//unsigned long timerDelay = 600000;
// Set timer to 5 seconds (5000)
unsigned long timerDelay = 5000;

uint64_t chipid;


void setup() {
  chipid = ESP.getEfuseMac(); // The chip ID is essentially its MAC address(length: 6 bytes).
  doc["id"] = chipid;
  pinMode (ledPin, OUTPUT);
  Serial.begin(9600);
  Serial2.begin(GPS_BAUDRATE);

  Serial.println(F("ESP32 - GPS module"));
  gps.encode(Serial2.read());
  if(!gps.location.isValid()){
      Serial.print(F("Connecting"));
      while(!gps.location.isValid()){
      Serial.print(F("."));
      digitalWrite (ledPin, HIGH);  // turn on the LED
      delay(100);
      digitalWrite (ledPin, LOW);  // turn on the LED

      gps.encode(Serial2.read());

      delay(100);
    }
    
  }

  while(!Serial) {}
  status = IMU.begin();
  if (status < 0) {
    
    Serial.println("IMU initialization unsuccessful");
    Serial.println("Check IMU wiring or try cycling power");
    Serial.println("Status: ");
    Serial.println(status);
    while(1) {}
  }
 

}
void send_data(){
  if(WiFi.status()== WL_CONNECTED){
      WiFiClient client;
      HTTPClient http;
    
      // Your Domain name with URL path or IP address with path
      http.begin(client, serverName);
      
      
      
      // If you need an HTTP request with a content type: application/json, use the following:
      http.addHeader("Content-Type", "application/json");
       serializeJson(doc, output);
      Serial.println(output);
      int httpResponseCode = http.POST(output);

      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
        
      http.end();
    }
    else {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();
  
}
void connect_to_wifi(){
   WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
 
  Serial.println("Timer set to 5 seconds (timerDelay variable), it will take 5 seconds before publishing the first reading.");
}
void disconnect_from_wifi() {
  WiFi.disconnect();
  Serial.println("Disconnected from WiFi network");
}

int get_acc() {
  float accX = IMU.getAccelX_mss();
  float accY = IMU.getAccelY_mss();
  float accZ = IMU.getAccelZ_mss();
  // Serial.print("accX:");
  // Serial.println(accX);
  // Serial.print("accY:");
  // Serial.println(accY);
  // Serial.print("accZ:");
  // Serial.println(accZ);
  int acc = (int)abs(9.9-sqrt(accX*accX + accY*accY + accZ*accZ));
  return acc;
}
void get_gps(){
    if (Serial2.available() > 0) {
    if (gps.encode(Serial2.read())) {
      if (gps.location.isValid()) {
        Serial.print(F("- latitude: "));
        Serial.println(gps.location.lat(), 7);

        Serial.print(F("- longitude: "));
        Serial.println(gps.location.lng(), 7);

        JsonObject position = doc["position"].to<JsonObject>();
        position["latitude"] = gps.location.lat();
        position["longitude"] = gps.location.lng();

        // Serial.print(F("- altitude: "));
        // if (gps.altitude.isValid())
        //   Serial.println(gps.altitude.meters(), 7);
        // else
        //   Serial.println(F("INVALID"));
      } 
      else {
        Serial.println(F("- location: INVALID"));
        JsonObject position = doc["position"].to<JsonObject>();
        position["latitude"] = 0;
        position["longitude"] = 0;
      }

      // Serial.print(F("- speed: "));
      // if (gps.speed.isValid()) {
      //   Serial.print(gps.speed.kmph());
      //   Serial.println(F(" km/h"));
      // } else {
      //   Serial.println(F("INVALID"));
      // }

      Serial.print(F("- GPS date&time: "));
      if (gps.date.isValid() && gps.time.isValid()) {

        JsonObject date = doc["date"].to<JsonObject>();
        date["year"] = gps.date.year();
        date["month"] = gps.date.month();
        date["day"] = gps.date.day();
        Serial.print(gps.date.year());
        Serial.print(F("-"));
        Serial.print(gps.date.month());
        Serial.print(F("-"));
        Serial.print(gps.date.day());
        Serial.print(F(" "));

        JsonObject time = doc["time"].to<JsonObject>();
        time["hour"] = gps.time.hour()+3;
        time["minute"] = gps.time.minute();
        time["second"] = gps.time.second();

        Serial.print(gps.time.hour()+3);
        Serial.print(F(":"));
        Serial.print(gps.time.minute());
        Serial.print(F(":"));
        Serial.println(gps.time.second());
      } else {
        Serial.println(F("INVALID"));

          JsonObject date = doc["date"].to<JsonObject>();
        date["year"] = 0;
        date["month"] = 0;
        date["day"] = 0;
        JsonObject time = doc["time"].to<JsonObject>();
        time["hour"] = 0;
        time["minute"] = 0;
        time["second"] = 0;
      }

      Serial.println();
    }
  }

  if (millis() > 5000 && gps.charsProcessed() < 10)
    Serial.println(F("No GPS data received: check wiring"));
}


void loop() {
IMU.readSensor();
  
  int acc = get_acc();

  // Serial.print("Variable_2:");
  // Serial.println(acc);
  get_gps();
  serializeJson(doc, output);
  //Serial.println(output);



  if(acc>=20){
    digitalWrite (ledPin, HIGH);  // turn on the LED
    connect_to_wifi();
    send_data();
    delay(1000);
    disconnect_from_wifi();
    
    // Serial.print('1');
    // Serial.print('/n');
    // delay(20000);      
  }
  else{

    // Serial.print('0');
    // Serial.print('\n');
  }
  //delay(10);
}

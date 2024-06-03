#include <WiFi.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <cmath>

String SSID = "";
String PW = "";

String NOWTem = "25.0";
String NOWHUM = "47.3";


void setup() {
  Serial.begin(115200);     //시리얼 통신 속도 설정


  WIFI_connect();      
}

void loop() {
  float temp = round(NOWTem.toFloat()*10)/10;
  float hum = round(NOWHUM.toFloat()*10)/10;

  Serial.println(temp);
  Serial.println(hum);

  // Prepare JSON document
  DynamicJsonDocument doc(2048);
  doc["temperature"] = NOWTem;
  doc["humidity"] = NOWHUM;
  doc["cageId"] = "c8487f39-b222-477a-955c-60e15be3ea6d"; 
  // Serialize JSON document
  String json;
  serializeJson(doc, json);

  WiFiClient client;  // or WiFiClientSecure for HTTPS
  HTTPClient http;

  // Send request
  http.begin(client, "http://httpbin.org/post");
  http.addHeader("Content-Type", "application/json");
  http.POST(json);
  //
  // Read response
  Serial.print(http.getString());

  // Disconnect
  http.end();
  
  delay(30000);
}

void WIFI_connect() {
  Serial.println("[SETUP] WIFI SETUP START");

  while ((SSID == "") || (PW == "") || (WiFi.status() != WL_CONNECTED))
  {
    WiFi.begin(SSID, PW);
    delay(1000);
  }
  Serial.println("wifi success!");
}

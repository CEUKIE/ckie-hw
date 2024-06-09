#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <cmath>

String SSID = "dlink1234";
String PW = "14159265";

const char* rootCACertificate = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\n" \
"ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\n" \
"b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\n" \
"MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\n" \
"b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\n" \
"ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\n" \
"9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\n" \
"IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\n" \
"VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\n" \
"93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\n" \
"jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\n" \
"AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\n" \
"A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI\n" \
"U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs\n" \
"N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\n" \
"o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU\n" \
"5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy\n" \
"rqXRfboQnoZsG4q5WTP468SQvvG5\n"\
"-----END CERTIFICATE-----\n";


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

  WiFiClientSecure *client = new WiFiClientSecure;  // or WiFiClientSecure for HTTPS
  if (client)
  {
    client->setCACert(rootCACertificate);
    
    HTTPClient https;

    // Send request
    https.begin(*client, "https://httpbin.org/post");
    https.addHeader("Content-Type", "application/json");
    https.POST(json);
    //
    // Read response
    Serial.print(https.getString());

    // Disconnect
    https.end();

    delay(100000);
  }
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

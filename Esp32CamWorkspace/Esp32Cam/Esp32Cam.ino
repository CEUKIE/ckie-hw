#include <Arduino.h>
#include <BluetoothSerial.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <SoftwareSerial.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WebSocketsClient.h>
#include <SocketIOclient.h>

#define RX 13
#define TX 14
#define SERVICE_UUID "c672da8f-05c6-472f-87d8-34201a97468f"
#define CHARACTERISTIC_READ "01e7eeab-2597-4c54-84e8-2fceb73c645d"
#define CHARACTERISTIC_WRITE "5a9edc71-80cb-4159-b2e6-a2913b761026"


SoftwareSerial Serial_soft(RX, TX);

String UID = "", bluetooth_data="", wifi_id = "", wifi_pw = "14159265", 
          MAXHum = "40.0", MINHum = "35.0", MAXTem = "29.7", MINTem = "20.8", NOWHUM = "12.0", NOWTem = "33.3";

SocketIOclient socketIO;

// to-do
// 테스트용 임시 온습도 설송
// 카메라 전송

// *************** bluetooth *************** 
unsigned int CAGE_NUM = 1;

bool deviceConnected = false;
BLECharacteristic *pTxCharacteristic;
BLECharacteristic *pRxCharacteristic;

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    Serial.println("[BLE] Bluetooth connected");
    deviceConnected = true;
  }

  void onDisconnect(BLEServer* pServer) {
    Serial.println("[BLE] Bluetooth disconnected");
    deviceConnected = false;
    pServer->startAdvertising();
  }
};

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    String rxValue = pCharacteristic->getValue().c_str();
    if (rxValue.length() > 0) {
      bluetooth_data = rxValue;
      if(bluetooth_data.startsWith("wi ")){
        wifi_id = bluetooth_data.substring(3);
        Serial.println("[BLE] WIFI ID = " + wifi_id);
      }
      else if(bluetooth_data.startsWith("wp ")){
        wifi_pw = bluetooth_data.substring(3);
        Serial.println("[BLE] WIFI PW = " + wifi_pw);
      }
    }
  }
};

void BT_setup() {
  Serial.println("[SETUP] BLUETOOTH: " + String(CAGE_NUM) + ".NO HELMET BLUETOOTH SETUP START");
  String bluetooth_name = "Esp32s3-cam";

  BLEDevice::init(bluetooth_name.c_str());
  
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);
  pTxCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_READ,
                        BLECharacteristic::PROPERTY_NOTIFY
                      );
  pTxCharacteristic->addDescriptor(new BLE2902());

  pRxCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_WRITE,
                        BLECharacteristic::PROPERTY_WRITE
                      );
  pRxCharacteristic->setCallbacks(new MyCallbacks());

  pService->start();
  pServer->getAdvertising()->start();

  // Wait for user ID
  while (!deviceConnected) {
    delay(1000);
  }

  Serial.println("[SETUP] BLUETOOTH: " + String(CAGE_NUM) + ".NO CAGE BLUETOOTH SETUP SUCCESS");
}


// *************** WIFI *************** 
void WIFI_setup(){
  Serial.println("[SETUP] WIFI: SETUP START");

  while (true)
  {
    pTxCharacteristic->setValue("wifi");
    pTxCharacteristic->notify();

    while(wifi_id == ""){
      pTxCharacteristic->setValue("wifi_id");
      pTxCharacteristic->notify();
      delay(1000);
    }
    pTxCharacteristic->setValue("wifi_pw");
    pTxCharacteristic->notify();
    delay(5000);
    Serial.println("WIFI ID = " + wifi_id + " PASSWORD = " + wifi_pw);
    WiFi.begin(wifi_id, wifi_pw);
    delay(15000);
    if(WiFi.status() == WL_CONNECTED){
      break;
    }
    else{
      Serial.println("연결안됨");
    }
    Serial.println("[SETUP] WIFI ID = " + wifi_id + " WIFI PASSWORD = " + wifi_pw);
  }
  
  Serial.println("[SETUP] WIFI: SETUP SUCCESS");
  pTxCharacteristic->setValue("wifi_success");
  pTxCharacteristic->notify();
}

// *************** time *************** 
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP,"pool.ntp.org",32400);

void TIME_setup() {
  Serial.println("[SETUP] TIME: SETUP START");
  timeClient.begin();
  timeClient.setTimeOffset(32400);  //GMT+9
  timeClient.forceUpdate();
  Serial.println("[SETUP] TIME: SETUP SUCCESS");
}

// *************** UID *************** 
// void UID_setup() {
//   for (size_t i = 0; i < 8; i++)
// 	{

//     UID += String(UniqueID8[i], HEX);
// 	}
// }

// *************** socket IO *************** 

void socketIO_setup() {
  // socketIO.begin("ip", port);
  // socketIO.onEvent(socketIOEvent);
}

void socketIOEvent(socketIOmessageType_t type, uint8_t * payload, size_t length) {
    // switch(type) {
    //     case sIOtype_DISCONNECT:
    //       // ...적절한 코드...
    //     case sIOtype_CONNECT:
    //         // ...적절한 코드...

    //         // join default namespace (no auto join in Socket.IO V3)
    //         socketIO.send(sIOtype_CONNECT, "/");
    //         break;
    //     case sIOtype_EVENT:
    //         // ...적절한 코드...
    //         switch (payload)
    //         {
    //         case "set-temp-humiddity":
              
    //           MAXHum = ;
    //           MINHum = ;
    //           send_MAXMINdata();
    //           breake;
    //         case "request-temp-humidity":
    //           DynamicJsonDocument doc(4096);
    //           doc["temperature"] = NOWTem.toFloat();
    //           doc["humidity"] = NOWHUM.toFloat();
    //           doc["cageId"] = "c8487f39-b222-477a-955c-60e15be3ea6d";

    //           // Serialize JSON document
    //           String nowdatajson;
    //           serializeJson(doc, nowdatajson);

    //           socketIO.send("response-temp-humidity", nowdatajson);
    //           break;
    //         case "request-img":
    //           //이미지 전송


    //           break;
    //         }
           
    //     case sIOtype_ACK:
    //         // ...적절한 코드...
    //     case sIOtype_ERROR:
    //         // ...적절한 코드...
    //     case sIOtype_BINARY_EVENT:
    //         // ...적절한 코드...
    //     case sIOtype_BINARY_ACK:
    //         // ...적절한 코드...
    // }
} 

// *************** send MAX MIN data *************** 

void send_MAXMINdata() {
  while (MAXTem == "" || MINTem == "" || MAXHum == "" || MINHum == "")
  {
    Serial_soft.print(MAXTem + " " + MINTem + " " + MAXHum + " " + MINHum + " " + MINHum + ";");
    delay(1000);
  }
}



// *************** setup *************** 
void setup() {
  Serial.begin(115200);     //시리얼 통신 속도 설정
  Serial_soft.begin(9600);  //소프트웨어 시리얼 통신 속도 설정

  // UID_setup();              //UID 저장
  BT_setup();
  WIFI_setup();           //WIFI 연결
  socketIO_setup();


  // send_MAXMINdata();        //최대 최소 온습도 전달 미완성
}

// *************** loop *************** 
void loop() {
  //현재 온습도 수신
  // if  (Serial_soft.available()){
  //   String text = Serial_soft.readStringUntil(';');
  //   Serial.println(text); //디버깅용
  //   int index = text.indexOf(' ');
  //   NOWTem = text.substring(0, index);
  //   NOWHUM = text.substring(index + 1);
  //   NOWTem.trim();
  //   NOWHUM.trim();
  //   //디버깅용
  //   Serial.println("nowtemp : " + NOWTem);
  //   Serial.println("nowhum : " + NOWHUM);
  //   Serial.println();
  //}



  // delay(30000);
  //현재 온습도 http 전송
  // Prepare JSON document
  DynamicJsonDocument doc(4096);
  doc["temperature"] = NOWTem.toFloat();
  doc["humidity"] = NOWHUM.toFloat();
  doc["cageId"] = "c8487f39-b222-477a-955c-60e15be3ea6d";

  // Serialize JSON document
  String json;
  serializeJson(doc, json);

  WiFiClientSecure client;  // or WiFiClientSecure for HTTPS
  HTTPClient http;

  // Send request
  http.begin(client, "https://api.ckie.store/cage-states");
  http.POST(json);

  // Read response
  Serial.print(http.getString());

  // Disconnect
  http.end();  

  delay(10000);
}




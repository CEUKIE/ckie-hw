/*
 * WebSocketClientSocketIOack.ino
 *
 *  Created on: 20.07.2019
 *
 */

#include <Arduino.h>
#include <BluetoothSerial.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

#include <ArduinoJson.h>
#include <HTTPClient.h>

#include <WebSocketsClient.h>
#include <SocketIOclient.h>

#include <SoftwareSerial.h>

#define RX 47
#define TX 21

#define SERVICE_UUID "2f05b2a5-079f-4a07-b9c0-3b1fe7d615c9"
#define CHARACTERISTIC_READ "01e7eeab-2597-4c54-84e8-2fceb73c645d"
#define CHARACTERISTIC_WRITE "5a9edc71-80cb-4159-b2e6-a2913b761026"

SoftwareSerial Serial_soft(RX, TX);

int now_hour = -1;

String UID = "", bluetooth_data="", MAXHum = "12", MINHum = "11", 
          MAXTem = "12", MINTem = "11", NOWHUM = "12.0", NOWTem = "33.3",
          wifi_id = "dlink1234", wifi_pw = "14159265";



WiFiMulti WiFiMulti;
SocketIOclient socketIO;




void socketIOEvent(socketIOmessageType_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case sIOtype_DISCONNECT:
            Serial.printf("[IOc] Disconnected!\n");
            break;
        case sIOtype_CONNECT:
            Serial.printf("[IOc] Connected to url: %s\n", payload);

            // join default namespace (no auto join in Socket.IO V3)
            socketIO.send(sIOtype_CONNECT, "/");
            break;
        case sIOtype_EVENT:
        {
            char * sptr = NULL;
            int id = strtol((char *)payload, &sptr, 10);
            Serial.printf("[IOc] get event: %s id: %d\n", payload, id);
            if(id) {
                payload = (uint8_t *)sptr;
            }
            DynamicJsonDocument doc(1024);
            DeserializationError error = deserializeJson(doc, payload, length);
            if(error) {
                Serial.print(F("deserializeJson() failed: "));
                Serial.println(error.c_str());
                return;
            }

            String eventName = doc[0];
            Serial.printf("[IOc] event name: %s\n", eventName.c_str());

            // Message Includes a ID for a ACK (callback)
            if(id) {
                // creat JSON message for Socket.IO (ack)
                DynamicJsonDocument docOut(1024);
                JsonArray array = docOut.to<JsonArray>();

                // add payload (parameters) for the ack (callback function)
                JsonObject param1 = array.createNestedObject();
                param1["now"] = millis();

                // JSON to String (serializion)
                String output;
                output += id;
                serializeJson(docOut, output);

                // Send event
                socketIO.send(sIOtype_ACK, output);
            }
        }
            break;
        case sIOtype_ACK:
            Serial.printf("[IOc] get ack: %u\n", length);
            break;
        case sIOtype_ERROR:
            Serial.printf("[IOc] get error: %u\n", length);
            break;
        case sIOtype_BINARY_EVENT:
            Serial.printf("[IOc] get binary: %u\n", length);
            break;
        case sIOtype_BINARY_ACK:
            Serial.printf("[IOc] get binary ack: %u\n", length);
            break;
    }
}

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
      /*
      데이터 전달 형식
      "wifi_id str; wifi_pw str; min_temp str; max_temp str; min_humidity str; max_humidity str;"
      */
      while (wifi_id == "" || wifi_pw == "" || MAXTem == "" || MINTem == "" || MAXHum == "" || MINHum == "")
      {
        if (bluetooth_data.indexOf("wifi_id") != -1)
        {
          int start = bluetooth_data.indexOf("wifi_id") + 8;
          int end = bluetooth_data.indexOf(';', start);
          wifi_id = bluetooth_data.substring(start, end);
        }
        if (bluetooth_data.indexOf("wifi_pw") != -1)
        {
          int start = bluetooth_data.indexOf("wifi_pw") + 8;
          int end = bluetooth_data.indexOf(';', start);
          wifi_pw = bluetooth_data.substring(start, end);
        }
        if (bluetooth_data.indexOf("min_temp") != -1)
        {
          int start = bluetooth_data.indexOf("min_temp") + 9;
          int end = bluetooth_data.indexOf(';', start);
          MINTem = bluetooth_data.substring(start, end);
        }
        if (bluetooth_data.indexOf("max_temp") != -1)
        {
          int start = bluetooth_data.indexOf("max_temp") + 9;
          int end = bluetooth_data.indexOf(';', start);
          MAXTem = bluetooth_data.substring(start, end);
        }
        if (bluetooth_data.indexOf("min_humidity") != -1)
        {
          int start = bluetooth_data.indexOf("min_humidity") + 13;
          int end = bluetooth_data.indexOf(';', start);
          MINHum = bluetooth_data.substring(start, end);
        }
        if (bluetooth_data.indexOf("max_humidity") != -1)
        {
          int start = bluetooth_data.indexOf("max_humidity") + 13;
          int end = bluetooth_data.indexOf(';', start);
          MAXHum = bluetooth_data.substring(start, end);
        }
      }

      Serial.println("wifi id : " + wifi_id + "wifi pw : " + wifi_pw);
      Serial.println("max temp : " + MAXTem + "min temp : " + MINTem + "max hum : " + MAXHum + "min hum : " + MINHum);
      Serial.println("payload : " + bluetooth_data);
    }
  }
};

void BT_setup() {
  Serial.println("[SETUP] BLUETOOTH SETUP START");
  String bluetooth_name = "Ckie";

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

  Serial.println("[SETUP] BLUETOOTH SETUP SUCCESS");
  send_MAXMINdata();
}
// *************** WIFI *************** 

void WIFI_setup(){
  Serial.println("[SETUP] WIFI: SETUP START");

  while (true)
  {
    pTxCharacteristic->setValue("wifi set up");
    pTxCharacteristic->notify();

    Serial.print("WIFI ID = ");
    Serial.println(wifi_id);
    Serial.print("PASSWORD = ");
    Serial.println(wifi_pw);
    WiFi.begin(wifi_id, wifi_pw);
    delay(15000);
    if(WiFi.status() == WL_CONNECTED){
      break;
    }
    else{
      Serial.println("연결안됨");
    }
    Serial.print("WIFI ID = ");
    Serial.println(wifi_id);
    Serial.print("PASSWORD = ");
    Serial.println(wifi_pw);
  }
  
  Serial.println("[SETUP] WIFI: SETUP SUCCESS");
  pTxCharacteristic->setValue("wifi_success");
  pTxCharacteristic->notify();
}

// *************** get GMT+9 time *************** 

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP,"pool.ntp.org",32400);

void TIME_setup() {
  Serial.println("[SETUP] TIME: SETUP START");
  timeClient.begin();
  timeClient.setTimeOffset(32400);  //GMT+9
  timeClient.forceUpdate();
  Serial.println("[SETUP] TIME: SETUP SUCCESS");
}
// 시간 변경 체크 후 현재 온습도 전송
void update_hour() {
  if (now_hour != timeClient.getHours())
  {
    Serial.println("hour changed!");
    now_hour = timeClient.getHours();
    send_now_data();
  }
}

// *************** send now data *************** 

void send_now_data() {
  // http POST request
  // Prepare JSON document
  DynamicJsonDocument doc(4096);
  doc["temperature"] = NOWTem.toFloat();
  doc["humidity"] = NOWHUM.toFloat();
  doc["cageId"] = "c8487f39-b222-477a-955c-60e15be3ea6d";

  // Serialize JSON document
  String json;
  serializeJson(doc, json);

  WiFiClient client;

  Serial.println("http POST start" + json);
  // client->setCACert(rootCACertificate);

  HTTPClient http;
  http.begin(client, "http://3.36.227.176:8080/cage-states");
  http.addHeader("Content-Type", "application/json");
  http.POST(json);
  
  // Read response
  Serial.println(http.getString());
  
  // Disconnect
  http.end();

  delay(1000);  
}

// *************** send MAX MIN data *************** 

void send_MAXMINdata() {
  while (MAXTem == "" || MINTem == "" || MAXHum == "" || MINHum == "")
  {
    Serial_soft.print(MAXTem + " " + MINTem + " " + MAXHum + " " + MINHum + " " + MINHum + ";");
    delay(1000);
  }
}

// ************** get now data *************** 

void get_now_data() {
  //현재 온습도 수신
  if  (Serial_soft.available()){
    String text = Serial_soft.readStringUntil(';');
    Serial.println("payload : " + text); //디버깅용
    int index = text.indexOf(' ');
    NOWTem = text.substring(0, index);
    NOWHUM = text.substring(index + 1);
    NOWTem.trim();
    NOWHUM.trim();
    //디버깅용
    Serial.println("nowtemp : " + NOWTem);
    Serial.println("nowhum : " + NOWHUM);
    Serial.println();
  }
}



void setup() {
    //Serial.begin(921600);
    Serial.begin(115200);

    Serial_soft.begin(9600);

    BT_setup();
    delay(1000);
    WIFI_setup();     
    delay(1000);
    TIME_setup();
    delay(1000);


    //Serial.setDebugOutput(true);
    Serial.setDebugOutput(true);

    Serial.println();
    Serial.println();
    Serial.println();

      for(uint8_t t = 4; t > 0; t--) {
          Serial.printf("[SETUP] BOOT WAIT %d...\n", t);
          Serial.flush();
          delay(1000);
      }

    // WiFiMulti.addAP("dlink1234", "14159265");

    // //WiFi.disconnect();
    // while(WiFiMulti.run() != WL_CONNECTED) {
    //     delay(100);
    // }

    // String ip = WiFi.localIP().toString();
    // Serial.printf("[SETUP] WiFi Connected %s\n", ip.c_str());

    // server address, port and URL
    socketIO.begin("3.36.227.176", 8080, "/socket.io/?EIO=4");

    // event handler
    socketIO.onEvent(socketIOEvent);
}

unsigned long messageTimestamp = 0;
void loop() {
    // update_hour();
    // delay(1000);

    socketIO.loop();

    uint64_t now = millis();

    if(now - messageTimestamp > 2000) {
        messageTimestamp = now;

        // creat JSON message for Socket.IO (event)
        DynamicJsonDocument doc(1024);
        JsonArray array = doc.to<JsonArray>();

        // add evnet name
        // Hint: socket.on('event_name', ....
        array.add("event_name");

        // add payload (parameters) for the event
        JsonObject param1 = array.createNestedObject();
        param1["now"] = (uint32_t) now;

        // JSON to String (serializion)
        String output;
        serializeJson(doc, output);

        // Send event
        socketIO.sendEVENT(output);

        // Print JSON for debugging
        Serial.println(output);
    }
}
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
#include "camera_pins.h"
#include "esp_camera.h"

#define RX 47
#define TX 21
#define SERVICE_UUID "2f05b2a5-079f-4a07-b9c0-3b1fe7d615c9"
#define CHARACTERISTIC_READ "01e7eeab-2597-4c54-84e8-2fceb73c645d"
#define CHARACTERISTIC_WRITE "5a9edc71-80cb-4159-b2e6-a2913b761026"

const char* rootCACertificate = \
"\n";


SoftwareSerial Serial_soft(RX, TX);

int now_hour = 0, port = 0;

String UID = "", bluetooth_data="", MAXHum = "", MINHum = "", 
          MAXTem = "", MINTem = "", NOWHUM = "12.0", NOWTem = "33.3",
          wifi_id = "", wifi_pw = "",
          ip = "";

SocketIOclient socketIO;

bool esp32_setup = false;

// to-do
// 카메라 전송
// 소캣 io 테스트

// *************** software reset *************** 
void restart_esp32 () {
  ESP.restart();
  delay(1000);
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
      //"wifi_id dlink1234; wifi_pw 14159265; min_temp 24.5; max_temp 26; min_humidity 60.4; max_humidity 70;"
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
      Serial.println("max temp : " + MAXTem + "min temp : " + MINTem + "max hum : " + MAXHum + "min hum : " + MINHum)
      Serial.println("payload : "bluetooth_data);
    }
  }
};

void BT_setup() {
  Serial.println("[SETUP] BLUETOOTH: " + String(CAGE_NUM) + ".NO BLUETOOTH SETUP START");
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

void update_hour() {
  if (now_hour != timeClient.getHours())
  {
    now_hour = timeClient.getHours();
    send_now_data();
  }
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
  socketIO.begin("192.168.1.1", 8080, "/socket.io/?EIO=4");
  socketIO.onEvent(socketIOEvent);
}

void socketIOEvent(socketIOmessageType_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case sIOtype_DISCONNECT:
      // ...적절한 코드...

      socketIO.send(sIOtype_DISCONNECT, "/CKIE Disconnected!");
      break;
    case sIOtype_CONNECT:
      // ...적절한 코드...

      // join default namespace (no auto join in Socket.IO V3)
      socketIO.send(sIOtype_CONNECT, "/CKIE Connected!");
      break;
    case sIOtype_EVENT:
      // ...적절한 코드...
      {
        Serial.printf("[IOc] get event: %s\n", payload);
        String msg = (char*)payload;
        if (msg.indexOf("camera") != -1)
        {
          grab_send_img();
        }
        else if (msg.indexOf("change-temp"))
        {
          
        }
        
        else if (msg.indexOf("change-humiddity") != -1)
        {
          // 데이터 처리 부분 미완성
          // MAXTem = ;
          // MINTem = ;
          // MAXHum = ;
          // MINHum = ;
          send_MAXMINdata();            
        }
        else if (msg.indexOf("request-temp-humidity") != -1)
        {
          send_now_data();
        }
        
        break;
      }
    case sIOtype_ACK:
      break;
    case sIOtype_ERROR:
      break;
    case sIOtype_BINARY_EVENT:
      break;
    case sIOtype_BINARY_ACK:
      break;
  }
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
    Serial.println(text); //디버깅용
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


// *************** send Now data *************** 

void send_now_data() {
  //현재 온습도

  // Send request https 전송
  // Prepare JSON document
  DynamicJsonDocument doc(4096);
  doc["temperature"] = NOWTem.toFloat();
  doc["humidity"] = NOWHUM.toFloat();
  doc["cageId"] = SERVICE_UUID;

  // Serialize JSON document
  String json;
  serializeJson(doc, json);

  WiFiClientSecure *client = new WiFiClientSecure;
  if (client)
  {
    client->setCACert(rootCACertificate);

    HTTPClient https;
    https.begin(client, "https://api.ckie.store/cage-states");
    https.POST(json);
  }
  // Read response
  Serial.print(https.getString());

  // Disconnect
  https.end();
  delay(1000);  
}

// *************** camera *************** 

void camera_setup() {
  Serial.println("[SETUP] CAMERA: SETUP START");
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_HVGA;
  config.pixel_format = PIXFORMAT_JPEG; // for streaming
  //config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  if(psramFound()){
      config.jpeg_quality = 10;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
  } else {
      // Limit the frame size when PSRAM is not available
      config.frame_size = FRAMESIZE_SVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
  }

  while (esp_camera_init(&config) != ESP_OK) {
      Serial.println("[ERROR] CAMERA: SETUP FAIL");
      delay(500);
  }
  Serial.println("[SETUP] CAMERA: SETUP SUCCESS");
}

void grab_send_img() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (fb != NULL && fb->format == PIXFORMAT_JPEG) {
    String img = "";
    for (size_t i = 0; i < fb->len; i++)
    {
      img += char(fb->buf[i]);
    }
    
    DynamicJsonDocument doc(1024);
    JsonArray array = doc.to<JsonArray>();
    //array.add("event-name")
    array.add("send-img");
    //add payload
    JsonObject param1 = array.createNestedObject();
    param1["img"] = img;
    String output;
    serializeJson(doc, output);

    // Send evnet
    socketIO.sendEVENT(output);

    // Print Json for debugging
    Serial.println(output);
  }
}




// *************** setup *************** 
void setup() {
  Serial.begin(115200);     //시리얼 통신 속도 설정
  Serial_soft.begin(9600);  //소프트웨어 시리얼 통신 속도 설정

  // UID_setup();              //UID 저장
  BT_setup();
  delay(1000);
  WIFI_setup();     
  delay(1000);           
  socketIO_setup();
  delay(1000);           
  camera_setup();
  delay(1000);           

  esp32_setup = true
  pTxCharacteristic->setValue(esp32_setup);
  pTxCharacteristic->notify();
}

// *************** loop *************** 
void loop() {
  socketIO.loop();
  delay(1000);
  get_now_data();
  delay(1000);


  // delay(30000);

  send_now_data();

  delay(10000);
}




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
#include <base64.h>

#define RX 47
#define TX 21
#define SERVICE_UUID "2f05b2a5-079f-4a07-b9c0-3b1fe7d615c9"
#define CHARACTERISTIC_READ "01e7eeab-2597-4c54-84e8-2fceb73c645d"
#define CHARACTERISTIC_WRITE "5a9edc71-80cb-4159-b2e6-a2913b761026"

unsigned long int preTime = 0;
unsigned long int interval = 10000;


SoftwareSerial Serial_soft(RX, TX);

int now_hour = -1;

String UID = "", bluetooth_data="", MAXHum = "60", MINHum = "70", MAXTem = "25", MINTem = "28", 
        NOWHUM = "12.0", NOWTem = "33.3", wifi_id = "", wifi_pw = "";

SocketIOclient socketIO;


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

// *************** socket IO *************** 

void socketIO_setup() {
  socketIO.begin("IP", "PORT", "/socket.io/?EIO=4");
  socketIO.onEvent(socketIOEvent);
}

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
        Serial.printf("[IOc] get event: %s\n", payload);
        String msg = (char*)payload;
        if (msg.indexOf("connection") != -1)
        {
          // creat JSON message for Socket.IO (event)
          //SocketIO로 보낼 JSON 메시지(=Event) 객체를 생성한다.
          DynamicJsonDocument doc(1024);
          JsonArray array = doc.to<JsonArray>();

          // add evnet name
          // Hint: socket.on('event_name', ....
          //객체에 event_name을 추가한다. 이벤트 처리함수의 매개변수 중 type에 해당하는 부분이다.
          array.add("connect-cage");

          // add payload (parameters) for the event
          //객체에 데이터를 추가한다. 이벤트 처리함수의 매개변수 중 payload에 해당하는 부분이다.
          JsonObject param1 = array.createNestedObject();
          param1["cageId"] = "c8487f39-b222-477a-955c-60e15be3ea6d";

          // JSON to String (serializion)
          //JSON 메시지를 문자열로 직렬화한다.
          String output;
          serializeJson(doc, output);

          // Send event
          //직렬화한 메시지를 송신(전송)한다.
          socketIO.sendEVENT(output);

          // Print JSON for debugging
          //보낸 메시지를 시리얼 모니터에 출력하여 확인한다.
          Serial.println(output);
        }
        if (msg.indexOf("request-photo") != -1)
        {
          grab_send_img();
        }
        if (msg.indexOf("change-temp") != -1)
        {
          StaticJsonDocument<200> doc;
          DeserializationError error = deserializeJson(doc, payload);

          if (error)
          {
            Serial.print(F("deserializeJson() 실패: "));
            Serial.println(error.f_str());
            return;
          }
          if (!doc.is<JsonArray>()) {
            Serial.println(F("JSON 데이터가 배열 형식이 아닙니다."));
            return;
          }

          // JSON 배열에서 요소를 가져옵니다.
          JsonArray arr = doc.as<JsonArray>();

          // 두 번째 요소 (객체)를 확인하고 문자열로 변환합니다.
          JsonObject obj = arr[1];

          if (obj.containsKey("minTemp") && obj.containsKey("maxTemp")) {
            const char* minTemp = obj["minTemp"];
            const char* maxTemp = obj["maxTemp"];

            MINTem = String(minTemp);
            MAXTem = String(maxTemp);
   
          } else {
            Serial.println("minTemp 또는 maxTemp 키를 찾을 수 없습니다.");
          }

          send_MAXMINdata();
        }
        if (msg.indexOf("change-humidity") != -1)
        {
          StaticJsonDocument<200> doc;
          DeserializationError error = deserializeJson(doc, payload);

          if (error)
          {
            Serial.print(F("deserializeJson() 실패: "));
            Serial.println(error.f_str());
            return;
          }
          if (!doc.is<JsonArray>()) {
            Serial.println(F("JSON 데이터가 배열 형식이 아닙니다."));
            return;
          }

          // JSON 배열에서 요소를 가져옵니다.
          JsonArray arr = doc.as<JsonArray>();

          // 두 번째 요소 (객체)를 확인하고 문자열로 변환합니다.
          JsonObject obj = arr[1];

          if (obj.containsKey("minHumidity") && obj.containsKey("maxHumidity")) {
            const char* minHumi = obj["minHumidity"];
            const char* maxHumi = obj["maxHumidity"];

            MINHum = String(minHumi);
            MAXHum = String(maxHumi);
   
          } else {
            Serial.println("minHum 또는 maxHum 키를 찾을 수 없습니다.");
          }

          send_MAXMINdata();            
        }
        if (msg.indexOf("request-temp-humidity") != -1)
        {
          DynamicJsonDocument doc(1024);
          JsonArray array = doc.to<JsonArray>();

          array.add("response-temp-humidity");

          JsonObject param1 = array.createNestedObject();
          param1["temperature"] = NOWTem.toFloat();
          param1["humidity"] = NOWHUM.toFloat();
          param1["cageId"] = "c8487f39-b222-477a-955c-60e15be3ea6d";

          String output;
          serializeJson(doc, output);

          socketIO.sendEVENT(output);

          Serial.println(output);
        }
        if (msg.indexOf("request-target-temp") != -1)
        {
          DynamicJsonDocument doc(1024);
          JsonArray array = doc.to<JsonArray>();

          array.add("response-target-temp");

          JsonObject param1 = array.createNestedObject();
          param1["minTemp"] = MINTem.toFloat();
          param1["maxTemp"] = MAXTem.toFloat();
          param1["cageId"] = "c8487f39-b222-477a-955c-60e15be3ea6d";
          String output;
          serializeJson(doc, output);

          socketIO.sendEVENT(output);

          Serial.println(output);
        } 
        if (msg.indexOf("request-target-humidity") != -1)
        {
          DynamicJsonDocument doc(1024);
          JsonArray array = doc.to<JsonArray>();

          array.add("response-target-humidity");

          JsonObject param1 = array.createNestedObject();
          param1["minHumidity"] = MINHum.toFloat();
          param1["maxHumidity"] = MAXHum.toFloat();
          param1["cageId"] = "c8487f39-b222-477a-955c-60e15be3ea6d";

          String output;
          serializeJson(doc, output);

          socketIO.sendEVENT(output);

          Serial.println(output);
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
  Serial_soft.print("data" +MAXTem + " " + MINTem + " " + MAXHum + " " + MINHum + " " + ";");
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

  HTTPClient http;
  http.begin(client, "http://3.36.227.176:8080/cage-states");
  http.addHeader("Content-Type", "application/json");
  http.POST(json);
  
  // Read response
  Serial.println(http.getString());
  
  // Disconnect
  http.end();
}

// *************** camera *************** 

void camera_setup() {
  Serial.println("[SETUP] CAMERA: SETUP START");

  camera_config_t config;
  
  config.grab_mode = CAMERA_GRAB_LATEST;

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
  config.frame_size = FRAMESIZE_SVGA;
  config.pixel_format = PIXFORMAT_JPEG; // for streaming
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;
  
  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  // for larger pre-allocated frame buffer.
  if(psramFound()){
    config.jpeg_quality = 10;
    config.fb_count = 2;
    config.grab_mode = CAMERA_GRAB_LATEST;
  } else {
    // Limit the frame size when PSRAM is not available
    config.frame_size = FRAMESIZE_SVGA;
    config.fb_location = CAMERA_FB_IN_DRAM;
  }

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  Serial.println("[SETUP] CAMERA: SETUP SUCCESS");
}

void grab_send_img() {

  camera_fb_t *fb = esp_camera_fb_get();
  if (fb != NULL && fb->format == PIXFORMAT_JPEG) {
    // String image = "";
    // for (size_t i = 0; i < fb->len; i++)
    // {
    //   image += char(fb->buf[i]);
    // }

    Serial.println("http POST start");
    String boundary = "------c8487f39-b222-477a-955c-60e15be3ea6d";
    String contentType = "multipart/form-data; boundary=" + boundary;

// WiFiClient를 사용하여 직접 HTTP 요청 작성 및 전송
    WiFiClient client;
    if (!client.connect("3.36.227.176", 8080)) {
      Serial.println("Connection to server failed");
      esp_camera_fb_return(fb);
      return;
    }

    String bodyStart = "--" + boundary + "\r\n" +
                       "Content-Disposition: form-data; name=\"image\"; filename=\"image.jpg\"\r\n" +
                       "Content-Type: image/jpeg\r\n\r\n";
    String bodyEnd = "\r\n--" + boundary + "--\r\n";

    int contentLength = bodyStart.length() + fb->len + bodyEnd.length();

    // HTTP 요청 헤더 작성
    client.print(String("POST ") + "/files/upload" + " HTTP/1.1\r\n");
    client.print("Host: 3.36.227.176:8080\r\n");
    client.print("Content-Type: " + contentType + "\r\n");
    client.print("Content-Length: " + String(contentLength) + "\r\n");
    client.print("Connection: close\r\n\r\n");

    // HTTP 요청 바디 작성
    client.print(bodyStart);
    client.write(fb->buf, fb->len);
    client.print(bodyEnd);

    // 서버 응답 읽기
    while (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") {
        break;
      }
    }

    String response = client.readString();
    Serial.println(response);

    if (response.indexOf("\"statusCode\":200") != -1)
    {
      int start = response.indexOf("Path\":\"") + 7;
      int end = response.indexOf('\"', start);
      String url = response.substring(start, end);
      DynamicJsonDocument doc(1024);
      JsonArray array = doc.to<JsonArray>();

      array.add("response-photo");

      JsonObject param1 = array.createNestedObject();
      param1["cageId"] = "c8487f39-b222-477a-955c-60e15be3ea6d";
      param1["url"] = url;

      String output;
      serializeJson(doc, output);

      socketIO.sendEVENT(output);

      Serial.println(output);
    }
    else 
    {
      Serial.println("사진이 보내지지 않았습니다!!!");
    }


    
    

    client.stop();
    esp_camera_fb_return(fb); // 카메라 프레임 버퍼 반환
    else {
      Serial.println("Camera capture failed");
      if (fb != NULL) {
        esp_camera_fb_return(fb);
      }
  }
}

// *************** software reset *************** 

void restart_esp32 () {
  ESP.restart();
}

// *************** setup *************** 
void setup() {
  Serial.begin(115200);     //시리얼 통신 속도 설정
  Serial_soft.begin(9600);  //소프트웨어 시리얼 통신 속도 설정

  BT_setup();
  WIFI_setup();     
  socketIO_setup();
  camera_setup();
  TIME_setup();
  send_MAXMINdata();

  // setup 완료시 ble notify TRUE로 변경
  pTxCharacteristic->setValue("setup completed");
  pTxCharacteristic->notify();
}

// *************** loop ***************
void loop() {
  unsigned long int currentTime = millis();
  socketIO.loop();
  if (currentTime - preTime > interval) {
    	preTime = currentTime;
      get_now_data();
  }
  update_hour();
} 

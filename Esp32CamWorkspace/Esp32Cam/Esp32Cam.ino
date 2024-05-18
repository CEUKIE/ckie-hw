//시리얼 통신
#include <SoftwareSerial.h>

#define RX 13
#define TX 14

SoftwareSerial Serial_soft(RX, TX);

//legacy 블루투스 연결
#include <BluetoothSerial.h>

BluetoothSerial Serial_BT;

// 보드 UID
#include <ArduinoUniqueID.h>

String UID = "";

// wifi
#include <WiFi.h>

String SSID = "";
String PW = "";


// 카메라
#include "camera_pins.h"


String MAXHum = "";
String MINHum = "";
String MAXTem = "";
String MINTem = "";

void setup() {
  Serial.begin(115200);   //시리얼 통신 속도 설정
  Serial_soft.begin(9600);   //소프트웨어 시리얼 통신 속도 설정

  UID_setup();             //UID 저장

  Serial_BT.register_callback(BT_status); //블루투스 연결 콜백
  Serial_BT.begin("Ckie" + UID); //블루투스 셋업

  WIFI_connect();

}

void loop() {

  //소프트웨어 시리얼 통신 부분 (미완성)
  if  (Serial_soft.available()){
    String text = Serial_soft.readStringUntil('\n');
    Serial.println(text);
  }

  Serial_soft.println("esp32 Serial communication");
  delay(1500);


}

//블루투스 연결 이벤트
void BT_status (esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {

  if (event == ESP_SPP_SRV_OPEN_EVT) {
    Serial.println ("Client Connected");
    Serial_BT.println(UID);
    // 연결 될 경우 UID 전달
  }

  else if (event == ESP_SPP_CLOSE_EVT ) {
    Serial.println ("Client Disconnected");
  }
}

void WIFI_connect() {
  //wifi 통신 부분 (미완성)>
  while ((SSID == "") || (PW == ""))
  {
    /* code */
  }
  
}

void UID_setup() {
  for (size_t i = 0; i < 8; i++)
	{

    UID += String(UniqueID8[i], HEX);
	}
}
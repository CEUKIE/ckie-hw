//시리얼 통신
#include <SoftwareSerial.h>
#define RX 13
#define TX 14

SoftwareSerial Serial_soft(RX, TX);

//블루투스
#include <BluetoothSerial.h>

BluetoothSerial Serial_BT;
String bluetooth_data;

//보드 UID
#include <ArduinoUniqueID.h>

String UID = "";

//WIFI
#include <WiFi.h>

String SSID = "";
String PW = "";


//카메라
#include "camera_pins.h"

//테스트용으로 임시 온습도 설정
String MAXHum = "40.0";
String MINHum = "35.0";
String MAXTem = "29.7";
String MINTem = "20.8";

void setup() {
  Serial.begin(115200);     //시리얼 통신 속도 설정
  Serial_soft.begin(9600);  //소프트웨어 시리얼 통신 속도 설정

  UID_setup();              //UID 저장

  Serial_BT.register_callback(BT_status); //블루투스 콜백 등록
  Serial_BT.begin("Ckie" + UID);          //블루투스 이름 설정

  WIFI_connect();           //WIFI 연결

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
    Serial.println ("Bluetooth Connected");
    Serial_BT.println(UID);
    //블루투스가 연결 될 경우 UID 전달
  }

  else if (event == ESP_SPP_CLOSE_EVT ) {
    Serial.println ("Bluetooth Disconnected");
  }
}

//wifi 연결 함수
void WIFI_connect() {
  Serial.println("[SETUP] WIFI SETUP START");

  while ((SSID == "") || (PW == "") || (WiFi.status() != WL_CONNECTED))
  {
    bluetooth_data = Serial_BT.readStringUntil('\n');\

    if (bluetooth_data[0] == 's')
    {
      int spacePos = bluetooth_data.indexOf(' ');
      SSID = bluetooth_data.substring(spacePos + 1);
      SSID.trim();
    }
    else if (bluetooth_data[0] == 'p')
    {
      int spacePos = bluetooth_data.indexOf(' ');
      PW = bluetooth_data.substring(spacePos + 1);
      PW.trim();
    }

    WiFi.begin(SSID, PW);
    delay(1000);
  }
  Serial.println("wifi success!");
}

//UID 변수 저장 함수
void UID_setup() {
  for (size_t i = 0; i < 8; i++)
	{

    UID += String(UniqueID8[i], HEX);
	}
}
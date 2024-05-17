//시리얼 통신
#include <SoftwareSerial.h>

#define RX 13
#define TX 14

SoftwareSerial mySerial(RX, TX);

//legacy 블루투스 연결
#include <BluetoothSerial.h>

BluetoothSerial SerialBT;

void setup() {
  Serial.begin(115200);   //시리얼 통신 속도 설정

  mySerial.begin(9600);   //소프트웨어 시리얼 통신 속도 설정

  SerialBT.begin("ESP32cam-test"); //블루투스 셋업
}

void loop() {
  //소프트웨어 시리얼 통신 부분 (미완성)
  if  (mySerial.available()){
    String text = mySerial.readStringUntil('\n');
    Serial.println(text);
  }

  mySerial.println("esp32 Serial communication");
  delay(1500);

  //블루투스 통신 부분 (미완성)>
  if (Serial.available())
  {
    SerialBT.write(Serial.read());
  }
  if (SerialBT.available())
  {
    Serial.write(SerialBT.read());
  }
  delay(20);
}
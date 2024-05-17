#include <SoftwareSerial.h>

#define RX 3
#define TX 4

SoftwareSerial soft(RX, TX);

void setup() {
  Serial.begin(9600); // 시리얼 통신 속도 설정
  soft.begin(9600);
}

void loop() {
  soft.println("hello boss");
  delay(1500);

  if(soft.available()){
    String text = soft.readStringUntil('\n');
    Serial.println(text);
  }
}

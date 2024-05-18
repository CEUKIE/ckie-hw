//시리얼 통신
#include <SoftwareSerial.h>

#define RX 3
#define TX 4

SoftwareSerial Serial_soft(RX, TX);

String MINTem = "";   //사육장 최저 온도
String MAXTem = "";   //사육장 최고 온도
String MINHum = "";   //사육장 최저 습도
String MAXHum = "";   //사육장 최고 습도


//온습도 센서
#include <DHT.h>
#include <DHT_U.h>

#define pinDHT 11
#define DHTTYPE DHT22
DHT dht (pinDHT, DHTTYPE);

float NOWTem;
float NOWHum;

//lcd
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);

//가습기 모듈
int pinMICRO = 5;

//릴레이 모듈
int pinRelay = 6;


void setup() {
  Serial.begin(9600); //시리얼 통신 속도 설정
  dht.begin();        //dht센서 연결

  lcd_Setup();        //lcd 초기화

  pinMode(pinMICRO, OUTPUT);  //가습기 모듈 핀모드 설정

  pinMode(pinRelay, OUTPUT);  //릴레이 모듈 핀모드 설정

  Serial_soft.begin(9600);    //소프트웨어 시리얼 통신 속도 설정

  recieve_MAXMINdata();       //최고최저 온습도 수신
}

void loop() {
  //현재 온습도 저장
  NOWHum = dht.readHumidity();
  NOWTem = dht.readTemperature();

  //시리얼 모니터에 온습도 출력
  Serial.print(NOWTem);
  Serial.print(", ");
  Serial.println(NOWHum);

  // lcd 출력
  lcd_Screen(NOWTem, NOWHum);
  delay(1000);


  //가습기 모듈 작동
  if (NOWHum < MINHum.toFloat() && NOWHum < MAXHum.toFloat())
  {
    digitalWrite(pinMICRO, HIGH);
  }
  else
  {
    digitalWrite(pinMICRO, LOW);
  }
  
  delay(1000);


  // 릴레이 작동
  if (NOWTem < MINTem.toFloat() && NOWTem < MAXTem.toFloat())
  {
    digitalWrite(pinRelay, HIGH);
  }
  else{
    digitalWrite(pinRelay, LOW);
  }
  delay(1000);



  // 현재 온습도 송신 시리얼 통신 
  Serial_soft.println(String(NOWTem) + " " + String(NOWHum) + ";");
  delay(1000);
}

//lcd 설정 함수
void lcd_Setup() {
  lcd.init();
  lcd.backlight();
  lcd.clear();
}

//lcd 출력 함수
void lcd_Screen(float NOWTem, float NOWHum) {
  String temp = "Temp : " + String(NOWTem) + " C";
  lcd.setCursor(0, 0);
  lcd.print(temp);

  String hum = "Hum : " + String(NOWHum) + " %";
  lcd.setCursor(0, 1);
  lcd.print(hum);
}

//최대 최소 온습도 수신
void recieve_MAXMINdata() {
  while (MAXTem == "" || MINTem == "" || MAXHum == "" || MINHum == "") {
    String text = Serial_soft.readStringUntil(';');
    Serial.println(text); //디버깅용
    int index1 = text.indexOf(' ');
    int index2 = text.indexOf(' ', index1+1);
    int index3 = text.indexOf(' ', index2+1);
    int index4 = text.indexOf(' ', index3+1);
    
    MAXTem = text.substring(0, index1);
    MINTem = text.substring(index1 + 1, index2);
    MAXHum = text.substring(index2 + 1, index3);
    MINHum = text.substring(index3 + 1, index4);

    MAXTem.trim();
    MINTem.trim();    
    MAXHum.trim();
    MINHum.trim();

    //디버깅용
    Serial.println("maxtemp : " + MAXTem);
    Serial.println("mintemp : " + MINTem);
    Serial.println("maxhum : " + MAXHum);
    Serial.println("minhum : " + MINHum);
    Serial.println();

    delay(1000);
  }
}

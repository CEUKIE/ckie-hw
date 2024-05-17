//시리얼 통신
#include <SoftwareSerial.h>

#define RX 3
#define TX 4

SoftwareSerial soft(RX, TX);

float setMinTemp; //사육장 최저 온도
float setMaxTemp; //사육장 최고 온도
float setMinHum;  //사육장 최저 습도
float sethum;     //사육장 최고 습도


//온습도 센서
#include <DHT.h>
#include <DHT_U.h>

#define pinDHT 11
#define DHTTYPE DHT22
DHT dht (pinDHT, DHTTYPE);

float realtemp;
float realhum;

//lcd
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);

//가습기 모듈
int pinMICRO = 5;

//릴레이 모듈
int pinRelay = 6;


void setup() {
  Serial.begin(9600); // 시리얼 통신 속도 설정
  dht.begin();        // dht센서 연결

  lcd_Setup();        // lcd 초기화

  pinMode(pinMICRO, OUTPUT); //가습기 모듈 핀모드 설정

  pinMode(pinRelay, OUTPUT); //릴레이 모듈 핀모드 설정

  soft.begin(9600);   // 소프트웨어 시리얼 통신 속도 설정
}

void loop() {
  //dht read
  realhum = dht.readHumidity();
  realtemp = dht.readTemperature();

  // 시리얼 모니터에 온습도 출력
  Serial.print(realtemp);
  Serial.print(", ");
  Serial.println(realhum);

  // lcd 출력
  lcd_Screen(realtemp, realhum);
  delay(1000);


  //가습기 모듈 작동 (미완성)
  digitalWrite(pinMICRO, HIGH);
  delay(1000);
  digitalWrite(pinMICRO, LOW);
  delay(1000);


  // 릴레이 작동 (미완성)
  digitalWrite(pinRelay, HIGH);
  delay(1000);
  digitalWrite(pinRelay, LOW);
  delay(1000);



  // 소프트웨어 시리얼 통신 (미완성)
  soft.println("hello boss");
  delay(1500);

  if(soft.available()){
    String text = soft.readStringUntil('\n');
    Serial.println(text);
  }
}

//lcd 설정 함수
void lcd_Setup() {
  lcd.init();
  lcd.backlight();
  lcd.clear();
}

//lcd 출력 함수
void lcd_Screen(float realtemp, float realhum) {
  String temp = "Temp : " + String(realtemp) + " C";
  lcd.setCursor(0, 0);
  lcd.print(temp);

  String hum = "Hum : " + String(realhum) + " %";
  lcd.setCursor(0, 1);
  lcd.print(hum);
}

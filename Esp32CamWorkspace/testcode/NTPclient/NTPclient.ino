#include <NTPClient.h>
#include <WiFi.h>
#include <WiFiUdp.h>

const char *ssid     = "";
const char *password = "";

WiFiUDP ntpUDP;

// By default 'pool.ntp.org' is used with 60 seconds update interval and
// no offset
NTPClient timeClient(ntpUDP);

// You can specify the time server pool and the offset, (in seconds)
// additionally you can specify the update interval (in milliseconds).
// NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);

void setup(){
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 );
    Serial.print ( "." );
  }

  timeClient.begin();
  timeClient.setTimeOffset(32400); // GMT+9
}

void loop() {
  timeClient.update();
  int hour = timeClient.getHours();

  Serial.println(hour);
  Serial.println(timeClient.getHours());
  Serial.println(timeClient.getFormattedTime());

  delay(1000);
}
// 아두이노 안의 해당 함수를 수정하면 된다.
void socketIOEvent(socketIOmessageType_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case sIOtype_DISCONNECT:
      USE_SERIAL.printf("[IOc] Disconnected!\n");
      break;
    case sIOtype_CONNECT:
      USE_SERIAL.printf("[IOc] Connected to url: %s\n", payload);

      // join default namespace (no auto join in Socket.IO V3)
      socketIO.send(sIOtype_CONNECT, "/");
      break;
    case sIOtype_EVENT:
    {		// 아두이노가 메세지를 수신하는 지점
      USE_SERIAL.printf("[IOc] get event: %s\n", payload);
      // 비교를 위해서 문자열로 되어 있는 payload를 String 형식으로 바꿔주고
      String msg = (char*)payload;
      // 아두이노가 msg를 해석하고 동작할 코드
      if (msg == "[\"chat message\",\"1\"]") {
        digitalWrite(D1, HIGH);
        delay(1000);
        digitalWrite(D1, LOW);
      // msg라는 String에서 '2'라는 문자가 있으면 index위치를, 없다면 -1를 뱉는 indexOf() 방법2
      } else if (msg.indexOf('2') != -1) {
        digitalWrite(D1, HIGH);
        delay(2000);
        digitalWrite(D1, LOW);
      }
      break;
    }
    case sIOtype_ACK:
      USE_SERIAL.printf("[IOc] get ack: %u\n", length);
      hexdump(payload, length);
      break;
    case sIOtype_ERROR:
      USE_SERIAL.printf("[IOc] get error: %u\n", length);
      hexdump(payload, length);
      break;
    case sIOtype_BINARY_EVENT:
      USE_SERIAL.printf("[IOc] get binary: %u\n", length);
      hexdump(payload, length);
      break;
    case sIOtype_BINARY_ACK:
      USE_SERIAL.printf("[IOc] get binary ack: %u\n", length);
      hexdump(payload, length);
      break;
  }
}

// 아두이노 코드 안의 loop문을 이렇게 수정해주면 된다.
void loop() {
  socketIO.loop();
  // 아두이노 시리얼에서 입력 후 테스트 할 부분
  if (Serial.available()) {
    char c = Serial.read();
    DynamicJsonDocument doc(1024);
    JsonArray array = doc.to<JsonArray>();

    if (c == '0') {
      // add evnet name
      // Hint: socket.on('event_name', ....
      array.add("chat message");
      array.add("ESP8266BOARD");
    } else if (c == '1') {
      array.add("chat message");
      array.add("disconnect");
    } else if (c == '2') {
      array.add("chat message");
      array.add("잘 전송 되나?");
    }
    // 만들어 놓은 구조를 바탕으로 JSON Seriallize(직렬화)
    String output;
    serializeJson(doc, output);

    //시리얼모니터에 테스트 메세지 출력(전송한 메세지 출력)
    USE_SERIAL.println(output);

    // 서버로 전송
    // ["chat message", "원하는 메세지"]
    socketIO.sendEVENT(output);
  }
}
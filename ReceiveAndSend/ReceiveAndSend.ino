#include <WiFi.h>
#include <WebServer.h>
#include <Arduino.h>
#include "PinDefinitionsAndMore.h"
#include <IRremote.hpp>

#define RAW_BUFFER_LENGTH 750
#define MARK_EXCESS_MICROS 20

#define pinLED 27
int STATUS_PIN = 2;
int DEFAULT_NUMBER_OF_REPEATS_TO_SEND = 1;
int estadoAnt = 0;

struct storedIRDataStruct {
  IRData receivedIRData;              
  uint16_t rawCode[RAW_BUFFER_LENGTH]; 
  uint16_t rawCodeLength;             
} sStoredIRData[10];

void storeCode(IRData *aIRReceivedData, int index);
void sendCode(storedIRDataStruct *aIRDataToSend);

volatile int registerIndex = -1;

void setupIR() {
  pinMode(pinLED, OUTPUT);
  digitalWrite(pinLED, LOW);
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
  IrSender.begin();
  pinMode(STATUS_PIN, OUTPUT);
  Serial.println(F("IR module initialized."));
}

void loopIR() {
  static int index = -1;
  static int estadoAtual = 0;

  if (Serial.available()) {
    char recebido = Serial.read();
    if (recebido == '#') {
      estadoAtual = 1;
      Serial.println(F("Modo Gravacao (Serial)"));
      delay(50);
      recebido = Serial.read();
      index = atoi(&recebido);
      Serial.print(F("Aguardando Codigo #"));
      Serial.println(recebido);
      digitalWrite(pinLED, HIGH);
    }
    else if (recebido >= '0' && recebido <= '9') {
      Serial.println(F("Modo Envio (Serial)"));
      estadoAtual = 2;
      index = atoi(&recebido);
    }
  }

  if (registerIndex >= 0) {
    estadoAtual = 1;
    index = registerIndex;
  }

  if (estadoAtual != estadoAnt && estadoAtual == 1) {
    IrReceiver.start();
  }

  if (estadoAtual == 2 && index >= 0) {
    IrReceiver.stop();
    Serial.println(WiFi.localIP());
    digitalWrite(STATUS_PIN, HIGH);
    sendCode(&sStoredIRData[index]);
    digitalWrite(STATUS_PIN, LOW);
    index = -1;
    estadoAtual = 0;
    registerIndex = -1;
  }

  if (estadoAtual == 1 && IrReceiver.available()) {
    Serial.print(F("Registrando comando no slot "));
    Serial.println(index);
    storeCode(IrReceiver.read(), index);
    IrReceiver.resume();
    digitalWrite(pinLED, LOW);
    registerIndex = -1;
    estadoAtual = 0;
  }
  estadoAnt = estadoAtual;
}

void storeCode(IRData *aIRReceivedData, int index) {
  if (aIRReceivedData->flags & IRDATA_FLAGS_IS_REPEAT) {
    Serial.println(F("Ignore repeat"));
    return;
  }
  if (aIRReceivedData->flags & IRDATA_FLAGS_IS_AUTO_REPEAT) {
    Serial.println(F("Ignore autorepeat"));
    return;
  }
  if (aIRReceivedData->flags & IRDATA_FLAGS_PARITY_FAILED) {
    Serial.println(F("Ignore parity error"));
    return;
  }
  sStoredIRData[index].receivedIRData = *aIRReceivedData;
  if (sStoredIRData[index].receivedIRData.protocol == UNKNOWN) {
    Serial.print(F("Received unknown code and storing "));
    uint16_t len = IrReceiver.decodedIRData.rawDataPtr->rawlen - 1;
    Serial.print(len);
    Serial.println(F(" timing entries as raw "));
    IrReceiver.printIRResultRawFormatted(&Serial, true);

    sStoredIRData[index].rawCodeLength = len;
    for (uint16_t i = 0; i < len; i++) {
      int16_t rawValue = IrReceiver.decodedIRData.rawDataPtr->rawbuf[i + 1];
      uint16_t value = abs(rawValue);
      if ((i % 2) == 0) {
        if (value > MARK_EXCESS_MICROS) {
          value -= MARK_EXCESS_MICROS;
        }
      }
      sStoredIRData[index].rawCode[i] = value;
    }
  }
  else {
    IrReceiver.printIRResultShort(&Serial);
    IrReceiver.printIRSendUsage(&Serial);
    sStoredIRData[index].receivedIRData.flags = 0; 
    Serial.println();
  }
  Serial.println(F("Comando registrado com sucesso."));
}

void sendCode(storedIRDataStruct *aIRDataToSend) {
  if (aIRDataToSend->receivedIRData.protocol == UNKNOWN) {
    IrSender.sendRaw(aIRDataToSend->rawCode, aIRDataToSend->rawCodeLength, 38);
    Serial.print(F("Sent raw "));
    Serial.print(aIRDataToSend->rawCodeLength);
    Serial.println(F(" marks or spaces"));
  }
  else {
    IrSender.write(&aIRDataToSend->receivedIRData, DEFAULT_NUMBER_OF_REPEATS_TO_SEND);
    Serial.print(F("Sent: "));
    printIRResultShort(&Serial, &aIRDataToSend->receivedIRData, false);
  }
}

const char* ssid     = "";
const char* password = "";

WebServer server(80);

void handleSendCode() {
  if (server.hasArg("index")) {
    int index = server.arg("index").toInt();
    if (index < 0 || index > 9) {
      server.send(400, "text/plain", "Invalid index. Use 0-9.");
      return;
    }
    IrReceiver.stop();
    sendCode(&sStoredIRData[index]);
    server.send(200, "text/plain", "IR Code sent.");
    IrReceiver.start();
  } else {
    server.send(400, "text/plain", "Missing 'index' parameter.");
  }
}

void handleRegisterCode() {
  if (server.hasArg("index")) {
    int index = server.arg("index").toInt();
    if (index < 0 || index > 9) {
      server.send(400, "text/plain", "Invalid index. Use 0-9.");
      return;
    }
    registerIndex = index;
    digitalWrite(pinLED, HIGH);
    server.send(200, "text/plain", ("Registration mode activated for slot " + String(index) + ". Press the remote button now.").c_str());
    Serial.print(F("Registration mode activated for slot "));
    Serial.println(index);
  } else {
    server.send(400, "text/plain", "Missing 'index' parameter.");
  }
}

void setupWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi.");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void setupHTTPServer() {
  server.on("/send", HTTP_GET, handleSendCode);
  server.on("/register", HTTP_GET, handleRegisterCode);
  server.begin();
  Serial.println("HTTP server started.");
}

void setup() {
  Serial.begin(115200);
  setupWiFi();
  setupHTTPServer();
  setupIR();
}

void loop() {
  server.handleClient();
  loopIR();
}

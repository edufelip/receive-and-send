#include <Arduino.h>

#define RAW_BUFFER_LENGTH 750
#define MARK_EXCESS_MICROS 20

#define pinLED 27

#include "PinDefinitionsAndMore.h"
#include <IRremote.hpp>            

struct storedIRDataStruct {
  IRData receivedIRData;              
  uint16_t rawCode[RAW_BUFFER_LENGTH]; 
  uint16_t rawCodeLength;             
} sStoredIRData[10];

int STATUS_PIN = 2;
int DEFAULT_NUMBER_OF_REPEATS_TO_SEND = 1;
int estadoAnt = 0;

void storeCode(IRData *aIRReceivedData, int index);
void sendCode(storedIRDataStruct *aIRDataToSend);

void setup() {
  pinMode(pinLED, OUTPUT);
  digitalWrite(pinLED, LOW);
  Serial.begin(115200);

  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
  IrSender.begin();
  
  pinMode(STATUS_PIN, OUTPUT);

  Serial.println(F("Para gravar digite '#' seguido da posicao entre 0 e 9"));
  Serial.println(F("Para enviar digite o numero entre 0 e 9 de um codigo ja gravado"));
}

void loop() {
  static int index = -1;
  static int estadoAtual = 0;

  if (Serial.available()) {
    char recebido = Serial.read();
    if (recebido == '#') {
      estadoAtual = 1;
      Serial.println(F("Modo Gravacao"));
      delay(50);
      recebido = Serial.read();
      index = atoi(&recebido);
      Serial.print(F("Aguardando Codigo #"));
      Serial.println(recebido);
      digitalWrite(pinLED, HIGH);
    }
    else if (recebido >= '0' && recebido <= '9') {
      Serial.println(F("Modo Envio"));
      estadoAtual = 2;
      index = atoi(&recebido);
    }
  }

  if (estadoAtual != estadoAnt && estadoAtual == 1) {
    IrReceiver.start();
  }

  if (estadoAtual == 2 && index >= 0) {
    IrReceiver.stop();
    Serial.println(F("Enviando..."));
    digitalWrite(STATUS_PIN, HIGH);
    sendCode(&sStoredIRData[index]);
    digitalWrite(STATUS_PIN, LOW);
    index = -1;
  }

  if (estadoAtual == 1 && IrReceiver.available()) {
    storeCode(IrReceiver.read(), index);
    IrReceiver.resume();
    digitalWrite(pinLED, LOW);
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
    Serial.print(F("Received unknown code and store "));
    uint16_t len = IrReceiver.decodedIRData.rawDataPtr->rawlen - 1;
    Serial.print(len);
    Serial.println(F(" timing entries as raw "));
    IrReceiver.printIRResultRawFormatted(&Serial, true);

    sStoredIRData[index].rawCodeLength = len;
    for (uint16_t i = 0; i < len; i++) {
      int16_t rawValue = IrReceiver.decodedIRData.rawDataPtr->rawbuf[i + 1];
      uint16_t value = abs(rawValue);
      if ((i % 2) == 0) {  // mark
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

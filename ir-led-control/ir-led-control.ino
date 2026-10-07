#include <IRremote.hpp>

#include "ir_codes.h"
#include "pins.h"

#define IR_CAPTURE_MODE 0

bool ledIsOn = false;

void setup() {
  pinMode(kOnboardLedPin, OUTPUT);
  digitalWrite(kOnboardLedPin, LOW);
  IrReceiver.begin(kIrReceiverPin, DISABLE_LED_FEEDBACK);
  pinMode(kIrReceiverPin, INPUT_PULLUP);

#if IR_CAPTURE_MODE
  Serial.begin(9600);
  Serial.println(F("IR capture ready"));
#endif
}

void loop() {
  if (IrReceiver.decode()) {
    if (!(IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT)
        && IrReceiver.decodedIRData.protocol == kPowerButtonProtocol
        && IrReceiver.decodedIRData.address == kPowerButtonAddress
        && IrReceiver.decodedIRData.command == kPowerButtonCommand) {
      ledIsOn = !ledIsOn;
      digitalWrite(kOnboardLedPin, ledIsOn ? HIGH : LOW);
    }
#if IR_CAPTURE_MODE
    Serial.print(F("protocol="));
    Serial.print(IrReceiver.getProtocolString());
    Serial.print(F(" protocol_id="));
    Serial.print(static_cast<int>(IrReceiver.decodedIRData.protocol));
    Serial.print(F(" address=0x"));
    Serial.print(IrReceiver.decodedIRData.address, HEX);
    Serial.print(F(" command=0x"));
    Serial.print(IrReceiver.decodedIRData.command, HEX);
    Serial.print(F(" raw=0x"));
    Serial.print(IrReceiver.decodedIRData.decodedRawData, HEX);
    Serial.print(F(" repeat="));
    Serial.println((IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT) ? 1 : 0);
#endif
    IrReceiver.resume();
  }
}

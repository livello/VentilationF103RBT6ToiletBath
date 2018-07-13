#include "Arduino.h"
#include "HardwareSerial.h"



void setup() {
    Serial.begin(115200);
    Serial1.begin(115200);
    Serial2.begin(115200);
    pinMode(PB8,OUTPUT);
    pinMode(PB9,OUTPUT);

}

void loop() {
  while(1){
      digitalWrite(PB8,LOW);
      digitalWrite(PB9,HIGH);
      delay(250);
      Serial.println("Hello0 from f103c8 ugly board!!!");
      Serial1.println("Hello1 from f103c8 ugly board!!!");
      Serial2.println("Hello2 from f103c8 ugly board!!!");
      digitalWrite(PB8,HIGH);
      digitalWrite(PB9,LOW);
      delay(250);

  }
}


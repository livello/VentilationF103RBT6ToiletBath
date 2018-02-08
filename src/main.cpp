#include "Arduino.h"
#include "HardwareSerial.h"
#include <EtherCard_STM.h>


#define UDP_BROADCAST_PORT (uint16_t)4000

const PROGMEM uint8 digitalInputPins[6] = {PB12, PB14,PB15,PB2,PA2,PA3};
const PROGMEM WiringPinMode digitalInputPinsMode[6] = {INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLDOWN,INPUT_PULLDOWN};
const PROGMEM uint8 solidStateRelayPins[4] = {PB11, PB10, PB1, PB0};
const PROGMEM WiringPinMode solidStateRelayPinsMode[4] = {OUTPUT, OUTPUT, OUTPUT, OUTPUT};

static byte mymac[] = {0x1A, 0x2B, 0x3C, 0x4D, 0x5E, 0x6F};
byte Ethernet::buffer[700];
static byte myip[] = {192, 168, 1, 181};
// gateway ip address
static byte gwip[] = {192, 168, 1, 110};

static byte broadcast_ip[] = {192, 168, 1, 255};
// domain name server (dns) address
static byte dnsip[] = {192, 168, 1, 110};
// remote website name
const char website[] PROGMEM = "google.com";
char textToSend[] = "test 123";

static uint32_t timer;
const int srcPort PROGMEM = 4321;
int checkNum = 0;
void blink(int channel){
//    digitalWrite(solidStateRelayPins[channel],!digitalRead(solidStateRelayPins[channel]));
    for(int i=0;i<sizeof(solidStateRelayPins);i++)
        digitalWrite(solidStateRelayPins[i],(i+checkNum)%2);
}
void checkDigitalInput(){
    blink(checkNum%4);
    checkNum++;

}
void setup() {
    Serial.begin(115200);
    if (ether.begin(sizeof Ethernet::buffer, mymac, PA4) == 0) {
        Serial.println("Failed to access Ethernet controller");
//    if (!ether.dhcpSetup()) {
//        Serial.println("DHCP failed");
//    }
    }
    ether.staticSetup(myip, gwip, dnsip);
    ether.copyIp(ether.dnsip, dnsip);
    for(int i=0;i<sizeof(digitalInputPins);i++){
        pinMode(digitalInputPins[i],digitalInputPinsMode[i]);
    }
    for(int i=0;i<sizeof(solidStateRelayPins);i++){
        pinMode(solidStateRelayPins[i],solidStateRelayPinsMode[i]);
    }
    Timer1.setPeriod(1000000); // set a timer of length 100000 microseconds (or 0.1 sec - or 10Hz => the led will blink 5 times, 5 cycles of on-and-off, per second)
    Timer1.attachInterrupt(1, checkDigitalInput ); // attach the service routine here
}

uint32_t nextSerialSend = 0;
void loop() {
    ether.packetLoop(ether.packetReceive());
    if (millis() > timer) {
        timer = millis() + 2000;
        //static void sendUdp (char *data,uint8_t len,uint16_t sport, uint8_t *dip, uint16_t dport);
        ether.sendUdp(textToSend, sizeof(textToSend), srcPort, broadcast_ip, UDP_BROADCAST_PORT);
    }
    if(millis()>nextSerialSend){
        Serial.print(checkNum);
        Serial.print(": I");
        for(int i=0;i<sizeof(digitalInputPins);i++){
            digitalRead(digitalInputPins[i])?Serial.print("+"):Serial.print("-");
        }
        Serial.print(" R");
        for(int i=0;i<sizeof(solidStateRelayPins);i++){

            digitalRead(solidStateRelayPins[i])?Serial.print("+"):Serial.print("-");
        }
        Serial.println();
        nextSerialSend=millis()+1000;
    }
}

#include "Arduino.h"
#include "HardwareSerial.h"
#include <EtherCard_STM.h>

#define UDP_BROADCAST_PORT (uint16_t)4000

static byte mymac[] = { 0x1A,0x2B,0x3C,0x4D,0x5E,0x6F };
byte Ethernet::buffer[700];
static byte myip[] = { 192,168,1,181 };
// gateway ip address
static byte gwip[] = { 192,168,1,110 };

static byte broadcast_ip[] = { 192,168,1,255 };
// domain name server (dns) address
static byte dnsip[] = { 192,168,1,110 };
// remote website name
const char website[] PROGMEM = "google.com";
char textToSend[] = "test 123";

static uint32_t timer;
const int srcPort PROGMEM = 4321;

void setup () {
    Serial.begin(115200);

    if (ether.begin(sizeof Ethernet::buffer, mymac) == 0)
        Serial.println( "Failed to access Ethernet controller");
//    if (!ether.dhcpSetup()) {
//        Serial.println("DHCP failed");
//    }
    ether.staticSetup(myip, gwip);
    ether.copyIp(ether.dnsip, dnsip);

    ether.printIp("IP:  ", ether.myip);
    ether.printIp("GW:  ", ether.gwip);
    ether.printIp("DNS: ", ether.dnsip);

    if (!ether.dnsLookup(website))
        Serial.println("DNS failed");

    ether.printIp("SRV: ", ether.hisip);
}



void loop () {
    ether.packetLoop(ether.packetReceive());
    if (millis() > timer) {
        timer = millis() + 2000;
        //static void sendUdp (char *data,uint8_t len,uint16_t sport, uint8_t *dip, uint16_t dport);
        ether.sendUdp(textToSend, sizeof(textToSend), srcPort, broadcast_ip, UDP_BROADCAST_PORT);
    }
}

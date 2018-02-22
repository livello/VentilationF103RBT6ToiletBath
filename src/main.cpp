#include "Arduino.h"
#include "HardwareSerial.h"
#include <EtherCard_STM.h>

#define MIN_PRESS_TIME 500
#define UDP_BROADCAST_PORT (uint16_t)4000


static byte mymac[] = {0x1A, 0x2B, 0x3C, 0x4D, 0x5E, 0x6F};
byte Ethernet::buffer[900];
static byte myip[] = {192, 168, 10, 121};
// gateway ip address
static byte gwip[] = {192, 168, 10, 1};

static byte broadcast_ip[] = {255, 255, 255, 255};
// domain name server (dns) address
static byte dnsip[] = {192, 168, 10, 1};
// remote website name
const char website[] PROGMEM = "google.com";
char textToSend[] = "test 123";
static uint32_t timeUdpSend = 0;
const int srcPort PROGMEM = 4321;
char message_buffer[100];
char portStatesString[]="------";
char pageState[] =
        "<html>\n"
                "<head>\n"
                "<meta http-equiv=\"refresh\" content=\"30\">\n"
                "<title>Zelectro. Relay + Ethernet shield.</title>\n"
                "</head>\n"
                "<body>\n"
                "<h3>Zelectro. Relay + Ethernet shield.</h3>\n"
                "<form method='get'>\n"
                "<div>Relay 1 <input type='checkbox'  name='r0'></div>\n"
                "<div>Relay 2 <input type='checkbox' checked name='r1'></div>\n"
                "<div>Relay 3 <input type='checkbox' checked name='r2'></div>\n"
                "<div>Relay 4 <input type='checkbox' checked name='r3'></div>\n"
                "<input type='submit' value='Refresh'>\n"
                "</form>\n"
                "PortStates:\n"
                "@ABCDEF@\n"
                "!REL! \n"
                "</html>";


const volatile PROGMEM uint8 digitalInputPins[6] = {PB12, PB14, PB15, PB2, PA2, PA3};
const volatile PROGMEM WiringPinMode digitalInputPinsMode[6] = {INPUT_PULLUP, INPUT_PULLUP, INPUT_PULLUP, INPUT_PULLUP,
                                                                INPUT_PULLDOWN, INPUT_PULLDOWN};
const volatile PROGMEM uint8 solidStateRelayPins[4] = {PB11, PB10, PB1, PB0};
const volatile PROGMEM WiringPinMode solidStateRelayPinsMode[4] = {OUTPUT, OUTPUT, OUTPUT, OUTPUT};

bool digitalInputState[6] = {0, 0, 0, 0, 0, 0}, nextInputState[6] = {0, 0, 0, 0, 0, 0};
bool currentStateBuffer = 0;
uint64_t willChangeTime[6] = {0, 0, 0, 0, 0, 0};

void checkDigitalInput();

bool isChangeRequested(int);

bool isRequestNew(int);

bool isChangeTimeOut(int);

bool isOnDigitalInput(int);


void checkDigitalInput() {
    for (int i = 0; i < 6; i++) {
        currentStateBuffer = isOnDigitalInput(i);
        if (isChangeRequested(i)) {
            if (isRequestNew(i)) {
                nextInputState[i] = currentStateBuffer;
                willChangeTime[i] = millis() + MIN_PRESS_TIME;
                continue;
            } else if (isChangeTimeOut(i)) {
                digitalInputState[i] = nextInputState[i];
                willChangeTime[i] = 0;
                portStatesString[i]=digitalInputState[i]?'+':'-';
            }

        } else {
            willChangeTime[i] = 0;
            nextInputState[i] = digitalInputState[i];
        }
    }
}


bool isOnDigitalInput(int index) { return digitalRead(digitalInputPins[index]); }

bool isChangeTimeOut(int i) { return willChangeTime[i] < millis(); }

bool isRequestNew(int i) { return nextInputState[i] != currentStateBuffer; }

bool isChangeRequested(int i) { return digitalInputState[i] != currentStateBuffer; }


void sendRelayControlPageToEthernetClient();


void sendPageState();

void blink(int channel) {
    digitalWrite(solidStateRelayPins[channel], !digitalRead(solidStateRelayPins[channel]));
//    for(int i=0;i<sizeof(solidStateRelayPins);i++)
//        digitalWrite(solidStateRelayPins[i],(i+checkNum)%2);
}


void setup() {
    Serial.begin(460800);
    Serial.println("Hello from f103c8 ugly board!!!");
    delay(1);
    Serial.println("Hello from f103c8 ugly board Again!!!");
    if (ether.begin(sizeof Ethernet::buffer, mymac, PA4) == 0) {
        Serial.println("Failed to access Ethernet controller");
    }
    if (!ether.dhcpSetup()) {
        Serial.println("DHCP failed");
        ether.staticSetup(myip, gwip, dnsip);
    }
    ether.copyIp(ether.dnsip, dnsip);
    for (int i = 0; i < sizeof(digitalInputPins); i++) {
//        modbusIP.addIsts(1000+i,0);
        pinMode(digitalInputPins[i], digitalInputPinsMode[i]);
    }
    for (int i = 0; i < sizeof(solidStateRelayPins); i++) {
//        modbusIP.addCoil(1100+i,0);
        pinMode(solidStateRelayPins[i], solidStateRelayPinsMode[i]);
    }


    Timer1.setPeriod(
            100000); // set a timer of length 100000 microseconds (or 0.1 sec - or 10Hz => the led will blink 5 times, 5 cycles of on-and-off, per second)
    Timer1.attachInterrupt(1, checkDigitalInput); // attach the service routine here

}


uint32_t nextSerialSend = 0;

void loop() {
//    ether.packetLoop(ether.packetReceive());
//    modbusIP.task();
    if (millis() > timeUdpSend) {
        timeUdpSend = millis() + 5000;
        //static void sendUdp (char *data,uint8_t len,uint16_t sport, uint8_t *dip, uint16_t dport);
//        sprintf(message_buffer, "%ld %s", millis(),textToSend);
        ether.sendUdp(message_buffer, sizeof(message_buffer), srcPort, broadcast_ip, UDP_BROADCAST_PORT);
    }
    if (millis() > nextSerialSend) {
        Serial.print(millis());
        Serial.print(": DI");
        for (int i = 0; i < sizeof(digitalInputPins); i++) {
            (digitalInputState[i]) ? Serial.print("+") : Serial.print("-");
        }
        Serial.print(": I");
        for (int i = 0; i < sizeof(digitalInputPins); i++) {
            digitalRead(digitalInputPins[i]) ? Serial.print("+") : Serial.print("-");
        }
        Serial.print(" R");
        for (int i = 0; i < sizeof(solidStateRelayPins); i++) {

            digitalRead(solidStateRelayPins[i]) ? Serial.print("+") : Serial.print("-");
        }
        Serial.print(message_buffer);
        Serial.println();
        nextSerialSend = millis() + 250;
    }
    word pos = ether.packetLoop(ether.packetReceive());
    // check if valid tcp data is received
    if (pos) {
        char *data = (char *) Ethernet::buffer + pos;
        memccpy(&message_buffer, data, 10, 90);
        if (strncmp("GET / ", data, 6) == 0) {
            sendPageState();
        } else {
            sendPageState();
        }
    }
}

void sendPageState() {
    ether.httpServerReplyAck(); // send ack to the request
    memcpy_P(ether.tcpOffset(), pageState, sizeof pageState); // send fiveth packet and send the terminate flag
    ether.httpServerReply_with_flags(sizeof pageState - 1, TCP_FLAGS_ACK_V | TCP_FLAGS_FIN_V);
}




void sendRelayControlPageToEthernetClient() {
}

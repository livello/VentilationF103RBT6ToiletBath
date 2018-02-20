#include "Arduino.h"
#include "HardwareSerial.h"
#include <EtherCard_STM.h>
#include <EthernetClient.h>
#include <EthernetServer.h>

#define MIN_PRESS_TIME 500
#define UDP_BROADCAST_PORT (uint16_t)4000




static byte mymac[] = {0x1A, 0x2B, 0x3C, 0x4D, 0x5E, 0x6F};
byte Ethernet::buffer[700];
static byte myip[] = {192, 168, 10, 121};
// gateway ip address
static byte gwip[] = {192, 168, 10, 1};

static byte broadcast_ip[] = {192, 168, 10, 255};
// domain name server (dns) address
static byte dnsip[] = {192, 168, 10, 1};
// remote website name
const char website[] PROGMEM = "google.com";
char textToSend[] = "test 123";
static uint32_t timer=0;
const int srcPort PROGMEM = 4321;


EthernetServer server(80);
EthernetClient client;


const volatile PROGMEM uint8 digitalInputPins[6] = {PB12, PB14,PB15,PB2,PA2,PA3};
const volatile PROGMEM WiringPinMode digitalInputPinsMode[6] = {INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLDOWN,INPUT_PULLDOWN};
const volatile PROGMEM uint8 solidStateRelayPins[4] = {PB11, PB10, PB1, PB0};
const volatile PROGMEM WiringPinMode solidStateRelayPinsMode[4] = {OUTPUT, OUTPUT, OUTPUT, OUTPUT};
int checkNum = 0;

bool digitalInputState[6]={0, 0, 0, 0, 0, 0}, nextInputState[6]={0, 0, 0, 0, 0, 0};
bool currentStateBuffer = 0;
uint64_t willChangeTime[6]={0,0,0,0,0,0};

void blinkCycle();

void checkDigitalInput();
bool isChangeRequested(int);
bool isRequestNew(int);
uint32_t changeTimeRemained(int);
bool isOnDigitalInput(int);
void sendRelayControlPageToEthernetClient();

void checkDigitalInput() {
    for (int i = 0; i < 6; i++) {
        currentStateBuffer = isOnDigitalInput(i);
        if (isChangeRequested(i)) {
            if (isRequestNew(i)) {
                nextInputState[i] = currentStateBuffer;
                willChangeTime[i] = millis() + MIN_PRESS_TIME;
                continue;
            } else if (changeTimeRemained(i) < 0) {
                digitalInputState[i] = nextInputState[i];
//                modbusIP.Ists(1000+i,digitalInputState[i]);
                willChangeTime[i] = 0;
//                blink(i);
            }

        } else {
            willChangeTime[i] = 0;
            nextInputState[i] = digitalInputState[i];

        }
    }
    blinkCycle();
}

bool isOnDigitalInput(int index) { return (0!=digitalRead(digitalInputPins[index])); }

uint32_t changeTimeRemained(int i) { return willChangeTime[i] - millis(); }

bool isRequestNew(int i) { return nextInputState[i] != currentStateBuffer; }

bool isChangeRequested(int i) { return digitalInputState[i] != currentStateBuffer; }


void blink(int channel){
    digitalWrite(solidStateRelayPins[channel],!digitalRead(solidStateRelayPins[channel]));
//    for(int i=0;i<sizeof(solidStateRelayPins);i++)
//        digitalWrite(solidStateRelayPins[i],(i+checkNum)%2);
}
void blinkCycle(){
    blink(checkNum%4);
    checkNum++;

}



void setup() {
    Serial.begin(115200);
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
    for(int i=0;i<sizeof(digitalInputPins);i++){
//        modbusIP.addIsts(1000+i,0);
        pinMode(digitalInputPins[i],digitalInputPinsMode[i]);
    }
    for(int i=0;i<sizeof(solidStateRelayPins);i++){
//        modbusIP.addCoil(1100+i,0);
        pinMode(solidStateRelayPins[i],solidStateRelayPinsMode[i]);
    }


    server.begin();
    Timer1.setPeriod(1000000); // set a timer of length 100000 microseconds (or 0.1 sec - or 10Hz => the led will blink 5 times, 5 cycles of on-and-off, per second)
    Timer1.attachInterrupt(1, checkDigitalInput ); // attach the service routine here


}



uint32_t nextSerialSend = 0;
void loop() {
    ether.packetLoop(ether.packetReceive());
//    modbusIP.task();
    if (millis() > timer) {
        timer = millis() + 200;
        //static void sendUdp (char *data,uint8_t len,uint16_t sport, uint8_t *dip, uint16_t dport);
        ether.sendUdp(textToSend, sizeof(textToSend), srcPort, broadcast_ip, UDP_BROADCAST_PORT);
    }
    if(millis()>nextSerialSend){
        Serial.print(millis());
        Serial.print(": DI");
        for(int i=0;i<sizeof(digitalInputPins);i++){
            (digitalInputState[i])?Serial.print("+"):Serial.print("-");
        }
        Serial.print(": I");
        for(int i=0;i<sizeof(digitalInputPins);i++){
            digitalRead(digitalInputPins[i])?Serial.print("+"):Serial.print("-");
        }
        Serial.print(" R");
        for(int i=0;i<sizeof(solidStateRelayPins);i++){

            digitalRead(solidStateRelayPins[i])?Serial.print("+"):Serial.print("-");
        }
        Serial.println();
        nextSerialSend=millis()+250;
    }
    client = server.available();

    if (client) {
        String webRequestType = client.readStringUntil('/');
        if (webRequestType.compareTo("GET ") == 0) {
            sendRelayControlPageToEthernetClient();
            String requestedFileName = client.readStringUntil(' ');

            Serial.println("GET");
            Serial.println(requestedFileName);
            if (requestedFileName.length() == 0){

            }
        } else if (webRequestType.compareTo("POST ") == 0) {
            String clientRequest = client.readString();
            if (clientRequest.indexOf("r0=on")>0)
                digitalWrite(solidStateRelayPins[0],1);
            else
                digitalWrite(solidStateRelayPins[0],0);
            if (clientRequest.indexOf("r1=on")>0)
                digitalWrite(solidStateRelayPins[1],1);
            else
                digitalWrite(solidStateRelayPins[1],0);
            if (clientRequest.indexOf("r2=on")>0)
                digitalWrite(solidStateRelayPins[2],1);
            else
                digitalWrite(solidStateRelayPins[2],1);
            if (clientRequest.indexOf("r3=on")>0)
                digitalWrite(solidStateRelayPins[3],0);
            else
                digitalWrite(solidStateRelayPins[3],1);


            sendRelayControlPageToEthernetClient();
        } else {
            Serial.println("." + webRequestType + ".");
            Serial.println("" + client.readString());
            sendRelayControlPageToEthernetClient();
        }
        client.stop();
    }

}
void sendRelayControlPageToEthernetClient() {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println();
    client.println("<html>");
    client.println("<head>");
    client.println( "<meta http-equiv=\"refresh\" content=\"30\">");
    client.println("<title>Zelectro. Relay + Ethernet shield.</title>");
    client.println("</head>");
    client.println("<body>");
    client.println("<h3>Zelectro. Relay + Ethernet shield.</h3>");
    client.println("<form method='post'>");
    client.print("<div>Relay 1 <input type='checkbox' ");
    if (digitalRead(solidStateRelayPins[0]) == 1)
        client.print("checked");
    client.println(" name='r0'></div>");
    client.print("<div>Relay 2 <input type='checkbox' ");
    if (digitalRead(solidStateRelayPins[1]) == 1)
        client.print("checked");
    client.println(" name='r1'></div>");
    client.print("<div>Relay 3 <input type='checkbox' ");
    if (digitalRead(solidStateRelayPins[1]) == 1)
        client.print("checked");
    client.println(" name='r2'></div>");
    client.print("<div>Relay 4 <input type='checkbox' ");
    if (digitalRead(solidStateRelayPins[1]) == 1)
        client.print("checked");
    client.println(" name='r3'></div>");
    client.println("<input type='submit' value='Refresh'>");
    client.println("</form>");
    client.println("Temperature:");
//    client.print(temp_DS18B20);
    client.print(" celsius, ");
//    client.print(temp_DHT22);
    client.print(" celsius, humidity(%):");
//    client.print(humidity_DHT22);
    client.println("</body>");
    client.println("</html>");
}

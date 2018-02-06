#include "mbed.h"
#include "UIPEthernet.h"

#define MIN_PRESS_TIME 500

DigitalOut myled(PB_8);
DigitalOut SolidStayRelay[4] = {PB_11, PC_4, PB_0, PB_2};
DigitalIn digitalInput[6] = {PC_7, PB_15, PA_4, PA_0, PB_14, PB_13};
bool digitalInputState[6];
uint32_t stateChangedMillis[6];
Ticker ticker1, ticker2, ticker3;

static volatile uint32_t millisValue = 0;

void blink(int index) {
    if (index < 0 || index > 3)
        myled = !myled;
    else
        SolidStayRelay[index] = !SolidStayRelay[index];
}

uint32_t millis1() {
    return millisValue;
}

void checkDigitalInput() {
    bool currentStatei = 0;
    for (int i = 0; i < 6; i++) {
        currentStatei = digitalInput[i].read();
        if (digitalInputState[i] != currentStatei) {
            if (millis1() - stateChangedMillis[i] > MIN_PRESS_TIME) {
                blink(i);
                digitalInputState[i] = currentStatei;
            }

        } else
            stateChangedMillis[i] = millis1();
    }
}

void millisTicker() {
    millisValue++;
}


// MAC address must be unique within the connected network. Modify as appropriate.
const uint8_t MY_MAC[6] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
// IP address must be unique and compatible with your network.
const IPAddress MY_IP(192, 168, 1, 181);
const uint16_t MY_PORT = 7;
const char *message = "Hello World from mbed!";

Serial pc(USBTX, USBRX);
UIPEthernet uIPEthernet(D11, D12, D13, D10);    // mosi, miso, sck, cs
UIPUDP udp;

void setupBoard();

/**
 * @brief
 * @note
 * @param
 * @retval
 */
int main(void) {
    setupBoard();

    while (1) {
        int success;
        int size = udp.parsePacket();

        if (size > 0) {
            do {
                char *msg = (char *) malloc(size + 1);
                int len = udp.read(msg, size + 1);
                msg[len] = 0;
                printf("received: '%s", msg);
                free(msg);
            } while ((size = udp.available()) > 0);

            //finish reading this packet:
            udp.flush();
            printf("'\r\n");

            do {
                //send new packet back to ip/port of client. This also
                //configures the current connection to ignore packets from
                //other clients!
                success = udp.beginPacket(udp.remoteIP(), udp.remotePort());
                if (success)
                    pc.printf("beginPacket: succeeded%\r\n");
                else
                    pc.printf("beginPacket: failed%\r\n");

                //beginPacket fails if remote ethaddr is unknown. In this case an
                //arp-request is send out first and beginPacket succeeds as soon
                //the arp-response is received.
            } while (!success);

            success = udp.write((uint8_t *) message, strlen(message));

            if (success)
                pc.printf("bytes written: %d\r\n", success);

            success = udp.endPacket();

            if (success)
                pc.printf("endPacket: succeeded%\r\n");
            else
                pc.printf("endPacket: failed%\r\n");

            udp.stop();

            //restart with new connection to receive packets from other clients
            if (udp.begin(MY_PORT))
                pc.printf("restart connection: succeeded%\r\n");
            else
                pc.printf("restart connection: failed%\r\n");
        }
    }
}

void setupBoard() {
    for (int i = 0; i < 6; i++)
        digitalInput[i].mode(PushPullPullUp);

    //ticker1.attach(&blink, 5);
    ticker2.attach(&millisTicker, 0.001);
    ticker3.attach(&checkDigitalInput, 0.02);
    uIPEthernet.begin(MY_MAC, MY_IP);

    IPAddress localIP = uIPEthernet.localIP();
    pc.printf("Local IP = %s\r\n", localIP.toString());
    pc.printf("Initialization ");
    if (udp.begin(MY_PORT))
        pc.printf("succeeded.\r\n");
    else
        pc.printf("failed.\r\n");


}


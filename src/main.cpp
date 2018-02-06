#include <rtos/TARGET_CORTEX/rtx5/RTX/Include/rtx_os.h>
#include "mbed.h"
#include "UIPEthernet.h"

#define MIN_PRESS_TIME 500
#define CHECK_INPUT_PERIOD 10000
#define UDP_BROADCAST_PORT 4000

DigitalOut myled(PB_8);
DigitalOut SolidStayRelay[4] = {PB_11, PC_4, PB_0, PB_2};
DigitalIn digitalInput[6] = {PC_7, PB_15, PA_4, PA_0, PB_14, PB_13};
bool digitalInputState[6], nextInputState[6];
uint64_t willChangeTime[6];
uint64_t nextCheckDigitalInput = 0;

// MAC address must be unique within the connected network. Modify as appropriate.
const uint8_t MY_MAC[6] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
// IP address must be unique and compatible with your network.
const IPAddress MY_IP(192, 168, 1, 181);
const IPAddress BROADCAST_IP(192, 168, 1, 255);
const uint16_t MY_PORT = 7;
char *udp_message = "Hello from mbed";
uint32_t mes_num = 0;
bool now_send_udp=0;

Serial pc(USBTX, USBRX);
UIPEthernet uIPEthernet(D11, D12, D13, D10);    // mosi, miso, sck, cs
UIPUDP udp_receive, udp_send;

static volatile uint64_t millisValue = 0;

void setupBoard();
void blink(int);
void blink1();
uint64_t millis1();
void check_udp_print();
bool changeRequested(bool currentStatei, int i);
bool isRequestNew(bool currentStatei, int i);
uint32_t changeTimeRemained(int i);

#define SYSTICK_RELOAD_VAL 0x300000   //  48 * 0x10000    4799999
#define SYSTICK_DIV(x)    (((x>>4)*0x5555)>>16)


void SysTick_Handler(void) {
    millisValue++;                        // increment counter
}

void systick_init( void ) {
    // SysTick counter, interrupt every 65 ms
    SysTick->LOAD = SYSTICK_RELOAD_VAL-1;
    SysTick->VAL  = 0;
    SysTick->CTRL  = 4 | 2 | 1; //CLKSOURCE=CPU clock | TICKINT | ENABLE
    millisValue = 0;
}

unsigned int micros( void )
{
    unsigned int Tick;
    unsigned int count;

    do {
        count = millisValue;
        Tick = SysTick->VAL;
    } while (SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk);
    return (count << 16) + (0x10000 - SYSTICK_DIV(Tick));
}

void blink(int index) {
    if (index < 0 || index > 3)
        myled = !myled;
    else
        SolidStayRelay[index] = !SolidStayRelay[index];
}

void blink1() {
    blink(5);
    blink(1);
}

uint64_t millis1() {
    return millisValue;
}



void checkDigitalInput() {
    bool currentStatei = 0;
    for (int i = 0; i < 6; i++) {
        currentStatei = digitalInput[i].read();
        if (changeRequested(currentStatei, i)) {
            if (isRequestNew(currentStatei, i)) {
                nextInputState[i] = currentStatei;
                willChangeTime[i] = millis1() + MIN_PRESS_TIME;
                continue;
            } else if (changeTimeRemained(i) < 0) {
                digitalInputState[i] = nextInputState[i];
                willChangeTime[i] = 0;
                blink(i);
            }

        } else {
            willChangeTime[i] = 0;
            nextInputState[i] = digitalInputState[i];

        }
    }
}

uint32_t changeTimeRemained(int i) { return willChangeTime[i] - millis1(); }

bool isRequestNew(bool currentStatei, int i) { return nextInputState[i] != currentStatei; }

bool changeRequested(bool currentStatei, int i) { return digitalInputState[i] != currentStatei; }

void millisTicker() {
    millisValue++;
    if (millisValue > nextCheckDigitalInput) {
        nextCheckDigitalInput = millis1() + CHECK_INPUT_PERIOD;
        checkDigitalInput();
//        now_send_udp=1;
    }

}






void check_udp_print() {
    if(!now_send_udp)
        return;
    now_send_udp=0;
//    if (strlen(udp_message) <= 0)
//        return;
//    mes_num++;
//    char *message_buffer = (char *) malloc(strlen(udp_message) + 30);;
//    sprintf(message_buffer, "%ld_%ld:%s", (long) mes_num, (long) millis1(), udp_message);
    udp_send.beginPacket(BROADCAST_IP, UDP_BROADCAST_PORT);
    //udp_send.write((uint8_t *) message_buffer, strlen(message_buffer));
    udp_send.write((uint8_t *) udp_message, strlen(udp_message));
    udp_send.endPacket();

//    udp_message[0] = '\0';
}
int main(void) {
    setupBoard();

    while (1) {
//        check_udp_print();
        int success;
        int size = udp_receive.parsePacket();

        if (size > 0) {
            do {
                char *msg = (char *) malloc(size + 1);
                int len = udp_receive.read(msg, size + 1);
                msg[len] = 0;
                printf("received: '%s", msg);
                free(msg);
            } while ((size = udp_receive.available()) > 0);

            //finish reading this packet:
            udp_receive.flush();
            printf("'\r\n");

            do {
                //send new packet back to ip/port of client. This also
                //configures the current connection to ignore packets from
                //other clients!
                success = udp_receive.beginPacket(udp_receive.remoteIP(), udp_receive.remotePort());
                if (success)
                    pc.printf("beginPacket: succeeded%\r\n");
                else
                    pc.printf("beginPacket: failed%\r\n");

                //beginPacket fails if remote ethaddr is unknown. In this case an
                //arp-request is send out first and beginPacket succeeds as soon
                //the arp-response is received.
            } while (!success);

            success = udp_receive.write((uint8_t *) udp_message, strlen(udp_message));

            if (success)
                pc.printf("bytes written: %d\r\n", success);

            success = udp_receive.endPacket();

            if (success)
                pc.printf("endPacket: succeeded%\r\n");
            else
                pc.printf("endPacket: failed%\r\n");

            udp_receive.stop();

            //restart with new connection to receive packets from other clients
            if (udp_receive.begin(MY_PORT))
                pc.printf("restart connection: succeeded%\r\n");
            else
                pc.printf("restart connection: failed%\r\n");
        }
    }
}
osRtxTimer_t myTimer;
void setupBoard() {
//    TIM1->PSC = (SystemCoreClock / 1000000) - 1;
    systick_init();
    for (int i = 0; i < 6; i++)
        digitalInput[i].mode(PushPullPullUp);

    uIPEthernet.begin(MY_MAC, MY_IP);

    IPAddress localIP = uIPEthernet.localIP();
//    udp_send.begin(UDP_BROADCAST_PORT);
    pc.printf("Local IP = %s\r\n", localIP.toString());
    pc.printf("Initialization ");
    if (udp_receive.begin(MY_PORT))
        pc.printf("succeeded.\r\n");
    else
        pc.printf("failed.\r\n");


}




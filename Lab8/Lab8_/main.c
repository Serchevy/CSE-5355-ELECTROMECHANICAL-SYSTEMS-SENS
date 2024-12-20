#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "wait.h"
#include "gpio.h"
#include "clock.h"
#include "tm4c123gh6pm.h"
#include "uart0.h"
#include "uart1.h"

#define CLEAR_PUTTY     "\033[2J"
#define HOME_POS        "\033[H"
#define HIDE_CURSOR     "\033[?25l"
#define SHOW_CURSOR     "\033[?25h"
#define CLEAR_LINE      "\033[2K"
#define SAVE_POS        "\033[s"
#define RESTORE_POS     "\033[u"

#define PWM             PORTF,3

#define RQST            0xA5
#define STOP            0x25
#define INFO            0x50
#define SCAN            0x20

#define SIZE            260

bool start = false;
bool endScan = false;
bool valid = false;

typedef struct _LIDAR {
    float angle;
    float dist;
} LIDAR;
LIDAR lidar_data[SIZE] = {0};

uint8_t count = 0;


void initHw()
{
    // Initialize system clock to 40 MHz
    initSystemClockTo40Mhz();

    initUart0();
    setUart0BaudRate(115200, 40e6);

    initUart1();
    setUart1BaudRate(115200, 40e6);

    enablePort(PORTF);
    selectPinPushPullOutput(PWM);
    setPinAuxFunction(PWM, 5);

    SYSCTL_RCGCPWM_R |= SYSCTL_RCGCPWM_R1;
    _delay_cycles(3);

    // GREEN on M1PWM7 (PF3), M1PWM3b
    SYSCTL_SRPWM_R = SYSCTL_SRPWM_R1;               // reset PWM1 module
    SYSCTL_SRPWM_R = 0;                             // leave reset state
    PWM1_3_CTL_R = 0;                               // turn-off PWM1 generator 3 (drives outs 6 and 7)
    PWM1_3_GENB_R = PWM_1_GENB_ACTCMPBD_ONE | PWM_1_GENB_ACTLOAD_ZERO;
                                                    // output 7 on PWM1, gen 3b, cmpb
    PWM1_3_LOAD_R = 1024;
    PWM1_3_CMPB_R = 0;                              // Green LED off

    PWM1_3_CTL_R = PWM_1_CTL_ENABLE;                // turn-on PWM1 generator 3
    PWM1_ENABLE_R = PWM_ENABLE_PWM7EN;              // Enalbe output
}

void stop_info() {
    uint8_t info[27] = {0}; // 27 bytes expected
    uint8_t i;
    char str[20];

    putcUart1(RQST);
    putcUart1(STOP);        // Stop
    waitMicrosecond(1000);
    putcUart1(RQST);
    putcUart1(INFO);        // Info

    getBytesUart1(info, 27);

    // Print Response Descriptor
    for(i=0; i<7; i++) {
        itos(info[i], str, 1, 2);       // itos(number, format string, hex num?(T/F), number of digits (if hex num))
        if(info[0]==0xA5 && info[1]==0x5A)
            valid = true;
        putsUart0(str);
        putsUart0(" ");
    }
    putsUart0("\n");

    // Print data
    for(i=i; i<27; i++) {
        itos(info[i], str, 1, 2);
        putsUart0(str);
        putsUart0(" ");
    }
    putsUart0("\n\n");
}

void scan() {
    uint8_t scan[5] = {0};      // 5 Bytes expected (Quality, Angle & Distance)

    getBytesUart1(scan, 5);

    if(!(scan[1] & 0x01))       // Verify check bit (always 1) for data validation
        return;

    if(scan[0] & 0x01 & scan[0] & ~0x02) {        // 1st start bit means 1st 360 scan, 2nd start bit means new 360 scan (pg. 18)
        if(start) {
            endScan = true;     // Detected a 2nd start bit, end scan
            return;
        }
        start = true;           // Set start flag
    }

    if(!start)                  // Wait until 1st start bit is set to process data
        return;

    uint16_t angle_raw = ((scan[2]<<8) | scan[1])>>1;
    uint16_t dist_raw = (scan[4]<<8) | scan[3];

    float angle = angle_raw/64.0;
    float dist = dist_raw/4.0;

    if(dist < 1)
        return;

    lidar_data[count].angle = angle;
    lidar_data[count].dist = dist;

    count++;
}

int main(void)
{
    initHw();

    uint8_t response[7] = {0};
    uint16_t i;
    char str[50];

    putsUart0(CLEAR_PUTTY);
    putsUart0(HOME_POS);

    while(!valid)
        stop_info();                        // Send STOP & INFO command, then display output

    //setPinValue(PWM, 1);                // Drive motor
    PWM1_3_CMPB_R = 1023;
    waitMicrosecond(1e6);               // Wait for a second to achieve a consistent speed (maybe wait longer)

    putcUart1(RQST);
    putcUart1(SCAN);                    // send SCAN command

    getBytesUart1(response, 7);         // Get response descriptor, packets after descriptor are continuous 5 bytes of data (Quality, Angle & Distance)

    while(true) {

        scan();                         // Read data and display it

        if(endScan) {
            putcUart1(RQST);
            putcUart1(STOP);            // Stop scan
            PWM1_3_CMPB_R = 0;
            break;
        }
    }

    for(i=0; i<7; i++) {                // Print Response Descriptor (idk if needed)
        itos(response[i], str, 1, 2);
        putsUart0(str);
        putsUart0(" ");
    }
    putsUart0("\n");

    float sum = 0;

    putsUart0("\nAngle\tDistance\n");
    for(i=0; i<SIZE; i++) {
        //snprintf(str, sizeof(str), "Angle: %.2f, Distance: %.2f mm\n", lidar_data[i].angle, lidar_data[i].dist);
        if(i>0)
        {
            sum += lidar_data[i].dist * lidar_data[i-1].dist * 0.5 * sin((lidar_data[i].angle - lidar_data[i-1].angle) * (M_PI / 180.0));
        }

        if(lidar_data[i].dist != 0)
        {
        snprintf(str, sizeof(str), "%.2f\t%.2f\n", lidar_data[i].angle, lidar_data[i].dist);
        putsUart0(str);
        }
    }

    sum *= 0.00150;

    putsUart0("\n");
    snprintf(str, sizeof(str), "Room Area: %.2f in^2\n", sum);
    putsUart0(str);

    while(true);
}

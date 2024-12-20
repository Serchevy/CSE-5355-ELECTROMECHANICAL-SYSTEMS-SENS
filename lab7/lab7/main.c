
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "wait.h"
#include "gpio.h"
#include "clock.h"
#include "tm4c123gh6pm.h"
#include "uart0.h"
#include "uart0ext.h"

#define CLEAR_PUTTY     "\033[2J"
#define HOME_POS        "\033[H"
#define HIDE_CURSOR     "\033[?25l"
#define SHOW_CURSOR     "\033[?25h"
#define CLEAR_LINE      "\033[2K"
#define SAVE_POS        "\033[s"
#define RESTORE_POS     "\033[u"

#define CLK     PORTB, 5
#define DATA    PORTB, 0

#define AVG     50

float mass = 0;
bool set = 0;
float force = 0;

void initHw()
{
    initSystemClockTo40Mhz();
    initUart0();
    setUart0BaudRate(115200, 40e6);

    enablePort(PORTB);
    selectPinPushPullOutput(CLK);
    selectPinDigitalInput(DATA);

}

uint32_t readData() {
    uint32_t rawData = 0;
    uint8_t i;

    while(getPinValue(DATA));       // POLL data pin till it's low

    // 24 clocks, and a 25th to indicate gain
    for(i=0; i<25; i++) {
        setPinValue(CLK, 1);
        waitMicrosecond(30);

        if(i<24)    // Don't read 25th clock
            rawData = (rawData << 1) | getPinValue(DATA);

        waitMicrosecond(1);
        setPinValue(CLK, 0);
    }

    return rawData;
}

void printStuff(uint64_t avg, int64_t diff, uint32_t max, float mass, float force) {
    char str[50];

    putsUart0(SAVE_POS);
    putsUart0(HOME_POS);

    snprintf(str, sizeof(str), "Raw Average: %llu\n", avg);
    putsUart0(str);

    putsUart0(CLEAR_LINE);
    snprintf(str, sizeof(str), "Difference: %lld\n", diff);
    putsUart0(str);

    putsUart0(CLEAR_LINE);
    snprintf(str, sizeof(str), "Max: %d\n", max);
    putsUart0(str);

    putsUart0(CLEAR_LINE);
    snprintf(str, sizeof(str), "Mass(g): %.2f\n", mass);
    putsUart0(str);

    putsUart0(CLEAR_LINE);
    snprintf(str, sizeof(str), "Force(N): %.2f\n", force);
    putsUart0(str);

    putsUart0(RESTORE_POS);
}

/**
 * main.c
 */
int main(void)
{
    initHw();

    USER_DATA data;

    int64_t diff = 0;

    uint64_t avg;
    uint8_t i,j;
    uint32_t newVal;
    uint32_t avgArr[AVG];

    uint32_t max = 0;
    uint8_t buffer = 0;

    float currentMass = 0;
    uint32_t temp = 0;

    putsUart0(CLEAR_PUTTY);
    putsUart0(HOME_POS);
    putsUart0("\x1B[7;0H");

    while(1) {

        avg = 0;

        newVal = readData();
        avgArr[i] = newVal;
        i = (i+1)%AVG;

        for(j=0; j<AVG; j++) {
            avg+=avgArr[j];
        }
        avg /= AVG;                 // Get average

        diff = avg - newVal;        // Difference in averages

        // getting max from spike
        if(diff > 1800) {
            if(buffer < 5)
            {
                if(diff > max)
                    max = diff;
                buffer++;
            }
        }
        else if(diff < -2000)
        {
            mass = 0;
            max = 0;
            buffer = 0;
            set = 0;
            force = 0;
        }
        else if(diff < -900)
        {
            max = 0;
            buffer = 0;
            set = 0;
        }


        if(max == 0) {
            currentMass = 0;
        }
        else {
            currentMass = 0.0198*(max) - 10.819;

            if(max > 30000) currentMass -= 25;
            else if(max > 40000) currentMass -= 35;

            if(buffer == 5 && !set)
            {
                mass += currentMass;
                force = mass * 9.81;
                set = 1;
            }
        }

        printStuff(avg, diff, max, mass, force);

        if(kbhitUart0()) {
            getsUart0(&data);
            parseFields(&data);

            char *input = getFieldString(&data, 0);

            if(!strCompare(input, "clear")) {
                putsUart0(CLEAR_PUTTY);
            }

            putsUart0("\x1B[7;0H");
            putsUart0(CLEAR_LINE);
        }

        waitMicrosecond(200e3);
    }
}

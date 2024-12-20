#include "gpio.h"
#include "clock.h"
#include "tm4c123gh6pm.h"
#include "wait.h"
#include "uart0.h"
#include "uart0ext.h"

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

// Ranges for 3 different metals
//-----------------------------//
#define CHAIR_UP    139000//67000
#define CHAIR_LO    130000//65000

#define VSUPPLY_UP  103000//51000
#define VSUPPLY_LO  99000//49000

#define DIG_UP       130000//66000
#define DIG_LO       121000//64000
//-----------------------------//

#define WT1         PORTC, 6    // WT1CCP0 timer

#define RED_LED     PORTF,1
#define BLUE_LED    PORTF,2
#define GREEN_LED   PORTF,3

uint32_t ticks = 0;
bool ready = false;

void initHw()
{
    initSystemClockTo40Mhz();
    initUart0();
    setUart0BaudRate(115200, 40e6);

    SYSCTL_RCGCWTIMER_R |= SYSCTL_RCGCWTIMER_R1 | SYSCTL_RCGCWTIMER_R2;

    enablePort(PORTC);
    enablePort(PORTF);

    selectPinPushPullOutput(RED_LED);
    selectPinPushPullOutput(BLUE_LED);
    selectPinPushPullOutput(GREEN_LED);

    selectPinDigitalInput(WT1);
    setPinAuxFunction(WT1, 7);              // makes PC6 WT1CCP0 timer

    WTIMER1_CTL_R &= ~TIMER_CTL_TAEN;                           // turn-off timer before reconfiguring
    WTIMER1_CFG_R = TIMER_CFG_16_BIT;                           // select it as 16 bit timer
    WTIMER1_TAMR_R = TIMER_TAMR_TAMR_CAP | TIMER_TAMR_TACDIR;   //count up, edge count
    WTIMER1_CTL_R |= TIMER_CTL_TAEVENT_POS;                     // count pos edge
    WTIMER1_TAV_R = 0;                                          // start at 0
    WTIMER1_CTL_R |= TIMER_CTL_TAEN;                            // enable timer

    WTIMER2_CTL_R &= ~TIMER_CTL_TAEN;                 // turn-off timer before reconfiguring
    WTIMER2_TAMR_R = TIMER_TAMR_TAMR_PERIOD;          // configure for periodic mode (count down)
    WTIMER2_TAILR_R = 40000000;                       // set load value to 40e6 for 1 sec int
    WTIMER2_IMR_R = TIMER_IMR_TATOIM;                 // turn-on interrupts
    WTIMER2_CTL_R |= TIMER_CTL_TAEN;                  // turn-on timer
    NVIC_EN3_R = 1 << (INT_WTIMER2A-16-96);           // turn-on interrupt 96 (TIMER1A) (112 - 16 = 96) pg 104
                                                      // calls wtimer1Isrs
}

void wTimer2ISR() {
    ticks = WTIMER1_TAV_R;
    WTIMER1_TAV_R = 0;
    ready = true;
    WTIMER2_ICR_R = TIMER_ICR_TATOCINT;
}

void wTimer1ISR() {}

void compISR() {}

int main() {

    initHw();
    char str[50] = {0};

    while(true) {
        if(ready) {
            snprintf(str, sizeof(str), "Ticks: %d\n", ticks);
            putsUart0(str);
            ready = false;
        }

        if(ticks >= CHAIR_LO && ticks <= CHAIR_UP) {
            setPinValue(RED_LED, 1);
            setPinValue(BLUE_LED, 0);
            setPinValue(GREEN_LED, 0);
        }
        else if(ticks >= VSUPPLY_LO && ticks <= VSUPPLY_UP) {
            setPinValue(RED_LED, 0);
            setPinValue(BLUE_LED, 1);
            setPinValue(GREEN_LED, 0);
        }
        else if(ticks >=DIG_LO && ticks <= DIG_UP) {
            setPinValue(RED_LED, 0);
            setPinValue(BLUE_LED, 0);
            setPinValue(GREEN_LED, 1);
        }
        else {
            setPinValue(RED_LED, 0);
            setPinValue(BLUE_LED, 0);
            setPinValue(GREEN_LED, 0);
        }
    }
}

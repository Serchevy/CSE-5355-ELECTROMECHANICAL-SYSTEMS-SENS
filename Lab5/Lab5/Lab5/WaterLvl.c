
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

//#define CAPACITOR   PORTE, 0
#define COMINUS     PORTC, 7        // C0- input of comparator
#define COPLUS      PORTC, 6        // C0+ input of comparator, set to internal Vref
#define DEINT       PORTE, 1        // gpio to base of transistor

uint32_t timer1Val = 0;
int ready = 0;


// Initialize Hardware
void initHw()
{
    // Initialize system clock to 40 MHz
    initSystemClockTo40Mhz();
    initUart0();
    setUart0BaudRate(115200, 40e6);

    SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R1;  // init clock for timer 1

    // configuring analog comparator using pg 1220
    SYSCTL_RCGCACMP_R |= SYSCTL_RCGCACMP_R0;               // Analog Comparator Run Mode Clock Gating Control
                                                           // set to 1 to enable clocks


    enablePort(PORTE);
    enablePort(PORTC);

    selectPinAnalogInput(COPLUS);
    selectPinAnalogInput(COMINUS);

    selectPinPushPullOutput(DEINT);
    setPinValue(DEINT, 0);

    // Configuring free running Timer 1, counts up for charging time
    TIMER1_CTL_R &= ~TIMER_CTL_TAEN;                    // disable for config
    TIMER1_CFG_R = TIMER_CFG_32_BIT_TIMER;              // select it as 32 bit timer
    TIMER1_TAMR_R = TIMER_TAMR_TACDIR | TIMER_TAMR_TAMR_1_SHOT;     // set to count up and be one shot
    TIMER1_CTL_R |= TIMER_CTL_TAEN;                     // enable timer
    TIMER1_TAV_R = 0;

    // Configure Wide Timer 1 as the time base, just counts ten seconds
    SYSCTL_RCGCWTIMER_R |= SYSCTL_RCGCWTIMER_R1;
    _delay_cycles(3);
    WTIMER1_CTL_R &= ~TIMER_CTL_TAEN;                 // turn-off timer before reconfiguring
    WTIMER1_TAMR_R = TIMER_TAMR_TAMR_PERIOD;          // configure for periodic mode (count down)
    WTIMER1_TAILR_R = 40000000;                       // set load value to 400e6 for 1/10 Hz interrupt rate,
                                                        // 1 int per 10 sec
    WTIMER1_IMR_R = TIMER_IMR_TATOIM;                 // turn-on interrupts
    WTIMER1_CTL_R |= TIMER_CTL_TAEN;                  // turn-on timer
    NVIC_EN3_R = 1 << (INT_WTIMER1A-16-96);              // turn-on interrupt 96 (TIMER1A) (112 - 16 = 96) pg 104
                                                                    // use en4 bc of 112



    COMP_ACREFCTL_R |= COMP_ACREFCTL_EN | 0x0F;          // makes comparator reference Vref = 2.469 from page 1219
    COMP_ACCTL0_R |= COMP_ACCTL0_ASRCP_REF | COMP_ACCTL0_CINV | COMP_ACCTL0_ISEN_RISE;
                                    // bc we are using comparator 0 (C0-)
                                    // tells compartor to reference Vref & inverts output of C0o ( OVAL )
    _delay_cycles(10);
    NVIC_EN0_R = 1 << (INT_COMP0 - 16 );    // turn-on interrupt 41 page 104
                                                   // calls compISR when issues


    COMP_ACINTEN_R |= COMP_ACINTEN_IN0;         // interrupt enable for comparator 0
    setPinValue(DEINT, 0);
}

void wTimer2ISR() {}

void wTimer1ISR()
{
    // reset cap value
    setPinValue(DEINT, 1);
    waitMicrosecond(100);
    setPinValue(DEINT, 0);

    TIMER1_TAV_R = 0;
    COMP_ACINTEN_R |= COMP_ACINTEN_IN0;         // interrupt enable for comparator 0
    WTIMER1_ICR_R = TIMER_ICR_TATOCINT;         // clear interrupt flag
}


// called when comparator Vref matches comp input
void compISR(void)
{
    timer1Val = TIMER1_TAV_R;
    ready=1;

    COMP_ACMIS_R = COMP_ACMIS_IN0;          // clearing interrupt for comparator 0
    COMP_ACINTEN_R &= ~COMP_ACINTEN_IN0;    // interrupt disable for comparator 0
}

/**
 * main.c
 */
int main(void)
{
    initHw();
    USER_DATA data;

    while(1)
    {
        if(ready)
        {
            ready=0;
            int waterLvl = 2.8298*(timer1Val) - 1236.7;
            if(timer1Val<455)
                waterLvl = 0;
            char str[100];
            snprintf(str, sizeof(str), "Ticks: %d\tWater Lvl: %d\n", timer1Val, waterLvl);
            putsUart0(str);
        }
    }
	return 0;
}


#include "gpio.h"
#include "clock.h"
#include "tm4c123gh6pm.h"
#include "uart0.h"
#include "uart0ext.h"
#include "nvic.h"
#include "adc0.h"
#include "wait.h"

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define A_ENABLE    PORTF, 2    // M1PWM6
#define PB1         PORTF, 4
#define PB2         PORTF, 0
#define LED         PORTC, 4    // IR LED, phototransmitter, WT0CCP0 timer
#define AIN2        PORTE, 1

uint32_t count = 0;
uint32_t freq = 0;
uint32_t rawBackEmf = 0;
bool isCalculated = 0;

uint8_t button = 5;
uint16_t buttonPwm[11] = {0, 105, 204, 306, 408, 510, 612, 714, 816, 918, 1023};

// Initialize Hardware
void initHw()
{
    // Initialize system clock to 40 MHz
    initSystemClockTo40Mhz();
    initUart0();
    setUart0BaudRate(115200, 40e6);

    SYSCTL_RCGCPWM_R |= SYSCTL_RCGCPWM_R1;
    SYSCTL_RCGCWTIMER_R |= SYSCTL_RCGCWTIMER_R1 | SYSCTL_RCGCWTIMER_R0 | SYSCTL_RCGCWTIMER_R2;      // clocks for timer 1 & 0
    SYSCTL_RCGCADC_R |= SYSCTL_RCGCADC_R0;
    _delay_cycles(3);

    enablePort( PORTF );
    enablePort( PORTC );
    enablePort( PORTE );

    selectPinAnalogInput(AIN2);     // pin PE1
    initAdc0Ss3();
    setAdc0Ss3Log2AverageCount(0);
    setAdc0Ss3Mux(2);

    selectPinPushPullOutput( A_ENABLE );
    setPinAuxFunction(A_ENABLE, 5);

    setPinCommitControl(PB2);           // unlocked PB2
    selectPinDigitalInput(PB1);
    enablePinPullup(PB1);

    selectPinDigitalInput(PB2);
    enablePinPullup(PB2);

    selectPinDigitalInput(LED);
    setPinAuxFunction(LED, 7);          // makes WT0CCP0 timer

    // A_ENABLE
    SYSCTL_SRPWM_R = SYSCTL_SRPWM_R1 | SYSCTL_SRPWM_R0;                 // reset PWM module 0 & 2
    SYSCTL_SRPWM_R = 0;                                                 // leave reset state
    PWM1_3_CTL_R = 0;                                                   // pwm 6, block 3

    PWM1_3_GENA_R = PWM_3_GENA_ACTCMPAD_ONE | PWM_3_GENA_ACTLOAD_ZERO;  // pwm 6 -> gen a
    PWM1_3_LOAD_R = 1024;                                               // frequency

    PWM1_3_CTL_R = PWM_3_CTL_ENABLE;
    PWM1_ENABLE_R = PWM_ENABLE_PWM6EN;

    PWM1_3_CMPA_R = round(1023*0.5) ;       // turning on

    // Configure WTimer 0A
    WTIMER0_CTL_R &= ~TIMER_CTL_TAEN;                   // turn-off timer before reconfiguring
    WTIMER0_CFG_R = TIMER_CFG_16_BIT;                   // select it as 16 bit timer
    WTIMER0_TAMR_R = TIMER_TAMR_TAMR_CAP | TIMER_TAMR_TACDIR;         //count up, edge count
    WTIMER0_CTL_R |= TIMER_CTL_TAEVENT_NEG;             // count neg edge
    WTIMER0_TAV_R = 0;                                  // start at 0
    WTIMER0_CTL_R |= TIMER_CTL_TAEN;                    // enable timer

    // Configure Wide Timer 1 regular frequency calculator
    WTIMER1_CTL_R &= ~TIMER_CTL_TAEN;                 // turn-off timer before reconfiguring
    WTIMER1_TAMR_R = TIMER_TAMR_TAMR_PERIOD;          // configure for periodic mode (count down)
    WTIMER1_TAILR_R = 40000000;                       // set load value to 400e6 for 1/10 Hz interrupt rate,
                                                      // i int per 10 sec
    WTIMER1_IMR_R = TIMER_IMR_TATOIM;                 // turn-on interrupts
    WTIMER1_CTL_R |= TIMER_CTL_TAEN;                  // turn-on timer
    NVIC_EN3_R = 1 << (INT_WTIMER1A-16-96);           // turn-on interrupt 96 (TIMER1A) (112 - 16 = 96) pg 104
                                                      // calls wtimer1Isr

    // Configure Wide Timer 50 hz
    WTIMER2_CTL_R &= ~TIMER_CTL_TAEN;                 // turn-off timer before reconfiguring
    WTIMER2_TAMR_R = TIMER_TAMR_TAMR_PERIOD;          // configure for periodic mode (count down)
    WTIMER2_TAILR_R = 800000;                // set load value to 400e6 for 1/10 Hz interrupt rate,
                                                      // i int per 10 sec
    WTIMER2_IMR_R = TIMER_IMR_TATOIM;                 // turn-on interrupts
    WTIMER2_CTL_R |= TIMER_CTL_TAEN;                  // turn-on timer
    NVIC_EN3_R = 1 << (INT_WTIMER2A-16-96);           // turn-on interrupt 96 (TIMER1A) (112 - 16 = 96) pg 104
                                                      // calls wtimer1Isrs
}

void stopMotorISR()
{
    int prevPWM = PWM1_3_CMPA_R;
    PWM1_3_CMPA_R = 0;
    waitMicrosecond(500);
    rawBackEmf = readAdc0Ss3();
    PWM1_3_CMPA_R = prevPWM;
    WTIMER2_ICR_R = TIMER_ICR_TATOCINT;         // clear timer interrupt flag
}

void freqCalculatorISR()
{
    isCalculated = 1;                           // flag to print in main

    freq = (WTIMER0_TAV_R/32) * 60;             // based on periodic interval of this isr, calcs rps
    WTIMER0_TAV_R = 0;

    WTIMER1_ICR_R = TIMER_ICR_TATOCINT;         // clear timer interrupt flag
}

/**
 * main.c
 */
int main(void)
{
    initHw();

    char str[50] = {0};
    float invBackEmf = 0;
    float BackEmf = 0;
    float estimate_rpm = 0;

    while(1) {
        if(!getPinValue(PB1))
        {
            if(button==0)
            {
                button = 0;
            }
            else
            {
                button--;
                PWM1_3_CMPA_R = buttonPwm[button];
            }
            waitMicrosecond(500000);
        }
        else if(!getPinValue(PB2))
        {
            if(button==10)
            {
                button = 10;
            }
            else
            {
                button++;
                PWM1_3_CMPA_R = buttonPwm[button];
            }
            waitMicrosecond(500000);
        }

        if(isCalculated)
        {

            snprintf(str, sizeof(str), "PWM: %d\t", PWM1_3_CMPA_R);
            putsUart0(str);

            snprintf(str, sizeof(str), "rpm: %d\t", freq);
            putsUart0(str);

            invBackEmf = (((float)rawBackEmf + 0.5)/4096) * 3.3;
            BackEmf = 10 - (invBackEmf * (47+10)/10);

            snprintf(str, sizeof(str), "Inv-BEMF: %.2f V\t", invBackEmf);
            putsUart0(str);

            snprintf(str, sizeof(str), "Back-EMF: %.2f V\t", BackEmf);
            putsUart0(str);

            estimate_rpm = 237.29*BackEmf + 65.25;

            snprintf(str, sizeof(str), "estimate rpm: %.2f\n", estimate_rpm);
            putsUart0(str);

            isCalculated=0;
        }
    }

	return 0;
}

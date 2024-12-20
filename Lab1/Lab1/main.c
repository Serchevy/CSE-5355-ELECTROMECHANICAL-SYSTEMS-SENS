

/**
 * main.c
 */
#include "gpio.h"
#include "clock.h"
#include "wait.h"

// Microcontroller GPIO Pins
#define GATE_PIN PORTF, 1

// Initialize Hardware
void initHw()
{
    // Initialize system clock to 40 MHz
    initSystemClockTo40Mhz();
    enablePort( PORTF );
    selectPinPushPullOutput( GATE_PIN );
}



int main(void)
{
    initHw();
    while(true)
    {
//        togglePinValue(GATE_PIN);
//        waitMicrosecond(420000);
        setPinValue(GATE_PIN, 1);
        waitMicrosecond(55000);
        setPinValue(GATE_PIN, 0);
        waitMicrosecond(180000);
    }
	return 0;
}

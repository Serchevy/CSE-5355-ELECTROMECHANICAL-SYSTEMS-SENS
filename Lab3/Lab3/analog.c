// Analog Example
// Jason Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL with LCD/Temperature Sensor
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz
// Stack:           4096 bytes (needed for snprintf)

// Hardware configuration:
// LM60 Temperature Sensor:
//   AIN3/PE0 is driven by the sensor
//   (V = 424mV + 6.25mV / degC with +/-2degC uncalibrated error)
// UART Interface:
//   U0TX (PA1) and U0RX (PA0) are connected to the 2nd controller
//   The USB on the 2nd controller enumerates to an ICDI interface and a virtual COM port
//   Configured to 115,200 baud, 8N1

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "clock.h"
#include "uart0ext.h"
#include "wait.h"
#include "uart0.h"
#include "adc0.h"
#include "tm4c123gh6pm.h"

// PortE masks
#define AIN3_MASK 1

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// Initialize Hardware
void initHw()
{
    // Initialize system clock to 40 MHz
    initSystemClockTo40Mhz();

    // Enable clocks
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R4;
    _delay_cycles(3);

    // Configure AIN3 as an analog input
	GPIO_PORTE_AFSEL_R |= AIN3_MASK;                 // select alternative functions for AIN3 (PE0)
    GPIO_PORTE_DEN_R &= ~AIN3_MASK;                  // turn off digital operation on pin PE0
    GPIO_PORTE_AMSEL_R |= AIN3_MASK;                 // turn on analog operation on pin PE0
}

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------

int main(void)
{
    uint16_t raw;
//    uint16_t x[16];
//    float instantTemp, firTemp, iirTemp = 0;
//    uint8_t index = 0;
//    uint16_t sum = 0; // total fits in 16b since 12b adc output x 16 samples
//    uint8_t i;
//    float alpha = 0.80;
//    char str[80];
//
//    // Initialize hardware
//    initHw();
//    initUart0();
//    initAdc0Ss3();
//
//    // Setup UART0 baud rate
//    setUart0BaudRate(115200, 40e6);
//
//    // Use AIN3 input with N=4 hardware sampling
//    setAdc0Ss3Mux(3);
//    setAdc0Ss3Log2AverageCount(2);
//
//    // Clear FIR filter taps
//    for (i = 0; i < 16; i++)
//        x[i] = 0;

    // Endless loop
    while(true)
    {
        // Read sensor
        raw = readAdc0Ss3();
    }
}

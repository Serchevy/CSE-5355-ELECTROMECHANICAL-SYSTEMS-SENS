// NVIC Library
// Jason Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL
// Target uC:       TM4C123GH6PM
// System Clock:    -

// Hardware configuration: -

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include "nvic.h"
#include "tm4c123gh6pm.h"

//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void enableNvicInterrupt(uint8_t vectorNumber)
{
    volatile uint32_t* p = (uint32_t*) &NVIC_EN0_R;	// pointer is the NVIC enable 0 register
    vectorNumber -= 16;					// subtract first 16 interrupts
    p += vectorNumber >> 5;				// offsets for appropriate EN# (mod 32)
    *p = 1 << (vectorNumber & 31);			// offsets into the EN# register
							// writes a 1 bit to enable
}

void disableNvicInterrupt(uint8_t vectorNumber)
{
    volatile uint32_t* p = (uint32_t*) &NVIC_DIS0_R;	// pointer is the NVIC disable 0 reg
    vectorNumber -= 16;
    p += vectorNumber >> 5;
    *p = 1 << (vectorNumber & 31);			// write a 1 bit to disable
}

void setNvicInterruptPriority(uint8_t vectorNumber, uint8_t priority)
{
    volatile uint32_t* p = (uint32_t*) &NVIC_PRI0_R;
    vectorNumber -= 16;
    uint32_t shift = 5 + (vectorNumber & 3) * 8;
    p += vectorNumber >> 2;
    *p &= ~(7 << shift);
    *p |= priority << shift;
}


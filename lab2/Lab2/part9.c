
#include "gpio.h"
#include "clock.h"
#include "wait.h"

// pins
#define A_ENABLE    PORTF, 2
#define A_DIR1      PORTF, 3
#define A_DIR2      PORTB, 3
#define B_ENABLE    PORTC, 4
#define B_DIR1      PORTC, 5
#define B_DIR2      PORTC, 6

// Initialize Hardware
void initHw()
{
    // Initialize system clock to 40 MHz
    initSystemClockTo40Mhz();
    enablePort( PORTF );
    enablePort( PORTB );
    enablePort( PORTC );
    selectPinPushPullOutput( A_ENABLE );
    selectPinPushPullOutput( A_DIR1 );
    selectPinPushPullOutput( A_DIR2 );
    selectPinPushPullOutput( B_ENABLE );
    selectPinPushPullOutput( B_DIR1 );
    selectPinPushPullOutput( B_DIR2 );

    setPinValue( A_ENABLE, 0);
    setPinValue( A_DIR1, 0);
    setPinValue( A_DIR2, 0);
    setPinValue( B_ENABLE, 0);
    setPinValue( B_DIR1, 0);
    setPinValue( B_DIR2, 0);
}

void coilA(uint8_t dir1, uint8_t dir2)
{
    setPinValue( A_ENABLE, 1);
    setPinValue( A_DIR1, dir1);
    setPinValue( A_DIR2, dir2);
    waitMicrosecond(40000);
    //reset
    setPinValue( A_ENABLE, 0);
    setPinValue( A_DIR1, 0);
    setPinValue( A_DIR2, 0);
}

void coilB(uint8_t dir1, uint8_t dir2)
{
    setPinValue( B_ENABLE, 1);
    setPinValue( B_DIR1, dir1);
    setPinValue( B_DIR2, dir2);
    waitMicrosecond(40000);             // 40k is good
    //reset
    setPinValue( B_ENABLE, 0);
    setPinValue( B_DIR1, 0);
    setPinValue( B_DIR2, 0);
}

// main
int main(void)
{
    initHw();

    // one pin per coil is the enable pin for motor
    // two pins per coil should be inv of each other, drives coils, direction
    uint8_t i;
    for(i=0; i<4; i++)
    {
        coilA(1, 0);
        coilB(1, 0);
        coilA(0, 1);
        coilB(0, 1);
    }
    for(i=0; i<4; i++)
    {
        coilB(0, 1);
        coilA(0, 1);
        coilB(1, 0);
        coilA(1, 0);
    }
	while(1);
}

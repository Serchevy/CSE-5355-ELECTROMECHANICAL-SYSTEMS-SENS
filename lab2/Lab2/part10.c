
#include "gpio.h"
#include "clock.h"
#include "wait.h"
#include "uart0.h"
#include "uart0ext.h"
#include <stdlib.h>
#include <math.h>


// pins
#define A_ENABLE    PORTF, 2    // M1PWM6
#define A_DIR1      PORTF, 3
#define A_DIR2      PORTB, 3

#define B_ENABLE    PORTC, 4    // M0PWM6
#define B_DIR1      PORTC, 5
#define B_DIR2      PORTC, 6

#define COLLECTOR   PORTE, 3

// Initialize Hardware
void initHw()
{
    // Initialize system clock to 40 MHz
    initSystemClockTo40Mhz();


    initUart0();
    setUart0BaudRate( 115200, 40e6 );

    enablePort( PORTF );
    enablePort( PORTB );
    enablePort( PORTC );
    enablePort( PORTE );

    selectPinPushPullOutput( A_ENABLE );
    selectPinPushPullOutput( A_DIR1 );
    selectPinPushPullOutput( A_DIR2 );

    selectPinPushPullOutput( B_ENABLE );
    selectPinPushPullOutput( B_DIR1 );
    selectPinPushPullOutput( B_DIR2 );

    selectPinDigitalInput( COLLECTOR );

    setPinValue( A_ENABLE, 1);
    setPinValue( A_DIR1, 0);
    setPinValue( A_DIR2, 0);
    setPinValue( B_ENABLE, 1);
    setPinValue( B_DIR1, 0);
    setPinValue( B_DIR2, 0);


}

void coilA(uint8_t dir1, uint8_t dir2)
{
    //reset
    setPinValue( B_DIR1, 0);
    setPinValue( B_DIR2, 0);

    setPinValue( A_DIR1, dir1);
    setPinValue( A_DIR2, dir2);
    waitMicrosecond(20000);

}

void coilB(uint8_t dir1, uint8_t dir2)
{
    //reset
    setPinValue( A_DIR1, 0);
    setPinValue( A_DIR2, 0);

    setPinValue( B_DIR1, dir1);
    setPinValue( B_DIR2, dir2);
    waitMicrosecond(20000);
}

void step(uint8_t stepNum)
{
    switch(stepNum%4)
    {
        case 0:
            coilA(1, 0);
            break;
        case 1:
            coilB(1, 0);
            break;
        case 2:
            coilA(0, 1);
            break;
        case 3:
            coilB(0, 1);
            break;
    }

}

void stepBack(uint8_t stepNum)
{
    switch(stepNum%4)
    {
        case 0:
            coilB(0, 1);
            break;
        case 1:
            coilA(0, 1);
            break;
        case 2:
            coilB(1, 0);
            break;
        case 3:
            coilA(1, 0);
            break;
    }

}

// main
int main(void)
{
    initHw();

    // one pin per coil is the enable pin for motor
    // two pins per coil should be inv of each other, drives coils, direction
    uint8_t i;

    while(!getPinValue(COLLECTOR))
    {
        coilA(1, 0);
        coilB(1, 0);
        coilA(0, 1);
        coilB(0, 1);
    }

    //straightening
    for(i=0; i<6; i++)
    {
        coilB(0, 1);
        coilA(0, 1);
        coilB(1, 0);
        coilA(1, 0);
    }
    coilB(0, 1);
    coilA(0, 1);

    float currentAngle = 0;

    USER_DATA data;
    while(1)
    {
        putsUart0("Angle: ");
        getsUart0(&data);
        parseFields(&data);
        char* str = getFieldString(&data, 0);
        float angle = atof(str);
        putsUart0(str);
        putsUart0("\n");
        int i;
        float a;
        if(angle>currentAngle)
        {
            a = angle - (float)currentAngle;
            a = round(a/1.8);
            for(i=0; i<a; i++) // 1.35
            {
                step(i);
            }
            currentAngle=angle;
        }
        else if(angle<currentAngle)
        {
            a = angle - (float)currentAngle;
            a = -a;
            a = round(a/1.8);
            for(i=0; i<a; i++)
            {
                stepBack(i);
            }

            currentAngle=angle;
        }

    }





	while(1);
}

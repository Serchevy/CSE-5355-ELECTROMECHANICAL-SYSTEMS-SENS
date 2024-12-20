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
#include "uart0ext.h"


#define Aen     PORTD, 0
#define Ben     PORTD, 1
#define Cen     PORTD, 2
#define Adir    PORTD, 3
#define Bdir    PORTE, 1
#define Cdir    PORTE, 2

int wait = 10000;

void initHw()
{
    // Initialize system clock to 40 MHz
    initSystemClockTo40Mhz();
    initUart0();
    setUart0BaudRate(115200, 40e6);

    enablePort( PORTD );
    enablePort( PORTE );

    selectPinPushPullOutput(Aen);
    selectPinPushPullOutput(Ben);
    selectPinPushPullOutput(Cen);
    selectPinPushPullOutput(Adir);
    selectPinPushPullOutput(Bdir);
    selectPinPushPullOutput(Cdir);

    setPinValue(Aen, 0);
    setPinValue(Ben, 0);
    setPinValue(Cen, 0);
}

void step(uint64_t step)
{
    switch(step%6)
    {
    case 0:                         // blue down, orange d, brown h, 120
        setPinValue(Adir, 1);
        setPinValue(Bdir, 0);
        setPinValue(Cdir, 0);
        setPinValue(Cen, 0);
        setPinValue(Aen, 1);
        setPinValue(Ben, 1);
        break;
    case 1:                         // orange high, blue d, brown h, 150
        setPinValue(Adir, 0);
        setPinValue(Bdir, 0);
        setPinValue(Cdir, 1);
        setPinValue(Aen, 0);
        setPinValue(Ben, 1);
        break;
    case 2:                         // brown d, blue d, orange h, 0
        setPinValue(Adir, 0);
        setPinValue(Bdir, 0);
        setPinValue(Cdir, 1);
        setPinValue(Ben, 0);
        setPinValue(Aen, 1);
        setPinValue(Cen, 1);
        break;
    case 3:                         // blue h, broen d, orange h, 30
        setPinValue(Adir, 0);
        setPinValue(Bdir, 1);
        setPinValue(Cdir, 0);
        setPinValue(Cen, 0);
        setPinValue(Aen, 1);
        setPinValue(Ben, 1);
        break;
    case 4:                         // brown d, blue h, orange d, 60
        setPinValue(Cdir, 0);
        setPinValue(Bdir, 1);
        setPinValue(Adir, 0);
        setPinValue(Aen, 0);
        setPinValue(Cen, 1);
        setPinValue(Ben, 1);
        break;
    case 5:                         // brown h, blue h, orange d, 90
        setPinValue(Adir, 1);
        setPinValue(Bdir, 0);
        setPinValue(Cdir, 0);
        setPinValue(Ben, 0);
        setPinValue(Cen, 1);
        setPinValue(Aen, 1);
        break;
    default:
        setPinValue(Aen, 0);
        setPinValue(Ben, 0);
        setPinValue(Cen, 0);
        break;
    }
    if(wait>2750)
        wait-=10;
}

/**
 * main.c
 */
int main(void)
{
    initHw();

    USER_DATA data;


    uint64_t i=0;
    while(1)
    {
        if(kbhitUart0())
        {
            getsUart0(&data);
            parseFields(&data);
//            wait = getFieldInteger(&data, 0);
//            if(wait==0)
//                wait=1;
            char* str = getFieldString(&data, 0);

            if(!strCompare(str, "d"))
            {
                wait-=1000;
                char* microseconds = itoa(wait);
                putsUart0(microseconds);
                putsUart0("\n");
            }
            if(!strCompare(str, "i"))
            {
                wait+=1000;
                char* microseconds = itoa(wait);
                putsUart0(microseconds);
                putsUart0("\n");
            }
        }
        step(i);
        waitMicrosecond(wait);
        i++;
    }
    while(1);
	return 0;
}

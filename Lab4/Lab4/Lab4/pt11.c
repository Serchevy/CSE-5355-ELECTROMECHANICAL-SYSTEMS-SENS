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
#define HallBrown   PORTE, 5
#define HallBlue    PORTB, 4
#define HallOrange  PORTA, 5

uint32_t wait = 20000;
uint32_t counts[6] = {0,0,0,0,0,0};
uint32_t desired = 100;

uint16_t overflow = 0;
bool currStep[6] = {0,0,0,0,0,0};


void initHw()
{
    // Initialize system clock to 40 MHz
    initSystemClockTo40Mhz();
    initUart0();
    setUart0BaudRate(115200, 40e6);

    enablePort( PORTD );
    enablePort( PORTE );
    enablePort( PORTA );
    enablePort( PORTB );

    selectPinPushPullOutput(Aen);
    selectPinPushPullOutput(Ben);
    selectPinPushPullOutput(Cen);
    selectPinPushPullOutput(Adir);
    selectPinPushPullOutput(Bdir);
    selectPinPushPullOutput(Cdir);

    selectPinDigitalInput(HallBrown);
    selectPinDigitalInput(HallBlue);
    selectPinDigitalInput(HallOrange);

    setPinValue(Aen, 0);
    setPinValue(Ben, 0);
    setPinValue(Cen, 0);
}

void step(uint8_t step)
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
//    waitMicrosecond(wait);
    if(wait>desired)
    {
        if(wait<=100 && desired<100)
        {
            wait = desired;
        }
        else
            wait-=100;
    }
    else if(wait<desired)
        wait+=100;
}

void updateStep(int step)
{
    int j =0;

    if(!currStep[step]){               // state has transitioned
        overflow=0;
        for(j=0; j<6; j++)
        {
            currStep[j]=0;
        }
        currStep[step]=1;
    }
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
            desired = getFieldInteger(&data, 0);
            if(desired==0)
                desired=1;
//            char* str = getFieldString(&data, 0);
//
//
//            if(!strCompare(str, "d"))
//            {
//                wait-=100;
//                char* microseconds = itoa(wait);
//                putsUart0(microseconds);
//                putsUart0("\n");
//            }
//            if(!strCompare(str, "i"))
//            {
//                wait+=100;
//                char* microseconds = itoa(wait);
//                putsUart0(microseconds);
//                putsUart0("\n");
//            }
        }
        if(getPinValue(HallBrown) && !getPinValue(HallBlue) && !getPinValue(HallOrange) )
        {
            step(0);
            counts[0]++;
            overflow++;
            updateStep(0);
            if(overflow>=0x2000){           // is stopped
                setPinValue(Aen, 0);
                setPinValue(Ben, 0);
                setPinValue(Cen, 0);
                step(1);
                overflow=0;
            }
        }
        else if(getPinValue(HallBrown) && !getPinValue(HallBlue) && getPinValue(HallOrange) )
        {
            step(1);
            counts[1]++;
            overflow++;
            updateStep(1);
            if(overflow>=0x2000){
                setPinValue(Aen, 0);
                setPinValue(Ben, 0);
                setPinValue(Cen, 0);
                step(2);
                overflow=0;
            }

        }
        else if(!getPinValue(HallBrown) && !getPinValue(HallBlue) && getPinValue(HallOrange) )
        {
            step(2);
            counts[2]++;
            overflow++;
            updateStep(2);
            if(overflow>=0x2000){
                setPinValue(Aen, 0);
                setPinValue(Ben, 0);
                setPinValue(Cen, 0);
                step(3);
                overflow=0;
            }
        }
        else if(!getPinValue(HallBrown) && getPinValue(HallBlue) && getPinValue(HallOrange) )
        {
            step(3);
            counts[3]++;
            overflow++;
            updateStep(3);
            if(overflow>=0x2000){
                setPinValue(Aen, 0);
                setPinValue(Ben, 0);
                setPinValue(Cen, 0);
                step(4);
                overflow=0;
            }
        }
        else if(!getPinValue(HallBrown) && getPinValue(HallBlue) && !getPinValue(HallOrange) )
        {
            step(4);
            counts[4]++;
            overflow++;
            updateStep(4);
            if(overflow>=0x2000){
                setPinValue(Aen, 0);
                setPinValue(Ben, 0);
                setPinValue(Cen, 0);
                step(5);
                overflow=0;
            }
        }
        else if(getPinValue(HallBrown) && getPinValue(HallBlue) && !getPinValue(HallOrange) )
        {
            step(5);
            counts[5]++;
            overflow++;
            updateStep(5);
            if(overflow>=0x2000){
                setPinValue(Aen, 0);
                setPinValue(Ben, 0);
                setPinValue(Cen, 0);
                step(0);
                overflow=0;
            }
        }
    }
    while(1);
	return 0;
}

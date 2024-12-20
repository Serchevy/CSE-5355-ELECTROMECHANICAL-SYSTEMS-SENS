#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"
#include "gpio.h"
#include "clock.h"
#include "wait.h"

#define A_ENABLE    PORTB,7     // PWM pin
#define A_DIR1      PORTB,3
#define A_DIR2      PORTC,4

#define B_ENABLE    PORTB,6     // PWM pin
#define B_DIR1      PORTF,2
#define B_DIR2      PORTF,3

void initHw() {

    initSystemClockTo40Mhz();

    enablePort(PORTB);
    enablePort(PORTC);
    enablePort(PORTF);

    selectPinPushPullOutput(A_ENABLE);
    selectPinPushPullOutput(A_DIR1);
    selectPinPushPullOutput(A_DIR2);

    selectPinPushPullOutput(B_ENABLE);
    selectPinPushPullOutput(B_DIR1);
    selectPinPushPullOutput(B_DIR2);

//    setPinAuxFunction(A_ENABLE);
//    setPinAuxFunction(B_ENABLE);

    setPinValue(A_ENABLE, 0);
    setPinValue(A_DIR1, 0);
    setPinValue(A_DIR2, 0);
    setPinValue(B_ENABLE, 0);
    setPinValue(B_DIR1, 0);
    setPinValue(B_DIR2, 0);

}

void setA(uint32_t dir1, uint32_t dir2) {
    setPinValue(B_ENABLE, 0);
    setPinValue(B_DIR1, 0);
    setPinValue(B_DIR2, 0);

    setPinValue(A_ENABLE, 1);
    setPinValue(A_DIR1, dir1);
    setPinValue(A_DIR2, dir2);
}

void setB(uint32_t dir1, uint32_t dir2) {
    setPinValue(A_ENABLE, 0);
    setPinValue(A_DIR1, 0);
    setPinValue(A_DIR2, 0);

    setPinValue(B_ENABLE, 1);
    setPinValue(B_DIR1, dir1);
    setPinValue(B_DIR2, dir2);
}

int main(void) {

    initHw();

    uint32_t i;

    //     A     B
    //  | + - | * * |
    //  | * * | + - |
    //  | - + | * * |
    //  | * * | - + |

    for(i = 0; i < 4; i++) {

        setA(1, 0);
        waitMicrosecond(10000000);
        setB(1, 0);
        waitMicrosecond(10000000);
        setA(0, 1);
        waitMicrosecond(10000000);
        setB(0, 1);
        waitMicrosecond(10000000);

    }


    while(true);

	return 0;
}


#include "gpio.h"
#include "clock.h"
#include "wait.h"
#include "uart0.h"
#include "uart0ext.h"
#include <stdlib.h>
#include <math.h>

#define stepType    4

// 1    Full Step
// 2    Half Step
// 4    Quarter Step
// 8    1/8 Step
// 16   1/16 Step
// 32   1/32 Step

// pins
#define A_ENABLE    PORTF, 2    // M1PWM6
#define A_DIR1      PORTF, 3
#define A_DIR2      PORTB, 3

#define B_ENABLE    PORTC, 4    // M0PWM6
#define B_DIR1      PORTC, 5
#define B_DIR2      PORTC, 6

#define COLLECTOR   PORTE, 3

int timing[33];


// Initialize Hardware
void initHw()
{
    // Initialize system clock to 40 MHz
    initSystemClockTo40Mhz();

    SYSCTL_RCGCPWM_R |= SYSCTL_RCGCPWM_R1 | SYSCTL_RCGCPWM_R0;
    _delay_cycles(3);

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

    //setPinValue( A_ENABLE, 1);
    setPinValue( A_DIR1, 0);
    setPinValue( A_DIR2, 0);
    //setPinValue( B_ENABLE, 1);
    setPinValue( B_DIR1, 0);
    setPinValue( B_DIR2, 0);

    setPinAuxFunction(A_ENABLE, 5);
    setPinAuxFunction(B_ENABLE, 4);

    SYSCTL_SRPWM_R = SYSCTL_SRPWM_R1 | SYSCTL_SRPWM_R0;                 // reset PWM module 0 & 2
    SYSCTL_SRPWM_R = 0;                                                 // leave reset state
    PWM1_3_CTL_R = 0;                                                   // pwm 6, block 3
    PWM0_3_CTL_R = 0;                                                   // pwm 6, block 3

    PWM1_3_GENA_R = PWM_3_GENA_ACTCMPAD_ONE | PWM_3_GENA_ACTLOAD_ZERO;  // pwm 6 -> gen a
    PWM0_3_GENA_R = PWM_3_GENA_ACTCMPAD_ONE | PWM_3_GENA_ACTLOAD_ZERO;  // pwm 6 -> gen a

    PWM1_3_LOAD_R = 1024;                                               // frequency
    PWM0_3_LOAD_R = 1024;

    PWM1_3_CMPA_R = 0;                                                  // A_ENABLE
    PWM0_3_CMPA_R = 0;                                                  // B_ENABLE

    PWM1_3_CTL_R = PWM_3_CTL_ENABLE;
    PWM0_3_CTL_R = PWM_3_CTL_ENABLE;

    PWM1_ENABLE_R = PWM_ENABLE_PWM6EN;
    PWM0_ENABLE_R = PWM_ENABLE_PWM6EN;

    PWM1_3_CMPA_R = 1023;
    PWM0_3_CMPA_R = 1023;
}

void coilA(uint8_t dir1, uint8_t dir2)
{
    //reset
    setPinValue( B_DIR1, 0);
    setPinValue( B_DIR2, 0);

    setPinValue( A_DIR1, dir1);
    setPinValue( A_DIR2, dir2);
    waitMicrosecond(10000);

}

void coilB(uint8_t dir1, uint8_t dir2)
{
    //reset
    setPinValue( A_DIR1, 0);
    setPinValue( A_DIR2, 0);

    setPinValue( B_DIR1, dir1);
    setPinValue( B_DIR2, dir2);
    waitMicrosecond(10000);
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

void setA(uint8_t dir1, uint8_t dir2) {
    setPinValue( A_DIR1, dir1);
    setPinValue( A_DIR2, dir2);
}

void setB(uint8_t dir1, uint8_t dir2) {
    setPinValue( B_DIR1, dir1);
    setPinValue( B_DIR2, dir2);
}

void microStep(float A, float B) {
    if((fabsf(A) < 1e-14)) {
        setA(0, 0);
    }
    else if(A>0) {
        setA(1, 0);
    }
    else if(A<0) {
        setA(0, 1);
    }

    if((fabsf(B) < 1e-14)) {
        setB(0, 0);
    }
    else if(B>0) {
        setB(1, 0);
    }
    else if(B<0) {
        setB(0, 1);
    }

    waitMicrosecond(1000000);
}

// main
int main(void)
{
    initHw();

    uint8_t i;

    while(!getPinValue(COLLECTOR))
    {
        coilA(1, 0);
        coilB(1, 0);
        coilA(0, 1);
        coilB(0, 1);
    }

    //straightening
    for(i=0; i<5; i++)
    {
        coilB(0, 1);
        coilA(0, 1);
        coilB(1, 0);
        coilA(1, 0);

    }

    int boot = 0;

    float currentAngle = 0;

    USER_DATA data;

    int table_i = 0;
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

        a = angle - (float)currentAngle; 
        float div = 1.8/stepType;
        a = abs(round(a/div)); 

        if(!boot)
        {
            if(angle<currentAngle)                      // CCW
                table_i = 0;
            else
                table_i = 1;
            float A = cos(0 * (M_PI/(stepType*2)));
            float B = sin(0 * (M_PI/(stepType*2)));
            PWM1_3_CMPA_R = abs(1023 * A);
            PWM0_3_CMPA_R = abs(1023 * B);
            microStep(A, B);
            boot = 1;
        }

        for(i=0; i<a; i++) // 1.35
        {

            if(angle>currentAngle)                      // CW 
                table_i++;
            else
                table_i--;

            float A = cos(table_i * (M_PI/(stepType*2)));
            float B = sin(table_i * (M_PI/(stepType*2)));
            PWM1_3_CMPA_R = abs(1023 * A);
            PWM0_3_CMPA_R = abs(1023 * B);

            microStep(A, B);

        }
        currentAngle=angle;

    }

	while(1);
}

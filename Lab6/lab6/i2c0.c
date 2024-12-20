// I2C0 Library
// Jason Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

// Hardware configuration:
// I2C devices on I2C bus 0 with 2kohm pullups on SDA and SCL

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"
#include "gpio.h"
#include "i2c0.h"

// Pins
#define I2C0SCL PORTB,2
#define I2C0SDA PORTB,3

//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void initI2c0(void)
{
    // Enable clocks
    SYSCTL_RCGCI2C_R |= SYSCTL_RCGCI2C_R0;
    _delay_cycles(3);
    enablePort(PORTB);

    // Configure I2C
    selectPinPushPullOutput(I2C0SCL);
    setPinAuxFunction(I2C0SCL, GPIO_PCTL_PB2_I2C0SCL);      // Control by pin
    selectPinOpenDrainOutput(I2C0SDA);
    setPinAuxFunction(I2C0SDA, GPIO_PCTL_PB3_I2C0SDA);

    // Configure I2C0 peripheral
    // Complete config
    // 10/.025/10
    // 20 - 1
    I2C0_MCR_R = 0;                                     // disable to program
    I2C0_MTPR_R = 19;                                   // (40MHz/2) / (6+4) / (19+1) = 100kbps
    I2C0_MCR_R = I2C_MCR_MFE;                           // master
                                                        // set MEF bit for master mode
    I2C0_MCS_R = I2C_MCS_STOP;                          // Status/Control register
                                                        // when reading it its read only
                                                        // When writting it bits are completely different 
}

// For simple devices with a single internal register
void writeI2c0Data(uint8_t add, uint8_t data)
{
    // send start cond, send address, get ack,
    // send data, get ack, send stop condition
    I2C0_MSA_R = add << 1 | 0; // add:r/~w=0
    I2C0_MDR_R = data;
    I2C0_MICR_R = I2C_MICR_IC;  // Interrupts are clear
    I2C0_MCS_R = I2C_MCS_START | I2C_MCS_RUN | I2C_MCS_STOP;    // write to control register
                                                                // Send start condition code, 
                                                                // run causes data to be transmitted
                                                                // Stop sends stop condition
                                                                // Has to be continuous tranmition
    while ((I2C0_MRIS_R & I2C_MRIS_RIS) == 0);                  // Wait for register to be clear, busy bit, and be finished
}

uint8_t readI2c0Data(uint8_t add)
{
    // send start cond, send address, get ack,
    // rx data, send nack (no more data), send stop cond
    I2C0_MSA_R = (add << 1) | 1; // add:r/~w=1
    I2C0_MICR_R = I2C_MICR_IC;                                  // Were data is stored
    I2C0_MCS_R = I2C_MCS_START | I2C_MCS_RUN | I2C_MCS_STOP;    // start, run, and then sotp
    while ((I2C0_MRIS_R & I2C_MRIS_RIS) == 0);                  // wait till is idle again
    return I2C0_MDR_R;                                          // return data
}

// For devices with multiple registers
void writeI2c0Register(uint8_t add, uint8_t reg, uint8_t data)
{   
    // Third case on board
    // send start cond, send address, get ack, send register, get ack
    I2C0_MSA_R = add << 1 | 0; // add:r/~w=0
    I2C0_MDR_R = reg;                           
    I2C0_MICR_R = I2C_MICR_IC;
    I2C0_MCS_R = I2C_MCS_START | I2C_MCS_RUN;   // start and tranmit register. Fisrt data is register
    while ((I2C0_MRIS_R & I2C_MRIS_RIS) == 0);  // poll busy bit

    // write data to register, get ack, send stop cond
    I2C0_MDR_R = data;
    I2C0_MICR_R = I2C_MICR_IC;
    I2C0_MCS_R = I2C_MCS_RUN | I2C_MCS_STOP;    // this run transmits the data
    while (!(I2C0_MRIS_R & I2C_MRIS_RIS));      // poll busy bit
}

void writeI2c0Registers(uint8_t add, uint8_t reg, const uint8_t data[], uint8_t size)
{
    uint8_t i;
    // send start cond, send address, get ack
    // send reg, get ack
    I2C0_MSA_R = add << 1; // add:r/~w=0
    I2C0_MDR_R = reg;      // set register number
    if (size == 0)
    {
        // send start cond, send add, get ack, send reg, get ack, send stop cond
        I2C0_MICR_R = I2C_MICR_IC;
        I2C0_MCS_R = I2C_MCS_START | I2C_MCS_RUN | I2C_MCS_STOP;    // Stop since only data being sent is the register
        while ((I2C0_MRIS_R & I2C_MRIS_RIS) == 0);                  // poll busy bit
    }
    else
    {
        I2C0_MICR_R = I2C_MICR_IC;
        I2C0_MCS_R = I2C_MCS_START | I2C_MCS_RUN;       // Send address and registers
        while ((I2C0_MRIS_R & I2C_MRIS_RIS) == 0);      // Wait for ir, poll the busy bit   
        // first size-1 bytes
        // send data to register, get ack
        for (i = 0; i < size-1; i++)                    
        {
            I2C0_MDR_R = data[i];
            I2C0_MICR_R = I2C_MICR_IC;
            I2C0_MCS_R = I2C_MCS_RUN;                   // Run doesn't send the start or stop condition
            while ((I2C0_MRIS_R & I2C_MRIS_RIS) == 0);
        }
        // last byte
        // send data to register, get ack, send stop cond
        I2C0_MDR_R = data[size-1];
        I2C0_MICR_R = I2C_MICR_IC;
        I2C0_MCS_R = I2C_MCS_RUN | I2C_MCS_STOP;
        while ((I2C0_MRIS_R & I2C_MRIS_RIS) == 0);
    }
}

uint8_t readI2c0Register(uint8_t add, uint8_t reg)
{
    // set internal register counter in device
    // send start cond, send add, get ack, send reg, get ack
    I2C0_MSA_R = add << 1 | 0; // add:r/~w=0
    I2C0_MDR_R = reg;
    I2C0_MICR_R = I2C_MICR_IC;
    I2C0_MCS_R = I2C_MCS_START | I2C_MCS_RUN;       // Repeated start
    while ((I2C0_MRIS_R & I2C_MRIS_RIS) == 0);

    // read data from register
    // send start cond (repeated start), send add, get ack,
    // rx data from reg, send nack (no more data), send stop cond
    I2C0_MSA_R = (add << 1) | 1; // add:r/~w=1
    I2C0_MICR_R = I2C_MICR_IC;
    I2C0_MCS_R = I2C_MCS_START | I2C_MCS_RUN | I2C_MCS_STOP;
    while ((I2C0_MRIS_R & I2C_MRIS_RIS) == 0);
    return I2C0_MDR_R;                          // Raed data register and return value
}


// Read from the same register multiple times
// or read from more tha one register is increasing on registers
void readI2c0Registers(uint8_t add, uint8_t reg, uint8_t data[], uint8_t size)
{
    uint8_t i = 0;
    // send address and register number
    // send start cond, send add, get ack, send reg, get ack
    I2C0_MSA_R = add << 1; // add:r/~w=0
    I2C0_MDR_R = reg;
    I2C0_MICR_R = I2C_MICR_IC;
    I2C0_MCS_R = I2C_MCS_START | I2C_MCS_RUN;       // provide add and resgiter, and then run
    while ((I2C0_MRIS_R & I2C_MRIS_RIS) == 0);

    if (size == 1)
    {
        // send add and read one byte
        // send start cond (repeated start), send add, get ack,
        // rx data, send nack (no more data), send stop cond
        I2C0_MSA_R = (add << 1) | 1; // add:r/~w=1
        I2C0_MICR_R = I2C_MICR_IC;
        I2C0_MCS_R = I2C_MCS_START | I2C_MCS_RUN | I2C_MCS_STOP;        // Special case for a single register
        while ((I2C0_MRIS_R & I2C_MRIS_RIS) == 0);
        data[i++] = I2C0_MDR_R;                                         // Write into data array, where i is the number of register and the index on array 
    }
    else if (size > 1)
    {
        // Firts byte, last byte, and bytes in between 

        // first byte of read
        // send start cond (repeated start), send add, get ack,
        // rx data, send ack (more data)
        I2C0_MSA_R = (add << 1) | 1; // add:r/~w=1
        I2C0_MICR_R = I2C_MICR_IC;
        I2C0_MCS_R = I2C_MCS_START | I2C_MCS_RUN | I2C_MCS_ACK;     // Ack that there is more data
        while ((I2C0_MRIS_R & I2C_MRIS_RIS) == 0);
        data[i++] = I2C0_MDR_R;
        // read size-2 bytes
        // rx data, send ack (more data)
        while (i < size-1)
        {
            I2C0_MICR_R = I2C_MICR_IC;
            I2C0_MCS_R = I2C_MCS_RUN | I2C_MCS_ACK;                 // Ack there is more data
            while ((I2C0_MRIS_R & I2C_MRIS_RIS) == 0);
            data[i++] = I2C0_MDR_R;
        }
        // last byte of read
        // rx data, send nack (no more data), send stop cond
        I2C0_MICR_R = I2C_MICR_IC;
        I2C0_MCS_R = I2C_MCS_RUN | I2C_MCS_STOP;                    // get data, send stop bit, and no more data ack
        while ((I2C0_MRIS_R & I2C_MRIS_RIS) == 0);
        data[i++] = I2C0_MDR_R;
    }
}

bool pollI2c0Address(uint8_t add)           // Was there an ack
{
    // send start cond, send add, get ack (are you there?),
    // rx throw-away data, send nack (no more data), send stop cond
    I2C0_MSA_R = (add << 1) | 1; // add:r/~w=1
    I2C0_MICR_R = I2C_MICR_IC;
    I2C0_MCS_R = I2C_MCS_START | I2C_MCS_RUN | I2C_MCS_STOP;
    while ((I2C0_MRIS_R & I2C_MRIS_RIS) == 0);
    return !(I2C0_MCS_R & I2C_MCS_ERROR);
}

bool isI2c0Error(void)      // Reead error bit!
{
    return (I2C0_MCS_R & I2C_MCS_ERROR);
}



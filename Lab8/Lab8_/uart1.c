// UART0 Library
// Jason Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL
// Target uC:       TM4C123GH6PM
// System Clock:    -

// Hardware configuration:
// UART Interface:
//   U0TX (PA1) and U0RX (PA0) are connected to the 2nd controller
//   The USB on the 2nd controller enumerates to an ICDI interface and a virtual COM port

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"
#include "uart1.h"
#include "gpio.h"

//
#define UART_TX PORTB,1
#define UART_RX PORTB,0

//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// Initialize UART0
void initUart1(void)
{
    // Enable clocks
    SYSCTL_RCGCUART_R |= SYSCTL_RCGCUART_R1;
    _delay_cycles(3);
    enablePort(PORTB);

    // Configure UART0 pins
    selectPinPushPullOutput(UART_TX);
    selectPinDigitalInput(UART_RX);
    setPinAuxFunction(UART_TX, GPIO_PCTL_PB1_U1TX);
    setPinAuxFunction(UART_RX, GPIO_PCTL_PB0_U1RX);

    // Configure UART0 with default baud rate
    UART1_CTL_R = 0;                                    // turn-off UART0 to allow safe programming
    UART1_CC_R = UART_CC_CS_SYSCLK;                     // use system clock (usually 40 MHz)
}

// Set baud rate as function of instruction cycle frequency
void setUart1BaudRate(uint32_t baudRate, uint32_t fcyc)
{
    uint32_t divisorTimes128 = (fcyc * 8) / baudRate;   // calculate divisor (r) in units of 1/128,
                                                        // where r = fcyc / 16 * baudRate
    divisorTimes128 += 1;                               // add 1/128 to allow rounding
    UART1_CTL_R = 0;                                    // turn-off UART0 to allow safe programming
    UART1_IBRD_R = divisorTimes128 >> 7;                // set integer value to floor(r)
    UART1_FBRD_R = ((divisorTimes128) >> 1) & 63;       // set fractional value to round(fract(r)*64)
    UART1_LCRH_R = UART_LCRH_WLEN_8 | UART_LCRH_FEN;    // configure for 8N1 w/ 16-level FIFO
    UART1_CTL_R = UART_CTL_TXE | UART_CTL_RXE | UART_CTL_UARTEN;
                                                        // turn-on UART0
}

// Blocking function that writes a serial character when the UART buffer is not full
void putcUart1(char c)
{
    while (UART1_FR_R & UART_FR_TXFF);               // wait if uart0 tx fifo full
    UART1_DR_R = c;                                  // write character to fifo
}

// Blocking function that writes a string when the UART buffer is not full
void putsUart1(char* str)
{
    uint8_t i = 0;
    while (str[i] != '\0')
        putcUart1(str[i++]);
}

// Blocking function that returns with serial data once the buffer is not empty
char getcUart1(void)
{
    while (UART1_FR_R & UART_FR_RXFE);               // wait if uart0 rx fifo empty
    return UART1_DR_R & 0xFF;                        // get character from fifo
}

// Returns the status of the receive buffer
bool kbhitUart1(void)
{
    return !(UART1_FR_R & UART_FR_RXFE);
}

void getBytesUart1(uint8_t data[], uint8_t numBytes) {
    uint8_t i;

    for(i=0; i<numBytes; i++) {
        data[i] = getcUart1();
    }
}


//void parseFields1(COMMAND_DATA *data) {
//    // Numeric 48-57
//    // Alpha 65-90 and 97-122
//    // Everything else is a delimiter
//
//    char prev = 0;                              // assume previous char is delimiter
//    int i = 0;
//    data->fieldCount = 0;
//
//    char curr;
//
//    while(data->buffer[i] != 0) {
//
//        curr = data->buffer[i];
//
//        if( !((prev>=65 && prev<=90) || (prev>=97 && prev<=122) || (prev>=48 && prev<=57) || prev == '-') ) {    // Check if previous char IS a delimiter
//            if( (curr>=65 && curr<=90) || (curr>=97 && curr<=122) || (curr>=48 && curr<=57 || curr == '-') ) {   // Check if current char is NOT delimiter
//
//                if( (curr>=65 && curr<=90) || (curr>=97 && curr<=122) )
//                    data->fieldType[data->fieldCount] = 'a';
//
//                else if( (curr>=48 && curr<=57) || curr == '-' )
//                    data->fieldType[data->fieldCount] = 'n';
//
//                data->fieldPosition[data->fieldCount] = i;
//                data->fieldCount++;
//            }
//        }
//
//        prev = curr;
//
//        if(data->fieldCount >= MAX_FIELDS ) {
//            break;
//        }
//        i++;
//    }
//
//    i = 0;
//
//    while(data->buffer[i] != 0) {
//        if( !((data->buffer[i]>=65 && data->buffer[i]<=90) || (data->buffer[i]>=97 && data->buffer[i]<=122)) ) {    // if its not a letter
//            if( !(data->buffer[i]>=48 && data->buffer[i]<=57) && data->buffer[i] != '-' ) {                                                   // and not a number
//                data->buffer[i] = 0;                                                                                // set it to NULL
//            }
//        }
//        i++;
//    }
//
//    return;
//}
//
//char* getFieldString1(COMMAND_DATA* data, uint8_t fieldNumber) {
//
//    if(fieldNumber < data->fieldCount) {
//        return &data->buffer[data->fieldPosition[fieldNumber]];
//    } else {
//        return '\0';                                                // Previously had NULL
//    }
//}
//
//int32_t getFieldInteger1(COMMAND_DATA* data, uint8_t fieldNumber) {
//
//    int32_t num = 0;
//
//    if(fieldNumber < data->fieldCount) {
//        if((data->fieldType[fieldNumber]) == 'n') {
//            int temp = 0;
//            int i = 0;
//            bool neg = false;
//            bool hex = false;
//            char *c = &data->buffer[data->fieldPosition[fieldNumber]];
//
//            // handle negative #s
//            if(c[i] == '-') {
//                neg = true;
//                i++;
//            }
//
//            if(c[i] == '0' && c[i+1] == 'x') {          // check if hex
//                hex = true;
//                i += 2;                                 // if so skip 0 & x
//            }
//
//            while (c[i] != 0) {
//                // decimals
//                if(c[i] >= '0' && c[i] <= '9')
//                    temp = c[i] - 48;
//                // hex
//                else if(hex && ((c[i] >= 'A' && c[i] <= 'F') || (c[i] >= 'a' && c[i] <= 'f')))
//                    temp = (c[i] | 0x20) - 'a' + 10;    // convert A-F to 10-15 accordingly.
//
//                if(i) {
//                    num = num* ((hex) ? 16 : 10);       // Convert based on base
//                }
//
//                num += temp;
//                i++;
//            }
//
//            if(neg) num = -num;
//
//            return num;
//        }
//    }
//
//    return num;
//}
//
//bool isCommand1(COMMAND_DATA* data, const char strCommand[], uint8_t minArguments) {
//
//    if(data->buffer[0] == 0) return 0;
//
//    uint32_t i = 0, j = 0;
//    while(strCommand[i] != 0) { i++; }
//    while(data->buffer[j] != 0) { j++; }
//
//    if(i != j) return 0;                                            // same length?
//
//    uint32_t k = 0;
//    while(strCommand[k] != 0) {
//        if( !(strCommand[k] == data->buffer[k]) ) {                 // While not at the end strings compare chars
//            return 0;                                               // Mismatch, return 0;
//        }
//        k++;
//    }
//
//    if (data->fieldCount-1 == minArguments) {                       // required # of args?
//        return 1;
//    }
//
//    return 0;
//}

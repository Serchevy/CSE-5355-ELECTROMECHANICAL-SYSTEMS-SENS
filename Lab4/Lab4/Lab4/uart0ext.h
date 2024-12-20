// Serial Example -> lab4 code
// Jason Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL Evaluation Board
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

// Hardware configuration:
// Red LED:
//   PF1 drives an NPN transistor that powers the red LED
// Green LED:
//   PF3 drives an NPN transistor that powers the green LED
// UART Interface:
//   U0TX (PA1) and U0RX (PA0) are connected to the 2nd controller
//   The USB on the 2nd controller enumerates to an ICDI interface and a virtual COM port
//   Configured to 115,200 baud, 8N1

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------



#ifndef UART0EXT_H_
#define UART0EXT_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "clock.h"
#include "uart0.h"
#include "tm4c123gh6pm.h"

#define MAX_CHARS 80
#define MAX_FIELDS 5

typedef struct _USER_DATA
{
    char buffer[MAX_CHARS+1];   // plus one for null terminator
    uint8_t fieldCount;         // keeps track of field count
    uint8_t fieldPosition[MAX_FIELDS];  // where each field starts in buffer
    char fieldType[MAX_FIELDS]; // either d, n, a for each field
                                // last 3 are changed in parseFields()
} USER_DATA;

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

int strCompare( char* str1, char* str2 );
char* itoa(uint32_t num);
int isBool(char* str);
void getsUart0( USER_DATA *data );
void parseFields( USER_DATA *data );
char* getFieldString( USER_DATA* data, uint8_t fieldNumber );
int32_t getFieldInteger(USER_DATA* data, uint8_t fieldNumber);
bool isCommand(USER_DATA* data, const char strCommand[], uint8_t minArguments);


#endif


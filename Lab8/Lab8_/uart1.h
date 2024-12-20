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

#ifndef UART1_H_
#define UART1_H_

#include <stdint.h>
#include <stdbool.h>

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

#define MAX_CHARS 80
#define MAX_FIELDS 6

typedef struct _COMMAND_DATA {
    char buffer[MAX_CHARS+1];
    uint8_t fieldCount;
    uint8_t fieldPosition[MAX_FIELDS];
    char fieldType[MAX_FIELDS];
} COMMAND_DATA;

void initUart1();
void setUart1BaudRate(uint32_t baudRate, uint32_t fcyc);
void putcUart1(char c);
void putsUart1(char* str);
char getcUart1();
bool kbhitUart1();

void getBytesUart1(uint8_t data[], uint8_t numBytes);
//void parseFields1(COMMAND_DATA *data);
//char* getFieldString1(COMMAND_DATA* data, uint8_t fieldNumber);
//int32_t getFieldInteger1(COMMAND_DATA* data, uint8_t fieldNumber);
//bool isCommand1(COMMAND_DATA* data, const char strCommand[],uint8_t minArguments);

#endif

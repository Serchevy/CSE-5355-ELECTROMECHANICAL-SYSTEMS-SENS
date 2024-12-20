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

#include "uart0ext.h"

// indicate the maximum number of characters that can be accepted from the user
// and the structure for holding UI information


//#define DEBUG 1

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------



//-----------------------------------------------------------------------------
// strCompare() function
//-----------------------------------------------------------------------------
int strCompare( char* str1, char* str2 )
{
    char* str1Char = str1;
    char* str2Char = str2;
    uint8_t i = 0;

    while( str1Char[i] && (str1Char[i] == str2Char[i]) )
    {
            i++;
    }
    if( !str1Char[i] && !str2Char[i] )
    {
        return 0;
    }
    else
        return -1;
}


//-----------------------------------------------------------------------------
// isBool() function
//-----------------------------------------------------------------------------
int isBool(char* str)
{
    if(!strCompare(str, "ON") || !strCompare(str, "on"))
        return true;
    else if(!strCompare(str, "OFF") || !strCompare(str, "on"))
        return false;
    else
        return -1;
}


//-----------------------------------------------------------------------------
// getsUart0() function
//-----------------------------------------------------------------------------
void getsUart0( USER_DATA *data )
{
    uint8_t count = 0;

    // checks for backspace & printable char
    // exits if enter or reached end of buffer size MAX_CHARS
    while( true )
    {
        char c = getcUart0();
        // check for backspace
        if( ( c == 8 || c == 127) && count > 0 )
        {
            count--;
        }
        // checks for printable char
        else if( c >= 32 )
        {
            data -> buffer[ count ] = c;
            count++;

        }

        // if enter or maxed buffer
        if( c == 13 || count == MAX_CHARS )
        {
            data -> buffer[ count ] = '\0';
            return;
        }
    }
}

//-----------------------------------------------------------------------------
// parseFields() function
//-----------------------------------------------------------------------------
void parseFields( USER_DATA *data )
{
    data->fieldCount = 0;

    char previous = 'd'; //set to delimiter before parsing starts
    uint8_t i = 0;       // iterative variable to search buffer

    while( data->buffer[i] != '\0' )
    {

        //if it is A-Z or a-z
        if( (data->buffer[ i ] >= 'A' && data->buffer[ i ] <= 'Z') || (data->buffer[ i ] >= 'a' && data->buffer[ i ] <= 'z')  )
        {
            if( previous == 'd')
            {
                data->fieldType[ data->fieldCount ] = 'a';
                data->fieldPosition[ data->fieldCount ] = i;


                data->fieldCount++;
                previous = 'a';
            }
        }
        //if numeric 0-9 or negative sign
        else if( data->buffer[i] >= '0' && data->buffer[i] <= '9' || data->buffer[i] == '-' || data->buffer[i] == '.')
        {
            if( previous == 'd' )
            {
                data->fieldType[ data->fieldCount ] = 'a';
                data->fieldPosition[ data->fieldCount ] = i;


                data->fieldCount++;
                previous = 'a';
            }
        }
        else // not numeric & not alpha, delimiters set to null chars
        {
            previous = 'd';
            data->buffer[i] = '\0';
        }

        if( data->fieldCount == MAX_FIELDS )
        {
            return;
        }

        i++;
    }

}



//-----------------------------------------------------------------------------
// getFieldString(), part 7
//-----------------------------------------------------------------------------
char* getFieldString( USER_DATA* data, uint8_t fieldNumber )
{
    if( fieldNumber >= 0 && fieldNumber < data->fieldCount ) //what is fieldCounts initial value ?
    {
        return &data->buffer[ data->fieldPosition[ fieldNumber ] ];
    }
    else
    {
        return NULL;
    }
}


//-----------------------------------------------------------------------------
// getFieldInteger(), part 8
//-----------------------------------------------------------------------------
int32_t getFieldInteger(USER_DATA* data, uint8_t fieldNumber)
{
    if( fieldNumber >= 0 && fieldNumber < data->fieldCount && data->fieldType[ fieldNumber ] == 'n' )
        {

            int32_t value = atoi( &data->buffer[ data->fieldPosition[ fieldNumber ] ] );
            return value;
        }
        else
        {
            return NULL;
        }
}

//-----------------------------------------------------------------------------
// isCommand() part 9
//-----------------------------------------------------------------------------

bool isCommand(USER_DATA* data, const char strCommand[], uint8_t minArguments)
{
    if( strCompare( strCommand, &data->buffer[ data->fieldPosition[0] ] ) == 0 && data->fieldCount - 1 >= minArguments )
    {
        return true;
    }
    return false;
}


//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------
//
//int main(void)
//{
//    USER_DATA data;
//    initHw();
//
//
//    while(1)
//    {
//    // Get the string from the user
//    getsUart0(&data);
//    // Echo back to the user of the TTY interface for testing
//    #ifdef DEBUG
//        putsUart0(data.buffer);
//        putcUart0('\n');
//    #endif
//    // Parse fields
//    parseFields(&data);
//    // Echo back the parsed field data (type and fields)
//    #ifdef DEBUG
//        uint8_t i;
//        for (i = 0; i < data.fieldCount; i++)
//        {
//            putcUart0(data.fieldType[i]);
//            putcUart0('\t');
//            putsUart0(&data.buffer[ data.fieldPosition[i]]);
//            putcUart0('\n');
//        }
//    #endif
//    // Command evaluation
//    // set add, data -> add and data are integers
//    bool valid = 0;
//    if (isCommand(&data, "set", 2))
//    {
//        int32_t add = getFieldInteger(&data, 1);
//        int32_t Data = getFieldInteger(&data, 2);
//        valid = true;
//        // do something with this information
//    }
//    // alert ON|OFF -> alert ON or alert OFF are the expected commands.
//    if (isCommand(&data, "alert", 1))
//    {
//        char* str = getFieldString(&data, 1);
//        valid = true;
//        putsUart0(str);
//        // process the string with your custom strCompare instruction, then do something
//    }
//    // Process other commands here
//    // Look for error
//    if (!valid)
//        putsUart0("Invalid command\n");
//    }
//
//
//
//
//
//    while( true );
////    {
////        getsUart0(&data);
////        putsUart0(data.buffer);
////        putcUart0('\n');
////    }
//    return 0;
//}

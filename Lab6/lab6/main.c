#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "wait.h"
#include "gpio.h"
#include "clock.h"
#include "tm4c123gh6pm.h"
#include "uart0.h"
#include "uart0ext.h"
#include "i2c0.h"

#define CLEAR_PUTTY     "\033[2J"
#define HOME_POS        "\033[H"
#define HIDE_CURSOR     "\033[?25l"
#define SHOW_CURSOR     "\033[?25h"
#define CLEAR_LINE      "\033[2K"
#define SAVE_POS        "\033[s"
#define RESTORE_POS     "\033[u"

#define COVER_REG   0x0
#define CONFIG_REG  0x1
#define LO_THRE_REG 0x2
#define HI_THRE_REG 0x3
#define DEVICE_ADD  0x48    // 0b0100.1000

bool tBroken = false;

typedef struct nist_table{
    float temp;
    float voltage;
} nist_entry;

nist_entry nistTable[] = {
  {-80, -2.920}, {-70, -2.587}, {-60, -2.243}, {-50, -1.889},
  {-40, -1.527}, {-30, -1.156}, {-20, -0.778}, {-10, -0.392},
  {0, 0}, {10, 0.397}, {20, 0.798}, {30, 1.203}, {40, 1.612},
  {50, 2.023}, {60, 2.436}, {70, 2.851}, {80, 3.267}, {90, 3.682},
  {100, 4.096}, {110, 4.509}, {120, 4.920}, {130, 5.328}, {140, 5.735},
  {150, 6.138}, {160, 6.540}, {170, 6.941}, {180, 7.340}, {190, 7.739},
  {200, 8.138}, {210, 8.539}, {220, 8.940}, {230, 9.343}, {240, 9.747},
  {250, 10.153}, {260, 10.561}, {270, 10.971}, {280, 11.382}, {290, 11.795}, {300, 12.209}
};

void initHw()
{
    initSystemClockTo40Mhz();
    initUart0();
    setUart0BaudRate(115200, 40e6);

    // PB2 - SCL
    // PB3 - SDA
    initI2c0();
}

// interpolates from the nistTable to find approx temp corresponding to volt (mV)
float voltToTemp(float volt)
{
    int i=0;
    while(volt>nistTable[i].voltage)
        i++;
    // calculate temp as t1 + [(volt-v1)*(t2-t1)]/(v2-v1)
    float temp = (volt-nistTable[i-1].voltage)*(nistTable[i].temp-nistTable[i-1].temp);
    temp = temp/(nistTable[i].voltage-nistTable[i-1].voltage);
    temp =  nistTable[i-1].temp + temp;
    return temp;
}

// interpolate from the nistTable to find approx volt (mV) corresponding to temp
float tempToVolt(float temp)
{
    int i=0;
    while(temp>nistTable[i].temp)
        i++;
    // calculate volt as v1 + [(temp-t1)*(v2-v1)]/(t2-t1)
    float volt = (temp-nistTable[i-1].temp)*(nistTable[i].voltage-nistTable[i-1].voltage);
    volt = volt/(nistTable[i].temp-nistTable[i-1].temp);
    volt =  nistTable[i-1].voltage + volt;
    return volt;
}

float getTcj() {
    // Single Conversion        1
    // Config for AIN 2 & GND   110
    // FSR ±2.048 V             010     <-- Full Scale Resolution
    // Single shot mode         1
    // Data rate of 8 SPS       000
    // Comp mode traditional    0
    // Polarity active low      0
    // Non Latching Comp        0
    // Dis Comp & ALERT High    11

    //   E    5    0   3
    // 1110.0101 0000.0011
    uint8_t config[2] = {0xE5, 0x03};
    uint8_t status[2] = {0};
    uint8_t conver[2] = {0};
    int16_t rawResult;
    float Vout, Tcj;

    writeI2c0Registers(DEVICE_ADD, CONFIG_REG, config, 2);      // Configure AIN 2 & GND and start Single conversion

    do {
        readI2c0Registers(DEVICE_ADD, CONFIG_REG, status, 2);   // POLL OS bit (bit 15)
    } while(!(status[0] & 0x80));

    readI2c0Registers(DEVICE_ADD, COVER_REG, conver, 2);        // Get conversion raw value from Conversion Register
    rawResult = (conver[0]<<8) | conver[1];

    Vout = (rawResult * 62.5e-3);                               // convert to mV

    Tcj = (Vout-500)/10;                                        // Get Temp --> 10 mV/°C scale factor

    //Vout = (rawResult/32767.0) * 2.048;                   // convert voltage accordingly
    //Tcj =  (Vout - 0.5) * 100;                            // 10 mV/°C scale factor

    return Tcj;
}

float getVtc() {
    // Single Conversion        1
    // Config for AIN 0 & 1     000
    // FSR of ±0.256 V          101     <-- Full Scale Resolution
    // Single shot mode         1
    // Data rate of 8 SPS       000
    // Comp mode traditional    0
    // Polarity active low      0
    // Non Latching Comp        0
    // Dis Comp & ALERT High    11

    //   8    B    0    3
    // 1000.1011 0000.0011
    uint8_t config[2] = {0x8B, 0x03};
    uint8_t status[2] = {0};
    uint8_t conver[2] = {0};
    int16_t rawResult;
    float Vout;

    writeI2c0Registers(DEVICE_ADD, CONFIG_REG, config, 2);      // Configure AIN 0 & 1 and start Single conversion

    do {
        readI2c0Registers(DEVICE_ADD, CONFIG_REG, status, 2);   // POLL the OS bit (bit 15)
    } while(!(status[0] & 0x80));

    readI2c0Registers(DEVICE_ADD, COVER_REG, conver, 2);        // Get conversion raw value from Conversion Register
    rawResult = (conver[0]<<8) | conver[1];

    if(rawResult == 0x7FFF) {
        tBroken = true;
        return 0;
    }
    else {
        tBroken = false;
    }

    Vout = (rawResult * 7.8125e-3);                             // convert to mV

    //Vout = (rawResult/32767.0) * 0.256;                         // convert voltage accordingly

    return Vout;
}

void printData(float Tcj, float Vcj, float Vtc, float Ttc) {
    // Print Stuff to Putty
    char str[50];

    putsUart0(SAVE_POS);
    putsUart0(HOME_POS);

    putsUart0("----- DATA -----\n");
    snprintf(str, sizeof(str), "Tcj(C) : %.3f \n", Tcj);
    putsUart0(str);

    snprintf(str, sizeof(str), "Vcj(mV): %.3f \n", Vcj);
    putsUart0(str);

    snprintf(str, sizeof(str), "Vtc(mV): %.3f \n", Vtc);
    putsUart0(str);

    snprintf(str, sizeof(str), "Ttc(C) : %.3f \n", Ttc);
    putsUart0(str);

    if(tBroken)
        putsUart0("Thermocouple Disconnected/Broken!");
    else
        putsUart0(CLEAR_LINE);

    putsUart0(RESTORE_POS);
}

/**
 * main.c
 */
int main(void)
{
//    float temp1 = voltToTemp(-2.587);
//    float volt1 = tempToVolt(-25);

    initHw();

    USER_DATA data;

    float Tcj = 0;      // Cold Junction Temp and Volt (TMP36)
    float Vcj = 0;

    float Ttc = 0;      // Thermocouple Temp and Volt
    float Vtc = 0;
    float AddedVolt = 0;

    putsUart0(CLEAR_PUTTY);
    putsUart0(HOME_POS);
    putsUart0("\x1B[7;0H");

    printData(Tcj, Vcj, Vtc, Ttc);

    while(1)
    {
        Tcj = getTcj();                 // Get temperature @ Cold Junction (°C)
        Vcj = tempToVolt(Tcj);          // Convert temperature to voltage (mV)

        Vtc = getVtc();                 // Get voltage from Thermocoupler (mV)
        AddedVolt = Vcj + Vtc;          // Adjust voltage using voltage from cold junction
        Ttc = voltToTemp(AddedVolt);    // Convert voltage to temperature (°C)

        printData(Tcj, Vcj, Vtc, Ttc);

        if(kbhitUart0())
        {
            getsUart0(&data);
            parseFields(&data);

            char *input = getFieldString(&data, 0);

            if(!strCompare(input, "clear")) {
                putsUart0(CLEAR_PUTTY);
            }

            putsUart0("\x1B[7;0H");
            putsUart0(CLEAR_LINE);
        }

        waitMicrosecond(200e3);
    }

}

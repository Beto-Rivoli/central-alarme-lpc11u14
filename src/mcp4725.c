/*
 * mcp4725.c - Driver DAC MCP4725 (I2C)
 */

#include "mcp4725.h"
#include "i2c.h"

void mcp4725Init(void)
{
}

void mcp4725SetOutput(unsigned int value)
{
    if (value > 4095) value = 4095;

    unsigned char dados[3];
    dados[0] = 0x40;
    dados[1] = (value >> 4) & 0xFF;
    dados[2] = (value & 0x0F) << 4;

    i2cSend(MCP4725_ADDR, dados, 3);
}

void mcp4725SetOutputEEPROM(unsigned int value)
{
    if (value > 4095) value = 4095;

    unsigned char dados[3];
    dados[0] = 0x60;
    dados[1] = (value >> 4) & 0xFF;
    dados[2] = (value & 0x0F) << 4;

    i2cSend(MCP4725_ADDR, dados, 3);
}

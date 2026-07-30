/*
 * i2c.c - Driver I2C (LPC11U14)
 */

#include "i2c.h"
#include "LPC11Uxx.h"
#include "bits.h"
#include "io.h"
#include "serial.h"

void i2cInit(void)
{
    bitSet(LPC_SYSCON->SYSAHBCLKCTRL, 16);
    LPC_IOCON->PIO0_4 = 0x01;
    LPC_IOCON->PIO0_5 = 0x01;

    bitSet(LPC_SYSCON->SYSAHBCLKCTRL, 5);
    bitSet(LPC_SYSCON->PRESETCTRL, 1);

    LPC_I2C->SCLH = 240;
    LPC_I2C->SCLL = 240;

    bitSet(LPC_I2C->CONSET, 6);
}

void i2cSend(unsigned char endereco, unsigned char *dados, unsigned char qtd)
{
    unsigned char i;

    LPC_I2C->CONSET = (1 << 5);
    while ((LPC_I2C->STAT != 0x08) && (LPC_I2C->STAT != 0x10));

    LPC_I2C->DAT = endereco << 1;
    LPC_I2C->CONCLR = (1 << 3) | (1 << 5);
    while (LPC_I2C->STAT != 0x18) {
        if (LPC_I2C->STAT == 0x20) return;
    }

    for (i = 0; i < qtd; i++) {
        LPC_I2C->DAT = dados[i];
        LPC_I2C->CONCLR = (1 << 3);
        while (LPC_I2C->STAT != 0x28) {
            if (LPC_I2C->STAT == 0x30) return;
        }
    }

    LPC_I2C->CONSET = (1 << 4);
    LPC_I2C->CONCLR = (1 << 3);
}

void i2cReceive(unsigned char endereco, unsigned char *dados, unsigned char qtd)
{
    unsigned char i;     
    
    LPC_I2C->CONSET = (1 << 5);
    while ((LPC_I2C->STAT != 0x08) && (LPC_I2C->STAT != 0x10));

    LPC_I2C->DAT = (endereco << 1) | 1;
    LPC_I2C->CONCLR = (1 << 3) | (1 << 5);
    
    while (LPC_I2C->STAT != 0x40) {
        if (LPC_I2C->STAT == 0x48) return; 
    }
    
    for (volatile int i = 0; i < 1000; i++);

    for (i = 0; i < qtd; i++) {
        if (i < qtd - 1) {
            LPC_I2C->CONSET = (1 << 2);
        } else {
            LPC_I2C->CONCLR = (1 << 2);
        }

        LPC_I2C->CONCLR = (1 << 3);

        while ((LPC_I2C->STAT != 0x50) && (LPC_I2C->STAT != 0x58));

        dados[i] = LPC_I2C->DAT;
    }

    LPC_I2C->CONSET = (1 << 4);
    LPC_I2C->CONCLR = (1 << 3);
}

unsigned char i2cReadReg(unsigned char addr, unsigned char reg)
{
    unsigned char data;

    i2cSend(addr, &reg, 1);
    i2cReceive(addr, &data, 1);

    return data;
}

void i2cWriteReg(unsigned char addr, unsigned char reg, unsigned char data)
{
    unsigned char buffer[2];

    buffer[0] = reg;
    buffer[1] = data;

    i2cSend(addr, buffer, 2);
}
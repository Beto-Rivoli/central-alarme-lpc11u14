/*
 * eeprom_24lc512.c - Driver para EEPROM 24LC512 (I2C)
 */

#include "eeprom_24lc512.h"
#include "i2c.h"
#include "bits.h"

void I2C_EEPROM_Init(void)
{
    i2cInit();
}

void EEPROM_WriteByte(uint16_t mem_addr, uint8_t data)
{
    unsigned char buffer[3];
    
    buffer[0] = (uint8_t)(mem_addr >> 8);
    buffer[1] = (uint8_t)(mem_addr & 0xFF);
    buffer[2] = data;

    i2cSend(EEPROM_I2C_ADDR, buffer, 3);

    /* Ciclo de escrita interna (máx 5ms) */
    for (volatile uint32_t i = 0; i < 30000; i++);
}

void EEPROM_ReadByte(uint16_t mem_addr, uint8_t *data)
{
    unsigned char addr_buf[2];
    
    addr_buf[0] = (uint8_t)(mem_addr >> 8);
    addr_buf[1] = (uint8_t)(mem_addr & 0xFF);

    i2cSend(EEPROM_I2C_ADDR, addr_buf, 2);
    for (volatile uint32_t i = 0; i < 200; i++);

    i2cReceive(EEPROM_I2C_ADDR, data, 1);
}

void EEPROM_WritePage(uint16_t mem_addr, uint8_t *data, uint16_t len)
{
    while (len > 0) {
        uint16_t page_offset = mem_addr % EEPROM_PAGE_SIZE;
        uint16_t bytes_to_end = EEPROM_PAGE_SIZE - page_offset;
        uint16_t chunk = (len < bytes_to_end) ? len : bytes_to_end;

        unsigned char buffer[130];
        buffer[0] = (uint8_t)(mem_addr >> 8);
        buffer[1] = (uint8_t)(mem_addr & 0xFF);
        for (uint16_t i = 0; i < chunk; i++) {
            buffer[2 + i] = data[i];
        }

        i2cSend(EEPROM_I2C_ADDR, buffer, 2 + chunk);

        for (volatile uint32_t i = 0; i < 30000; i++);

        mem_addr += chunk;
        data += chunk;
        len -= chunk;
    }
}

void EEPROM_ReadSequential(uint16_t mem_addr, uint8_t *data, uint16_t len)
{
    unsigned char addr_buf[2];
    
    addr_buf[0] = (uint8_t)(mem_addr >> 8);
    addr_buf[1] = (uint8_t)(mem_addr & 0xFF);

    i2cSend(EEPROM_I2C_ADDR, addr_buf, 2);
    for (volatile uint32_t i = 0; i < 200; i++);

    i2cReceive(EEPROM_I2C_ADDR, data, len);
}

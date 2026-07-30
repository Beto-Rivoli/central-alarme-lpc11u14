/*
 * lcd.c - Driver para display LCD 16x2 (HD44780) em modo 4 bits
 *
 * Mapeamento (Porta 1):
 *   RS = PIO1_31, EN = PIO1_28
 *   D4 = PIO1_19, D5 = PIO1_20, D6 = PIO1_21, D7 = PIO1_22
 *
 * NOTA: PIO1_31 usa bit 31,shifts usam 1U para evitar UB.
 */

#include "LPC11Uxx.h"
#include "lcd.h"

#define LCD_ALL_PINS  ( (1U << LCD_RS_PIN) | (1U << LCD_EN_PIN) | \
                        (1U << LCD_D4_PIN) | (1U << LCD_D5_PIN) | \
                        (1U << LCD_D6_PIN) | (1U << LCD_D7_PIN) )

#define LCD_DATA_MASK ( (1U << LCD_D4_PIN) | (1U << LCD_D5_PIN) | \
                        (1U << LCD_D6_PIN) | (1U << LCD_D7_PIN) )

static void LCD_DelayUs(uint32_t us)
{
    volatile uint32_t i;
    for (i = 0; i < (us * 12); i++);
}

static void LCD_DelayMs(uint32_t ms)
{
    uint32_t i;
    for (i = 0; i < ms; i++) {
        LCD_DelayUs(1000);
    }
}

static void LCD_Pulse(void)
{
    LPC_GPIO->SET[1] = (1U << LCD_EN_PIN);
    LCD_DelayUs(5);
    LPC_GPIO->CLR[1] = (1U << LCD_EN_PIN);
    LCD_DelayUs(5);
}

static void LCD_SendNibble(uint8_t nibble)
{
    LPC_GPIO->CLR[1] = LCD_DATA_MASK;

    if (nibble & 0x01) LPC_GPIO->SET[1] = (1U << LCD_D4_PIN);
    if (nibble & 0x02) LPC_GPIO->SET[1] = (1U << LCD_D5_PIN);
    if (nibble & 0x04) LPC_GPIO->SET[1] = (1U << LCD_D6_PIN);
    if (nibble & 0x08) LPC_GPIO->SET[1] = (1U << LCD_D7_PIN);

    LCD_Pulse();
}

void LCD_Command(uint8_t cmd)
{
    LPC_GPIO->CLR[1] = (1U << LCD_RS_PIN);
    LCD_SendNibble(cmd >> 4);
    LCD_SendNibble(cmd & 0x0F);
    LCD_DelayMs(2);
}

void LCD_Char(char data)
{
    LPC_GPIO->SET[1] = (1U << LCD_RS_PIN);
    LCD_SendNibble((uint8_t)data >> 4);
    LCD_SendNibble((uint8_t)data & 0x0F);
    LCD_DelayMs(2);
}

void LCD_String(const char *str)
{
    while (*str) {
        LCD_Char(*str++);
    }
}

void LCD_Clear(void)
{
    LCD_Command(LCD_CMD_CLEAR);
    LCD_DelayMs(2);
}

void LCD_Home(void)
{
    LCD_Command(LCD_CMD_HOME);
    LCD_DelayMs(2);
}

void LCD_SetCursor(uint8_t row, uint8_t col)
{
    const uint8_t row_offsets[] = { 0x00, 0x40 };
    if (row > 1) row = 1;
    if (col > 15) col = 15;
    LCD_Command(LCD_CMD_SET_DDRAM | (row_offsets[row] + col));
}

void LCD_Number(int num)
{
    char buf[12];
    int i = 0;
    int neg = 0;

    if (num < 0) {
        neg = 1;
        num = -num;
    }

    if (num == 0) {
        LCD_Char('0');
        return;
    }

    while (num > 0) {
        buf[i++] = '0' + (num % 10);
        num /= 10;
    }

    if (neg) {
        LCD_Char('-');
    }

    while (i > 0) {
        LCD_Char(buf[--i]);
    }
}

void LCD_Init(void)
{
    LPC_SYSCON->SYSAHBCLKCTRL |= (1U << 6) | (1U << 16);

    LPC_IOCON->PIO1_31 = 0x00;
    LPC_IOCON->PIO1_28 = 0x00;
    LPC_IOCON->PIO1_19 = 0x00;
    LPC_IOCON->PIO1_20 = 0x00;
    LPC_IOCON->PIO1_21 = 0x00;
    LPC_IOCON->PIO1_22 = 0x00;

    LPC_GPIO->DIR[1] |= LCD_ALL_PINS;
    LPC_GPIO->CLR[1] = LCD_ALL_PINS;

    LCD_DelayMs(300);

    LPC_GPIO->CLR[1] = (1U << LCD_RS_PIN);

    LCD_SendNibble(0x03);
    LCD_DelayMs(5);

    LCD_SendNibble(0x03);
    LCD_DelayUs(200);

    LCD_SendNibble(0x03);
    LCD_DelayUs(200);

    LCD_SendNibble(0x02);
    LCD_DelayMs(5);

    LCD_Command(LCD_CMD_4BIT_2LINE);
    LCD_Command(LCD_CMD_DISP_ON);
    LCD_Command(LCD_CMD_ENTRY_MODE);
    LCD_Clear();
}

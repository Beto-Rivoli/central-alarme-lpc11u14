/*
 * serial.c - Driver UART (9600 8N1)
 */

#include "serial.h"
#include "LPC11Uxx.h"
#include "mcp7940.h"

void serialInit(void)
{
    LPC_SYSCON->SYSAHBCLKCTRL |= (1 << 16);
    LPC_IOCON->PIO0_18 = 0x01;
    LPC_IOCON->PIO0_19 = 0x01;

    LPC_SYSCON->SYSAHBCLKCTRL |= (1 << 12);
    LPC_SYSCON->UARTCLKDIV = 4;
    LPC_USART->LCR = 0x83;

    /* Baudrate 9600 @ 12MHz UART_PCLK */
    LPC_USART->DLM = 0x0;
    LPC_USART->DLL = 0x34;
    LPC_USART->FDR = 0x21;
    LPC_USART->FCR |= 0x07;

    LPC_USART->LCR = 0x03;
    LPC_USART->TER = 0x80;
}

void serialSendChar(char c)
{
    uint32_t timeout = 100000;
    while (((LPC_USART->LSR & (1 << 5)) == 0) && (timeout > 0)) {
        timeout--;
    }
    LPC_USART->THR = c;
}

void serialSendString(const char *str)
{
    while (*str) {
        serialSendChar(*str++);
    }
}

int serialReadChar(void)
{
    if (LPC_USART->LSR & (1 << 0)) {
        return LPC_USART->RBR;
    }
    return -1;
}

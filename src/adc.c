/*
 * adc.c - Conversor ADC (LPC11U14)
 */

#include "adc.h"
#include "LPC11Uxx.h"
#include "bits.h"

void adcInit(void)
{
    bitSet(LPC_SYSCON->SYSAHBCLKCTRL, 16);
    bitSet(LPC_SYSCON->SYSAHBCLKCTRL, 13);
    bitClr(LPC_SYSCON->PDRUNCFG, 4);

    LPC_IOCON->TDI_PIO0_11 = 0x2;
    LPC_ADC->CR = (1 << 0) | (11 << 8);      
}

unsigned int adcRead(void)
{
    LPC_ADC->CR |= (1 << 24);
    while ((LPC_ADC->DR[0] & (1 << 31)) == 0);
    return (LPC_ADC->DR[0] >> 6) & 0x3FF;
}

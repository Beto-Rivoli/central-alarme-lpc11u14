/*
 * pwm.c - Saída PWM via Timer 32-bit (CT32B0_MAT0 em PIO0_18)
 */

#include "pwm.h"
#include "LPC11Uxx.h"
#include "bits.h"

#define PWM_PERIOD 48000  /* 1 kHz @ 48 MHz */

void pwmInit(void)
{
    bitSet(LPC_SYSCON->SYSAHBCLKCTRL, 16);
    bitSet(LPC_SYSCON->SYSAHBCLKCTRL, 9);

    LPC_IOCON->PIO0_18 = 0x2;
    LPC_CT32B0->CTCR = 0x0;
    LPC_CT32B0->PR = 0;
    LPC_CT32B0->MR3 = PWM_PERIOD;
    LPC_CT32B0->MCR = (1 << 10);
    LPC_CT32B0->MR0 = 0;
    LPC_CT32B0->PWMC = (1 << 0);
    LPC_CT32B0->TC = 0;
    LPC_CT32B0->PC = 0;
    LPC_CT32B0->TCR = 1;
}

void pwmSetDuty(uint32_t duty)
{
    if (duty > 1000) {
        duty = 1000;
    }
    LPC_CT32B0->MR0 = (duty * PWM_PERIOD) / 1000;
}

/*
 * bod.c - Detecção de Brown-Out (BOD)
 */

#include "bod.h"
#include "LPC11Uxx.h"
#include "var.h"
#include "alarm.h"

#define BODRSTLEV_2V35      (0x02 << 0)
#define BODINTVAL_2V80      (0x03 << 2)
#define BODRSTENA           (1 << 4)

void BOD_Init(void)
{
    LPC_SYSCON->BODCTRL = BODRSTLEV_2V35 | BODINTVAL_2V80 | BODRSTENA;

    NVIC_SetPriority(BOD_IRQn, 0);
    NVIC_EnableIRQ(BOD_IRQn);
}

void BOD_IRQHandler(void)
{
    ALARM_Deactivate();
    VAR_SaveEmergency();

    while (1) {
        __WFI();
    }
}

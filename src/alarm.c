/*
 * alarm.c - Módulo de lógica de alarme
 */

#include "alarm.h"
#include "var.h"
#include "pwm.h"
#include "io.h"
#include "LPC11Uxx.h"

static uint8_t alarm_active = 0;

void ALARM_Init(void)
{
    LPC_IOCON->PIO1_24 = 0x00;
    pinMode(LED0, OUTPUT);
    digitalWrite(LED0, LOW);
    pwmSetDuty(0);

    alarm_active = 0;
}

uint8_t ALARM_Check(uint16_t adcVal)
{
    uint16_t hi = VAR_GetAlarmHi();
    uint16_t lo = VAR_GetAlarmLo();

    if (adcVal > hi) {
        return ALARM_HI;
    }
    if (adcVal < lo) {
        return ALARM_LO;
    }
    return ALARM_OK;
}

void ALARM_Activate(void)
{
    if (!alarm_active) {
        alarm_active = 1;
        pwmSetDuty(500);
        digitalWrite(LED0, HIGH);
    }
}

void ALARM_Deactivate(void)
{
    if (alarm_active) {
        alarm_active = 0;
        pwmSetDuty(0);
        digitalWrite(LED0, LOW);
    }
}

uint8_t ALARM_IsActive(void)
{
    return alarm_active;
}

/*
 * alarm.h - Módulo de lógica de alarme
 */

#ifndef ALARM_H
#define ALARM_H

#include <stdint.h>

#define ALARM_OK        0
#define ALARM_HI        1
#define ALARM_LO        2

void ALARM_Init(void);
uint8_t ALARM_Check(uint16_t adcVal);
void ALARM_Activate(void);
void ALARM_Deactivate(void);
uint8_t ALARM_IsActive(void);

#endif /* ALARM_H */

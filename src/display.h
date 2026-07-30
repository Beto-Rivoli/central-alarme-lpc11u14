/*
 * display.h - Módulo de IHM (LCD 16x2)
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include "stateMachine.h"

void DISP_ShowMenu(state_t st, uint8_t lang);
void DISP_ShowSubConfig(state_t st, uint8_t lang);
void DISP_ShowADC(uint16_t adcVal, uint8_t lang);
void DISP_ShowEditLimits(uint16_t hi, uint16_t lo, uint8_t cursor, uint8_t lang);
void DISP_ShowEditLang(uint8_t lang);
void DISP_ShowClock(uint32_t elapsedSec, uint8_t cursor, uint8_t lang);
void DISP_ShowControl(uint8_t alarmOn, uint8_t cursor, uint8_t lang);
void DISP_ShowAlarmAlert(uint16_t adcVal, uint8_t hiOrLo, uint8_t lang);

#endif /* DISPLAY_H */

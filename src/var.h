/*
 * var.h - Módulo de variáveis persistentes
 */

#ifndef VAR_H
#define VAR_H

#include <stdint.h>

#define LANG_PT     0
#define LANG_EN     1

#define ALARM_TYPE_HI   1
#define ALARM_TYPE_LO   2

#define DEFAULT_ALARM_HI    800
#define DEFAULT_ALARM_LO    200
#define DEFAULT_LANGUAGE    LANG_PT

#define MAX_LOGS            3

typedef struct {
    uint32_t timestamp;
    uint16_t adcValue;
    uint8_t  alarmType;
    uint8_t  reserved;
} alarmLog_t;

typedef struct {
    uint8_t   language;
    uint16_t  alarmHi;
    uint16_t  alarmLo;
    uint8_t   lastState;
    uint8_t   logCount;
    uint8_t   isArmed;
    alarmLog_t logs[MAX_LOGS];
} sysVars_t;

void VAR_Init(void);
void VAR_Save(void);
void VAR_SaveEmergency(void);

uint8_t VAR_GetLanguage(void);
void VAR_SetLanguage(uint8_t lang);

uint16_t VAR_GetAlarmHi(void);
void VAR_SetAlarmHi(uint16_t val);

uint16_t VAR_GetAlarmLo(void);
void VAR_SetAlarmLo(uint16_t val);

uint8_t VAR_GetLastState(void);
void VAR_SetLastState(uint8_t st);

void VAR_LogEvent(alarmLog_t log);
alarmLog_t VAR_GetLog(uint8_t index);
uint8_t VAR_GetLogCount(void);

const sysVars_t* VAR_GetAll(void);

uint8_t VAR_IsArmed(void);
void VAR_SetArmed(uint8_t armed);

#endif /* VAR_H */

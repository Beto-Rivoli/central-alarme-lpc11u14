/*
 * var.c - Módulo de variáveis persistentes (EEPROM 24LC512)
 *
 * Mapa de Endereços EEPROM:
 *   0x0100 — Magic byte (0xA5 = dados válidos)
 *   0x0101 — Idioma (1 byte)
 *   0x0102 — Alarme Hi MSB, 0x0103 — Alarme Hi LSB
 *   0x0104 — Alarme Lo MSB, 0x0105 — Alarme Lo LSB
 *   0x0106 — Último estado salvo (1 byte)
 *   0x0107 — Número de logs (1 byte)
 *   0x0110..0x0117 — Log evento 0 (8 bytes)
 *   0x0118..0x011F — Log evento 1 (8 bytes)
 *   0x0120..0x0127 — Log evento 2 (8 bytes)
 */

#include "var.h"
#include "eeprom_24lc512.h"

#define EEPROM_MAGIC_ADDR       0x0100
#define EEPROM_MAGIC_VALUE      0xA5

#define EEPROM_LANG_ADDR        0x0101
#define EEPROM_HI_MSB_ADDR      0x0102
#define EEPROM_HI_LSB_ADDR      0x0103
#define EEPROM_LO_MSB_ADDR      0x0104
#define EEPROM_LO_LSB_ADDR      0x0105
#define EEPROM_STATE_ADDR       0x0106
#define EEPROM_LOGCOUNT_ADDR    0x0107
#define EEPROM_ARMED_ADDR       0x0108

#define EEPROM_LOG0_ADDR        0x0110
#define EEPROM_LOG1_ADDR        0x0118
#define EEPROM_LOG2_ADDR        0x0120

#define LOG_ENTRY_SIZE          8

static sysVars_t sysVars;

static void writeLogEntry(uint16_t baseAddr, const alarmLog_t *log)
{
    uint8_t buf[LOG_ENTRY_SIZE];
    buf[0] = (uint8_t)(log->timestamp >> 24);
    buf[1] = (uint8_t)(log->timestamp >> 16);
    buf[2] = (uint8_t)(log->timestamp >> 8);
    buf[3] = (uint8_t)(log->timestamp & 0xFF);
    buf[4] = (uint8_t)(log->adcValue >> 8);
    buf[5] = (uint8_t)(log->adcValue & 0xFF);
    buf[6] = log->alarmType;
    buf[7] = 0x00;

    EEPROM_WritePage(baseAddr, buf, LOG_ENTRY_SIZE);
}

static void readLogEntry(uint16_t baseAddr, alarmLog_t *log)
{
    uint8_t buf[LOG_ENTRY_SIZE];
    EEPROM_ReadSequential(baseAddr, buf, LOG_ENTRY_SIZE);

    log->timestamp = ((uint32_t)buf[0] << 24) |
                     ((uint32_t)buf[1] << 16) |
                     ((uint32_t)buf[2] << 8)  |
                     ((uint32_t)buf[3]);
    log->adcValue  = ((uint16_t)buf[4] << 8) | buf[5];
    log->alarmType = buf[6];
    log->reserved  = 0;
}

static uint16_t logAddr(uint8_t index)
{
    switch (index) {
        case 0:  return EEPROM_LOG0_ADDR;
        case 1:  return EEPROM_LOG1_ADDR;
        case 2:  return EEPROM_LOG2_ADDR;
        default: return EEPROM_LOG0_ADDR;
    }
}

static void updateEEPROMByte(uint16_t addr, uint8_t val)
{
    uint8_t old = 0;
    EEPROM_ReadByte(addr, &old);
    if (old != val) {
        EEPROM_WriteByte(addr, val);
    }
}

void VAR_Init(void)
{
    uint8_t magic = 0x00;
    EEPROM_ReadByte(EEPROM_MAGIC_ADDR, &magic);

    if (magic == EEPROM_MAGIC_VALUE) {
        uint8_t hi_msb, hi_lsb, lo_msb, lo_lsb;

        EEPROM_ReadByte(EEPROM_LANG_ADDR, &sysVars.language);
        EEPROM_ReadByte(EEPROM_HI_MSB_ADDR, &hi_msb);
        EEPROM_ReadByte(EEPROM_HI_LSB_ADDR, &hi_lsb);
        EEPROM_ReadByte(EEPROM_LO_MSB_ADDR, &lo_msb);
        EEPROM_ReadByte(EEPROM_LO_LSB_ADDR, &lo_lsb);
        EEPROM_ReadByte(EEPROM_STATE_ADDR, &sysVars.lastState);
        EEPROM_ReadByte(EEPROM_LOGCOUNT_ADDR, &sysVars.logCount);
        EEPROM_ReadByte(EEPROM_ARMED_ADDR, &sysVars.isArmed);

        sysVars.alarmHi = ((uint16_t)hi_msb << 8) | hi_lsb;
        sysVars.alarmLo = ((uint16_t)lo_msb << 8) | lo_lsb;

        if (sysVars.alarmHi > 1023) sysVars.alarmHi = 1023;
        if (sysVars.alarmLo > 1023) sysVars.alarmLo = 0;
        if (sysVars.language > LANG_EN) sysVars.language = LANG_PT;
        if (sysVars.logCount > MAX_LOGS) sysVars.logCount = 0;
        if (sysVars.isArmed > 1) sysVars.isArmed = 1;

        uint8_t i;
        for (i = 0; i < sysVars.logCount && i < MAX_LOGS; i++) {
            readLogEntry(logAddr(i), &sysVars.logs[i]);
        }
    } else {
        sysVars.language  = DEFAULT_LANGUAGE;
        sysVars.alarmHi   = DEFAULT_ALARM_HI;
        sysVars.alarmLo   = DEFAULT_ALARM_LO;
        sysVars.lastState = 1;
        sysVars.logCount  = 0;
        sysVars.isArmed   = 1;

        uint8_t i;
        for (i = 0; i < MAX_LOGS; i++) {
            sysVars.logs[i].timestamp = 0;
            sysVars.logs[i].adcValue  = 0;
            sysVars.logs[i].alarmType = 0;
            sysVars.logs[i].reserved  = 0;
        }

        VAR_Save();
    }
}

void VAR_Save(void)
{
    updateEEPROMByte(EEPROM_MAGIC_ADDR, EEPROM_MAGIC_VALUE);
    updateEEPROMByte(EEPROM_LANG_ADDR, sysVars.language);
    updateEEPROMByte(EEPROM_HI_MSB_ADDR, (uint8_t)(sysVars.alarmHi >> 8));
    updateEEPROMByte(EEPROM_HI_LSB_ADDR, (uint8_t)(sysVars.alarmHi & 0xFF));
    updateEEPROMByte(EEPROM_LO_MSB_ADDR, (uint8_t)(sysVars.alarmLo >> 8));
    updateEEPROMByte(EEPROM_LO_LSB_ADDR, (uint8_t)(sysVars.alarmLo & 0xFF));
    updateEEPROMByte(EEPROM_STATE_ADDR, sysVars.lastState);
    updateEEPROMByte(EEPROM_LOGCOUNT_ADDR, sysVars.logCount);
    updateEEPROMByte(EEPROM_ARMED_ADDR, sysVars.isArmed);

    uint8_t i;
    for (i = 0; i < sysVars.logCount && i < MAX_LOGS; i++) {
        writeLogEntry(logAddr(i), &sysVars.logs[i]);
    }
}

void VAR_SaveEmergency(void)
{
    EEPROM_WriteByte(EEPROM_MAGIC_ADDR, EEPROM_MAGIC_VALUE);
    EEPROM_WriteByte(EEPROM_LANG_ADDR, sysVars.language);
    EEPROM_WriteByte(EEPROM_HI_MSB_ADDR, (uint8_t)(sysVars.alarmHi >> 8));
    EEPROM_WriteByte(EEPROM_HI_LSB_ADDR, (uint8_t)(sysVars.alarmHi & 0xFF));
    EEPROM_WriteByte(EEPROM_LO_MSB_ADDR, (uint8_t)(sysVars.alarmLo >> 8));
    EEPROM_WriteByte(EEPROM_LO_LSB_ADDR, (uint8_t)(sysVars.alarmLo & 0xFF));
    EEPROM_WriteByte(EEPROM_STATE_ADDR, sysVars.lastState);
}

uint8_t VAR_GetLanguage(void)
{
    return sysVars.language;
}

void VAR_SetLanguage(uint8_t lang)
{
    if (lang <= LANG_EN) {
        sysVars.language = lang;
    }
}

uint16_t VAR_GetAlarmHi(void)
{
    return sysVars.alarmHi;
}

void VAR_SetAlarmHi(uint16_t val)
{
    if (val <= 1023) {
        sysVars.alarmHi = val;
    }
}

uint16_t VAR_GetAlarmLo(void)
{
    return sysVars.alarmLo;
}

void VAR_SetAlarmLo(uint16_t val)
{
    if (val <= 1023) {
        sysVars.alarmLo = val;
    }
}

uint8_t VAR_GetLastState(void)
{
    return sysVars.lastState;
}

void VAR_SetLastState(uint8_t st)
{
    sysVars.lastState = st;
}

void VAR_LogEvent(alarmLog_t log)
{
    for (int i = MAX_LOGS - 1; i > 0; i--) {
        sysVars.logs[i] = sysVars.logs[i - 1];
    }
    sysVars.logs[0] = log;

    if (sysVars.logCount < MAX_LOGS) {
        sysVars.logCount++;
    }
}

alarmLog_t VAR_GetLog(uint8_t index)
{
    alarmLog_t empty = {0, 0, 0, 0};
    if (index < sysVars.logCount && index < MAX_LOGS) {
        return sysVars.logs[index];
    }
    return empty;
}

uint8_t VAR_GetLogCount(void)
{
    return sysVars.logCount;
}

const sysVars_t* VAR_GetAll(void)
{
    return &sysVars;
}

uint8_t VAR_IsArmed(void)
{
    return sysVars.isArmed;
}

void VAR_SetArmed(uint8_t armed)
{
    sysVars.isArmed = armed ? 1 : 0;
}

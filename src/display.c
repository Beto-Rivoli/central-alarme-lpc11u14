/*
 * display.c - Módulo de IHM (LCD 16x2)
 */

#include "display.h"
#include "lcd.h"
#include "var.h"

/* Tabelas de strings bilíngues [LANG_PT=0][LANG_EN=1] */
static const char * const str_config[]   = { "CONFIG",   "CONFIG"   };
static const char * const str_adc[]      = { "ADC",      "ADC"      };
static const char * const str_control[]  = { "CONTROLE", "CONTROL"  };

static const char * const str_cfgtitle[] = { "CONFIGURACAO",   "SETTINGS"      };
static const char * const str_clock[]    = { "> Cronometro",   "> Stopwatch"   };
static const char * const str_limits[]   = { "> Limites Hi/Lo","> Limits Hi/Lo"};
static const char * const str_langopt[]  = { "> Idioma: ",     "> Language: "  };

static const char * const str_langtitle[]= { "Idioma/Language","Idioma/Language"};
static const char * const str_clocktit[] = { "Cronometro",     "Stopwatch"     };
static const char * const str_ctrltit[]  = { "CONTROLE",       "CONTROL"       };
static const char * const str_alert[]    = { "!! ALARME !!",   "!! ALARM !!"   };

/* Buffer anti-flicker */
static char lbuf[2][17];
static uint8_t lpos[2];

static void bufInit(void)
{
    uint8_t i;
    for (i = 0; i < 16; i++) {
        lbuf[0][i] = ' ';
        lbuf[1][i] = ' ';
    }
    lbuf[0][16] = '\0';
    lbuf[1][16] = '\0';
    lpos[0] = 0;
    lpos[1] = 0;
}

static void bufSetCol(uint8_t row, uint8_t col)
{
    if (col < 16) {
        lpos[row] = col;
    }
}

static void bufChar(uint8_t row, char c)
{
    if (lpos[row] < 16) {
        lbuf[row][lpos[row]++] = c;
    }
}

static void bufStr(uint8_t row, const char *s)
{
    while (*s && lpos[row] < 16) {
        lbuf[row][lpos[row]++] = *s++;
    }
}

static void bufNum(uint8_t row, uint16_t val, uint8_t digits)
{
    char tmp[6];
    int i;

    for (i = digits - 1; i >= 0; i--) {
        tmp[i] = '0' + (val % 10);
        val /= 10;
    }
    for (i = 0; i < digits; i++) {
        bufChar(row, tmp[i]);
    }
}

static void bufTime(uint8_t row, uint32_t totalSec)
{
    uint8_t hh = (uint8_t)((totalSec / 3600) % 100);
    uint8_t mm = (uint8_t)((totalSec / 60) % 60);
    uint8_t ss = (uint8_t)(totalSec % 60);

    bufNum(row, hh, 2);
    bufChar(row, ':');
    bufNum(row, mm, 2);
    bufChar(row, ':');
    bufNum(row, ss, 2);
}

static void bufFlush(void)
{
    LCD_SetCursor(0, 0);
    LCD_String(lbuf[0]);
    LCD_SetCursor(1, 0);
    LCD_String(lbuf[1]);
}

void DISP_ShowMenu(state_t st, uint8_t lang)
{
    if (lang > LANG_EN) lang = LANG_PT;

    bufInit();

    switch (st) {
        case ST_MENU_CONFIG:
            bufStr(0, ">[");
            bufStr(0, str_config[lang]);
            bufStr(0, "]");

            bufSetCol(1, 1);
            bufStr(1, str_adc[lang]);
            bufStr(1, "   ");
            bufStr(1, str_control[lang]);
            break;

        case ST_MENU_ADC:
            bufSetCol(0, 1);
            bufStr(0, str_config[lang]);

            bufStr(1, ">[");
            bufStr(1, str_adc[lang]);
            bufStr(1, "] ");
            bufStr(1, "CTRL");
            break;

        case ST_MENU_CONTROL:
            bufSetCol(0, 1);
            bufStr(0, str_config[lang]);
            bufStr(0, "   ");
            bufStr(0, str_adc[lang]);

            bufStr(1, ">[");
            bufStr(1, str_control[lang]);
            bufStr(1, "]");
            break;

        default:
            break;
    }

    bufFlush();
}

void DISP_ShowSubConfig(state_t st, uint8_t lang)
{
    if (lang > LANG_EN) lang = LANG_PT;

    bufInit();

    bufStr(0, str_cfgtitle[lang]);

    switch (st) {
        case ST_CFG_CLOCK:
            bufStr(1, str_clock[lang]);
            break;
        case ST_CFG_LIMITS:
            bufStr(1, str_limits[lang]);
            break;
        case ST_CFG_LANG:
            bufStr(1, str_langopt[lang]);
            bufStr(1, (lang == LANG_PT) ? "PT" : "EN");
            break;
        case ST_CFG_BACK:
            bufStr(1, (lang == LANG_PT) ? "> Voltar" : "> Back");
            break;
        default:
            break;
    }

    bufFlush();
}

void DISP_ShowADC(uint16_t adcVal, uint8_t lang)
{
    if (lang > LANG_EN) lang = LANG_PT;

    bufInit();

    bufStr(0, "ADC: ");
    bufNum(0, adcVal, 4);

    bufStr(1, "Hi:");
    bufNum(1, VAR_GetAlarmHi(), 4);
    bufStr(1, " Lo:");
    bufNum(1, VAR_GetAlarmLo(), 4);

    bufFlush();
}

void DISP_ShowEditLimits(uint16_t hi, uint16_t lo, uint8_t cursor, uint8_t lang)
{
    (void)lang;

    bufInit();

    bufChar(0, (cursor == 0) ? '>' : ' ');
    bufStr(0, "Hi:");
    bufNum(0, hi, 4);
    bufStr(0, " ");
    bufChar(0, (cursor == 1) ? '>' : ' ');
    bufStr(0, "Lo:");
    bufNum(0, lo, 4);

    bufStr(1, "Enc=Ajust SW=OK");

    bufFlush();
}

void DISP_ShowEditLang(uint8_t lang)
{
    bufInit();

    bufStr(0, str_langtitle[0]);

    bufStr(1, "> ");
    if (lang == LANG_PT) {
        bufStr(1, "[PT] EN");
    } else {
        bufStr(1, "PT [EN]");
    }

    bufFlush();
}

void DISP_ShowClock(uint32_t elapsedSec, uint8_t cursor, uint8_t lang)
{
    if (lang > LANG_EN) lang = LANG_PT;

    bufInit();

    bufStr(0, str_clocktit[lang]);

    bufSetCol(1, 0);
    bufChar(1, (cursor == 0) ? '>' : ' ');
    bufTime(1, elapsedSec);
    
    if (cursor == 1) {
        bufStr(1, (lang == LANG_PT) ? " >Voltar" : " >Back");
    } else {
        bufStr(1, (lang == LANG_PT) ? "  Voltar" : "  Back");
    }

    bufFlush();
}

void DISP_ShowControl(uint8_t alarmOn, uint8_t cursor, uint8_t lang)
{
    if (lang > LANG_EN) lang = LANG_PT;

    bufInit();

    bufStr(0, str_ctrltit[lang]);

    bufSetCol(1, 0);
    bufChar(1, (cursor == 0) ? '>' : ' ');
    if (alarmOn) {
        bufStr(1, (lang == LANG_PT) ? "LIGADO" : "ON    ");
    } else {
        bufStr(1, (lang == LANG_PT) ? "DESLIG" : "OFF   ");
    }
    
    if (cursor == 1) {
        bufStr(1, (lang == LANG_PT) ? " >Voltar" : " >Back");
    } else {
        bufStr(1, (lang == LANG_PT) ? "  Voltar" : "  Back");
    }

    bufFlush();
}

void DISP_ShowAlarmAlert(uint16_t adcVal, uint8_t hiOrLo, uint8_t lang)
{
    if (lang > LANG_EN) lang = LANG_PT;

    bufInit();

    bufStr(0, str_alert[lang]);

    bufStr(1, "ADC:");
    bufNum(1, adcVal, 4);

    if (hiOrLo == 1) {
        bufStr(1, " >Hi");
        bufNum(1, VAR_GetAlarmHi(), 4);
    } else {
        bufStr(1, " <Lo");
        bufNum(1, VAR_GetAlarmLo(), 4);
    }

    bufFlush();
}

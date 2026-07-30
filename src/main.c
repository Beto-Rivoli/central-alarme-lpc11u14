/*
 * main.c - Central de Alarme (LPC11U14)
 */

#include "LPC11Uxx.h"
#include "system_LPC11Uxx.h"

#include "inputs.h"
#include "lcd.h"
#include "eeprom_24lc512.h"
#include "adc.h"
#include "pwm.h"
#include "serial.h"
#include "io.h"

#include "stateMachine.h"

#include "var.h"
#include "alarm.h"
#include "display.h"
#include "serialCmd.h"
#include "bod.h"

#define SYSTICK_RELOAD          (SystemCoreClock / 100)  /* 10ms */
#define DISPLAY_REFRESH_TICKS   20                       /* 200ms */

static volatile uint8_t tick_flag = 0;
static volatile uint8_t display_tick = 0;
static uint8_t alarm_manual_off = 0;

void SysTick_Handler(void)
{
    tick_flag = 1;
    inputs_tick();
    ESM_TickChrono();
    display_tick++;
}

int main(void)
{
    event_t evt;
    output_t action;
    uint16_t current_adc;
    uint8_t alarm_status;

    /* Hardware init */
    SystemInit();
    init_inputs();
    LCD_Init();
    I2C_EEPROM_Init();
    adcInit();
    pwmInit();
    serialInit();
    serialSendString("\r\n--- Central de Alarme Inicializada ---\r\n");

    /* Module init */
    VAR_Init();
    ALARM_Init();
    SCMD_Init();
    ESM_Init();

    /* Self-test EEPROM screen */
    {
        uint8_t test_val = 0x00;
        EEPROM_WriteByte(0x0010, 0xA5);
        EEPROM_ReadByte(0x0010, &test_val);

        LCD_SetCursor(0, 0);
        if (test_val == 0xA5) {
            LCD_String("Alarme V2       ");
            LCD_SetCursor(1, 0);
            LCD_String("EEPROM: OK      ");
        } else {
            LCD_String("Alarme V2       ");
            LCD_SetCursor(1, 0);
            LCD_String("EEPROM: ERRO!   ");
        }

        for (volatile uint32_t w = 0; w < 1500000; w++);
    }

    SysTick_Config(SYSTICK_RELOAD);

    current_adc = adcRead();
    ESM_UpdateADC(current_adc);
    ESM_UpdateDisplay();

    /* Main loop */
    while (1)
    {
        if (!tick_flag) {
            __WFI();
            continue;
        }
        tick_flag = 0;

        current_adc = adcRead();
        ESM_UpdateADC(current_adc);

        /* Alarm check */
        static uint8_t alarm_strike_counter = 0;
        static uint16_t rearm_counter = 0;
        alarm_status = ALARM_Check(current_adc);

        if (VAR_IsArmed() && alarm_status != ALARM_OK && !ALARM_IsActive() && !alarm_manual_off) {
            alarm_strike_counter++;
            if (alarm_strike_counter >= 5) {
                alarm_strike_counter = 0;
                ALARM_Activate();

                alarmLog_t log;
                log.timestamp = ESM_GetChronoSeconds();
                log.adcValue  = current_adc;
                log.alarmType = alarm_status;
                log.reserved  = 0;
                VAR_LogEvent(log);
                serialSendString("[DEBUG] Gravando na EEPROM...\r\n");
                VAR_Save();
                eeprom_alarm_write_count++;

                action = ESM_ProcessEvent(EVT_ALARM_TRIGGER);
                if (action == OUT_LCD_UPDATE) {
                    ESM_UpdateDisplay();
                }
            }
            rearm_counter = 0;
        }
        else {
            alarm_strike_counter = 0;
            
            if (alarm_status == ALARM_OK && alarm_manual_off) {
                rearm_counter++;
                if (rearm_counter >= 200) {
                    alarm_manual_off = 0;
                    rearm_counter = 0;
                }
            } else {
                rearm_counter = 0;
            }
        }

        /* Input events */
        evt = inputs_getEvent();
        if (evt != EVT_NONE) {
            action = ESM_ProcessEvent(evt);

            switch (action) {
                case OUT_LCD_UPDATE:
                    ESM_UpdateDisplay();
                    break;

                case OUT_ALARM_ON:
                    ALARM_Activate();
                    alarm_manual_off = 0;
                    ESM_UpdateDisplay();
                    break;

                case OUT_ALARM_OFF:
                    ALARM_Deactivate();
                    alarm_manual_off = 1;
                    ESM_UpdateDisplay();
                    break;

                case OUT_SAVE_VARS:
                    VAR_Save();
                    ESM_UpdateDisplay();
                    break;

                case OUT_SEND_STATUS:
                    SCMD_SendStatus(current_adc, ALARM_IsActive(),
                                    VAR_GetAlarmHi(), VAR_GetAlarmLo(),
                                    VAR_GetLanguage());
                    break;

                default:
                    break;
            }
        }

        /* Serial events */
        evt = SCMD_Poll();
        if (evt != EVT_NONE) {
            action = ESM_ProcessEvent(evt);

            switch (action) {
                case OUT_LCD_UPDATE:
                    ESM_UpdateDisplay();
                    break;

                case OUT_SAVE_VARS:
                    VAR_Save();
                    ESM_UpdateDisplay();
                    break;

                case OUT_SEND_STATUS:
                    SCMD_SendStatus(current_adc, ALARM_IsActive(),
                                    VAR_GetAlarmHi(), VAR_GetAlarmLo(),
                                    VAR_GetLanguage());
                    break;

                default:
                    break;
            }
        }

        /* Display refresh */
        if (display_tick >= DISPLAY_REFRESH_TICKS) {
            display_tick = 0;

            state_t st = ESM_GetState();
            if (st == ST_SUB_ADC_VIEW ||
                st == ST_SUB_CTRL_VIEW ||
                st == ST_EDIT_CLOCK)
            {
                action = ESM_ProcessEvent(EVT_TICK);
                if (action == OUT_LCD_UPDATE) {
                    ESM_UpdateDisplay();
                }
            }
        }
    }

    return 0;
}

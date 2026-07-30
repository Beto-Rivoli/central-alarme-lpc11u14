/*
 * stateMachine.h - Máquina de estados (Kernel ESM)
 */

#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <stdint.h>

#define NAV_STACK_SIZE  8

typedef struct {
    uint8_t screen_id;
    uint8_t cursor_pos;
} NavState_t;

typedef enum {
    ST_INIT = 0,
    ST_MENU_CONFIG,
    ST_MENU_ADC,
    ST_MENU_CONTROL,
    ST_CFG_CLOCK,
    ST_CFG_LIMITS,
    ST_CFG_LANG,
    ST_CFG_BACK,
    ST_EDIT_CLOCK,
    ST_EDIT_LIMITS,
    ST_EDIT_LANG,
    ST_SUB_ADC_VIEW,
    ST_SUB_CTRL_VIEW,
    STATE_COUNT
} state_t;

typedef enum {
    EVT_NONE = 0,
    EVT_ENC_CW,
    EVT_ENC_CCW,
    EVT_ENC_SW,
    EVT_KEY1,
    EVT_KEY2,
    EVT_KEY3,
    EVT_SERIAL_FWD,
    EVT_SERIAL_STATUS,
    EVT_SERIAL_SET_HI,
    EVT_SERIAL_SET_LO,
    EVT_SERIAL_SET_LANG,
    EVT_SERIAL_SET_ARM,
    EVT_TICK,
    EVT_ALARM_TRIGGER,
    EVT_ALARM_CLEAR,
    EVENT_COUNT
} event_t;

typedef enum {
    OUT_NONE = 0,
    OUT_LCD_UPDATE,
    OUT_ALARM_ON,
    OUT_ALARM_OFF,
    OUT_SAVE_VARS,
    OUT_SEND_STATUS,
    OUTPUT_COUNT
} output_t;

typedef struct {
    state_t  nextState;
    output_t action;
} transition_t;

typedef transition_t (*StateHandler)(event_t evt);

extern volatile uint16_t serial_param_value;

void ESM_Init(void);
output_t ESM_ProcessEvent(event_t evt);
state_t ESM_GetState(void);
void ESM_UpdateADC(uint16_t adcVal);
uint16_t ESM_GetCachedADC(void);
void ESM_TickChrono(void);
uint32_t ESM_GetChronoSeconds(void);
void ESM_UpdateDisplay(void);

#endif /* STATE_MACHINE_H */

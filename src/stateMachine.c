/*
 * stateMachine.c - Máquina de estados (Kernel ESM)
 */

#include "stateMachine.h"
#include "var.h"
#include "alarm.h"
#include "display.h"
#include "serialCmd.h"
#include "adc.h"

static state_t currentState = ST_INIT;

static uint16_t edit_hi = 0;
static uint16_t edit_lo = 0;
static uint8_t  edit_cursor = 0;
static uint8_t edit_lang = 0;

static volatile uint32_t chrono_ticks = 0;
static uint8_t chrono_running = 0;
static uint16_t cached_adc = 0;

/* Navigation stack */
static NavState_t nav_stack[NAV_STACK_SIZE];
static uint8_t    nav_sp = 0;

static void nav_push(state_t screen, uint8_t cursor)
{
    if (nav_sp < NAV_STACK_SIZE) {
        nav_stack[nav_sp].screen_id  = (uint8_t)screen;
        nav_stack[nav_sp].cursor_pos = cursor;
        nav_sp++;
    }
}

static uint8_t nav_pop(state_t *out_screen, uint8_t *out_cursor)
{
    if (nav_sp == 0) {
        return 0;
    }
    nav_sp--;
    *out_screen = (state_t)nav_stack[nav_sp].screen_id;
    *out_cursor = nav_stack[nav_sp].cursor_pos;
    return 1;
}

/* Handlers forward declarations */
static transition_t handle_Init(event_t evt);
static transition_t handle_MenuConfig(event_t evt);
static transition_t handle_MenuADC(event_t evt);
static transition_t handle_MenuControl(event_t evt);
static transition_t handle_CfgClock(event_t evt);
static transition_t handle_CfgLimits(event_t evt);
static transition_t handle_CfgLang(event_t evt);
static transition_t handle_CfgBack(event_t evt);
static transition_t handle_EditClock(event_t evt);
static transition_t handle_EditLimits(event_t evt);
static transition_t handle_EditLang(event_t evt);
static transition_t handle_SubADCView(event_t evt);
static transition_t handle_SubCtrlView(event_t evt);

static const StateHandler stateTable[STATE_COUNT] = {
    [ST_INIT]          = handle_Init,
    [ST_MENU_CONFIG]   = handle_MenuConfig,
    [ST_MENU_ADC]      = handle_MenuADC,
    [ST_MENU_CONTROL]  = handle_MenuControl,
    [ST_CFG_CLOCK]     = handle_CfgClock,
    [ST_CFG_LIMITS]    = handle_CfgLimits,
    [ST_CFG_LANG]      = handle_CfgLang,
    [ST_CFG_BACK]      = handle_CfgBack,
    [ST_EDIT_CLOCK]    = handle_EditClock,
    [ST_EDIT_LIMITS]   = handle_EditLimits,
    [ST_EDIT_LANG]     = handle_EditLang,
    [ST_SUB_ADC_VIEW]  = handle_SubADCView,
    [ST_SUB_CTRL_VIEW] = handle_SubCtrlView,
};

#define TRANSITION(next, act)   ((transition_t){ (next), (act) })
#define NO_TRANSITION()         ((transition_t){ currentState, OUT_NONE })

static transition_t do_back(void)
{
    state_t prev_screen;
    uint8_t prev_cursor;

    if (nav_pop(&prev_screen, &prev_cursor)) {
        edit_cursor = prev_cursor;
        return TRANSITION(prev_screen, OUT_LCD_UPDATE);
    }
    
    if (currentState != ST_MENU_CONFIG && currentState != ST_MENU_ADC && currentState != ST_MENU_CONTROL) {
        return TRANSITION(ST_MENU_CONFIG, OUT_LCD_UPDATE);
    }

    return NO_TRANSITION();
}

static transition_t handle_serial_common(event_t evt)
{
    switch (evt) {
        case EVT_SERIAL_STATUS:
            return TRANSITION(currentState, OUT_SEND_STATUS);

        case EVT_SERIAL_SET_HI:
            VAR_SetAlarmHi(serial_param_value);
            return TRANSITION(currentState, OUT_SAVE_VARS);

        case EVT_SERIAL_SET_LO:
            VAR_SetAlarmLo(serial_param_value);
            return TRANSITION(currentState, OUT_SAVE_VARS);

        case EVT_SERIAL_SET_LANG:
            VAR_SetLanguage((uint8_t)serial_param_value);
            return TRANSITION(currentState, OUT_SAVE_VARS);

        case EVT_SERIAL_SET_ARM:
            VAR_SetArmed((uint8_t)serial_param_value);
            return TRANSITION(currentState, OUT_SAVE_VARS);

        default:
            return NO_TRANSITION();
    }
}

void ESM_Init(void)
{
    uint8_t saved = VAR_GetLastState();

    if (saved > 0 && saved < STATE_COUNT) {
        currentState = (state_t)saved;
    } else {
        currentState = ST_MENU_CONFIG;
    }

    edit_hi = VAR_GetAlarmHi();
    edit_lo = VAR_GetAlarmLo();
    edit_cursor = 0;
    edit_lang = VAR_GetLanguage();
    chrono_ticks = 0;
    chrono_running = 0;
    cached_adc = 0;

    nav_sp = 0;
}

output_t ESM_ProcessEvent(event_t evt)
{
    if (evt == EVT_NONE) {
        return OUT_NONE;
    }

    if (evt == EVT_SERIAL_STATUS) {
        return OUT_SEND_STATUS;
    }
    if (evt == EVT_SERIAL_SET_HI) {
        VAR_SetAlarmHi(serial_param_value);
        return OUT_SAVE_VARS;
    }
    if (evt == EVT_SERIAL_SET_LO) {
        VAR_SetAlarmLo(serial_param_value);
        return OUT_SAVE_VARS;
    }
    if (evt == EVT_SERIAL_SET_LANG) {
        VAR_SetLanguage((uint8_t)serial_param_value);
        return OUT_SAVE_VARS;
    }
    if (evt == EVT_SERIAL_SET_ARM) {
        VAR_SetArmed((uint8_t)serial_param_value);
        return OUT_SAVE_VARS;
    }

    if (currentState >= STATE_COUNT || stateTable[currentState] == 0) {
        currentState = ST_MENU_CONFIG;
        return OUT_LCD_UPDATE;
    }

    transition_t trans = stateTable[currentState](evt);

    if (trans.nextState != currentState) {
        currentState = trans.nextState;
        VAR_SetLastState((uint8_t)currentState);
    }

    return trans.action;
}

state_t ESM_GetState(void)
{
    return currentState;
}

void ESM_UpdateADC(uint16_t adcVal)
{
    cached_adc = adcVal;
}

uint16_t ESM_GetCachedADC(void)
{
    return cached_adc;
}

void ESM_TickChrono(void)
{
    if (chrono_running) {
        chrono_ticks++;
    }
}

uint32_t ESM_GetChronoSeconds(void)
{
    return chrono_ticks / 100;
}

void ESM_UpdateDisplay(void)
{
    uint8_t lang = VAR_GetLanguage();

    switch (currentState) {
        case ST_MENU_CONFIG:
        case ST_MENU_ADC:
        case ST_MENU_CONTROL:
            DISP_ShowMenu(currentState, lang);
            break;

        case ST_CFG_CLOCK:
        case ST_CFG_LIMITS:
        case ST_CFG_LANG:
        case ST_CFG_BACK:
            DISP_ShowSubConfig(currentState, lang);
            break;

        case ST_EDIT_CLOCK:
            DISP_ShowClock(ESM_GetChronoSeconds(), edit_cursor, lang);
            break;

        case ST_EDIT_LIMITS:
            DISP_ShowEditLimits(edit_hi, edit_lo, edit_cursor, lang);
            break;

        case ST_EDIT_LANG:
            DISP_ShowEditLang(edit_lang);
            break;

        case ST_SUB_ADC_VIEW:
            if (ALARM_IsActive()) {
                if (cached_adc > VAR_GetAlarmHi()) {
                    DISP_ShowAlarmAlert(cached_adc, 1, lang);
                } else if (cached_adc < VAR_GetAlarmLo()) {
                    DISP_ShowAlarmAlert(cached_adc, 2, lang);
                } else {
                    DISP_ShowADC(cached_adc, lang);
                }
            } else {
                DISP_ShowADC(cached_adc, lang);
            }
            break;

        case ST_SUB_CTRL_VIEW:
            DISP_ShowControl(ALARM_IsActive(), edit_cursor, lang);
            break;

        default:
            break;
    }
}

/* Handlers de estado */

static transition_t handle_Init(event_t evt)
{
    (void)evt;
    nav_sp = 0;
    return TRANSITION(ST_MENU_CONFIG, OUT_LCD_UPDATE);
}

static transition_t handle_MenuConfig(event_t evt)
{
    switch (evt) {
        case EVT_ENC_CW:
        case EVT_KEY3:
        case EVT_SERIAL_FWD:
            return TRANSITION(ST_MENU_ADC, OUT_LCD_UPDATE);

        case EVT_ENC_CCW:
        case EVT_KEY2:
            return TRANSITION(ST_MENU_CONTROL, OUT_LCD_UPDATE);

        case EVT_ENC_SW:
            nav_push(ST_MENU_CONFIG, 0);
            return TRANSITION(ST_CFG_CLOCK, OUT_LCD_UPDATE);

        case EVT_KEY1:
            return do_back();

        default:
            return handle_serial_common(evt);
    }
}

static transition_t handle_MenuADC(event_t evt)
{
    switch (evt) {
        case EVT_ENC_CW:
        case EVT_KEY3:
        case EVT_SERIAL_FWD:
            return TRANSITION(ST_MENU_CONTROL, OUT_LCD_UPDATE);

        case EVT_ENC_CCW:
        case EVT_KEY2:
            return TRANSITION(ST_MENU_CONFIG, OUT_LCD_UPDATE);

        case EVT_ENC_SW:
            nav_push(ST_MENU_ADC, 0);
            return TRANSITION(ST_SUB_ADC_VIEW, OUT_LCD_UPDATE);

        case EVT_KEY1:
            return do_back();

        default:
            return handle_serial_common(evt);
    }
}

static transition_t handle_MenuControl(event_t evt)
{
    switch (evt) {
        case EVT_ENC_CW:
        case EVT_KEY3:
        case EVT_SERIAL_FWD:
            return TRANSITION(ST_MENU_CONFIG, OUT_LCD_UPDATE);

        case EVT_ENC_CCW:
        case EVT_KEY2:
            return TRANSITION(ST_MENU_ADC, OUT_LCD_UPDATE);

        case EVT_ENC_SW:
            edit_cursor = 0;
            nav_push(ST_MENU_CONTROL, 0);
            return TRANSITION(ST_SUB_CTRL_VIEW, OUT_LCD_UPDATE);

        case EVT_KEY1:
            return do_back();

        default:
            return handle_serial_common(evt);
    }
}

static transition_t handle_CfgClock(event_t evt)
{
    switch (evt) {
        case EVT_ENC_CW:
        case EVT_KEY3:
            return TRANSITION(ST_CFG_LIMITS, OUT_LCD_UPDATE);

        case EVT_ENC_CCW:
        case EVT_KEY2:
            return TRANSITION(ST_CFG_BACK, OUT_LCD_UPDATE);

        case EVT_ENC_SW:
            nav_push(ST_CFG_CLOCK, 0);
            return TRANSITION(ST_EDIT_CLOCK, OUT_LCD_UPDATE);

        case EVT_KEY1:
            return do_back();

        default:
            return NO_TRANSITION();
    }
}

static transition_t handle_CfgLimits(event_t evt)
{
    switch (evt) {
        case EVT_ENC_CW:
        case EVT_KEY3:
            return TRANSITION(ST_CFG_LANG, OUT_LCD_UPDATE);

        case EVT_ENC_CCW:
        case EVT_KEY2:
            return TRANSITION(ST_CFG_CLOCK, OUT_LCD_UPDATE);

        case EVT_ENC_SW:
            edit_hi = VAR_GetAlarmHi();
            edit_lo = VAR_GetAlarmLo();
            edit_cursor = 0;
            nav_push(ST_CFG_LIMITS, 0);
            return TRANSITION(ST_EDIT_LIMITS, OUT_LCD_UPDATE);

        case EVT_KEY1:
            return do_back();

        default:
            return NO_TRANSITION();
    }
}

static transition_t handle_CfgLang(event_t evt)
{
    switch (evt) {
        case EVT_ENC_CW:
        case EVT_KEY3:
            return TRANSITION(ST_CFG_BACK, OUT_LCD_UPDATE);

        case EVT_ENC_CCW:
        case EVT_KEY2:
            return TRANSITION(ST_CFG_LIMITS, OUT_LCD_UPDATE);

        case EVT_ENC_SW:
            edit_lang = VAR_GetLanguage();
            nav_push(ST_CFG_LANG, 0);
            return TRANSITION(ST_EDIT_LANG, OUT_LCD_UPDATE);

        case EVT_KEY1:
            return do_back();

        default:
            return NO_TRANSITION();
    }
}

static transition_t handle_CfgBack(event_t evt)
{
    switch (evt) {
        case EVT_ENC_CW:
        case EVT_KEY3:
            return TRANSITION(ST_CFG_CLOCK, OUT_LCD_UPDATE);

        case EVT_ENC_CCW:
        case EVT_KEY2:
            return TRANSITION(ST_CFG_LANG, OUT_LCD_UPDATE);

        case EVT_ENC_SW:
        case EVT_KEY1:
            return do_back();

        default:
            return NO_TRANSITION();
    }
}

static transition_t handle_EditClock(event_t evt)
{
    switch (evt) {
        case EVT_ENC_SW:
            if (edit_cursor == 1) {
                return do_back();
            } else {
                chrono_running = !chrono_running;
                return TRANSITION(currentState, OUT_LCD_UPDATE);
            }

        case EVT_ENC_CW:
        case EVT_KEY3:
            edit_cursor = 1;
            return TRANSITION(currentState, OUT_LCD_UPDATE);
            
        case EVT_ENC_CCW:
        case EVT_KEY2:
            edit_cursor = 0;
            return TRANSITION(currentState, OUT_LCD_UPDATE);

        case EVT_KEY1:
            return do_back();

        case EVT_TICK:
            if (chrono_running) {
                return TRANSITION(currentState, OUT_LCD_UPDATE);
            }
            return NO_TRANSITION();

        default:
            return NO_TRANSITION();
    }
}

static transition_t handle_EditLimits(event_t evt)
{
    switch (evt) {
        case EVT_ENC_CW:
        case EVT_KEY2:
            if (edit_cursor == 0) {
                if (edit_hi < 1023) edit_hi += 10;
                if (edit_hi > 1023) edit_hi = 1023;
            } else {
                if (edit_lo < 1023) edit_lo += 10;
                if (edit_lo > 1023) edit_lo = 1023;
            }
            return TRANSITION(currentState, OUT_LCD_UPDATE);

        case EVT_ENC_CCW:
        case EVT_KEY3:
            if (edit_cursor == 0) {
                if (edit_hi >= 10) edit_hi -= 10;
                else edit_hi = 0;
            } else {
                if (edit_lo >= 10) edit_lo -= 10;
                else edit_lo = 0;
            }
            return TRANSITION(currentState, OUT_LCD_UPDATE);

        case EVT_ENC_SW:
            if (edit_cursor == 0) {
                edit_cursor = 1;
                return TRANSITION(currentState, OUT_LCD_UPDATE);
            } else {
                VAR_SetAlarmHi(edit_hi);
                VAR_SetAlarmLo(edit_lo);
                edit_cursor = 0;
                return TRANSITION(ST_CFG_LIMITS, OUT_SAVE_VARS);
            }

        case EVT_KEY1:
            edit_cursor = 0;
            return do_back();

        default:
            return NO_TRANSITION();
    }
}

static transition_t handle_EditLang(event_t evt)
{
    switch (evt) {
        case EVT_ENC_CW:
        case EVT_ENC_CCW:
        case EVT_KEY2:
        case EVT_KEY3:
            edit_lang = (edit_lang == LANG_PT) ? LANG_EN : LANG_PT;
            return TRANSITION(currentState, OUT_LCD_UPDATE);

        case EVT_ENC_SW:
            VAR_SetLanguage(edit_lang);
            return TRANSITION(ST_CFG_LANG, OUT_SAVE_VARS);

        case EVT_KEY1:
            return do_back();

        default:
            return NO_TRANSITION();
    }
}

static transition_t handle_SubADCView(event_t evt)
{
    switch (evt) {
        case EVT_KEY1:
        case EVT_ENC_SW:
            return do_back();

        case EVT_TICK:
            {
                static uint8_t tick_div = 0;
                tick_div++;
                if (tick_div >= 20) {
                    tick_div = 0;
                    return TRANSITION(currentState, OUT_LCD_UPDATE);
                }
                return NO_TRANSITION();
            }

        case EVT_ALARM_TRIGGER:
            return TRANSITION(currentState, OUT_ALARM_ON);

        case EVT_ALARM_CLEAR:
            return NO_TRANSITION();

        case EVT_SERIAL_STATUS:
            return TRANSITION(currentState, OUT_SEND_STATUS);

        default:
            return NO_TRANSITION();
    }
}

static transition_t handle_SubCtrlView(event_t evt)
{
    switch (evt) {
        case EVT_KEY1:
            return do_back();

        case EVT_ENC_CW:
        case EVT_KEY3:
            edit_cursor = 1;
            return TRANSITION(currentState, OUT_LCD_UPDATE);
            
        case EVT_ENC_CCW:
        case EVT_KEY2:
            edit_cursor = 0;
            return TRANSITION(currentState, OUT_LCD_UPDATE);

        case EVT_ENC_SW:
            if (edit_cursor == 1) {
                return do_back();
            } else {
                if (ALARM_IsActive()) {
                    return TRANSITION(currentState, OUT_ALARM_OFF);
                } else {
                    return TRANSITION(currentState, OUT_ALARM_ON);
                }
            }

        case EVT_TICK:
            {
                static uint8_t tick_div = 0;
                tick_div++;
                if (tick_div >= 20) {
                    tick_div = 0;
                    return TRANSITION(currentState, OUT_LCD_UPDATE);
                }
                return NO_TRANSITION();
            }

        case EVT_ALARM_TRIGGER:
            return TRANSITION(currentState, OUT_ALARM_ON);

        case EVT_ALARM_CLEAR:
            return NO_TRANSITION();

        case EVT_SERIAL_STATUS:
            return TRANSITION(currentState, OUT_SEND_STATUS);

        default:
            return NO_TRANSITION();
    }
}

/*
 * inputs.c - Leitura de teclado e encoder rotativo
 */

#include "LPC11Uxx.h"
#include "inputs.h"

#define IOCON_KEY_CFG      (0x00 | (1 << 3))   /* 0x08 */
#define IOCON_ENC_CFG      (0x00 | (2 << 3) | (1 << 5))   /* 0x30 */

static const int8_t enc_table[16] = {
     0, -1, +1,  0,
    +1,  0,  0, -1,
    -1,  0,  0, +1,
     0, +1, -1,  0
};

static uint8_t enc_prev_state = 0;
static int8_t  enc_accum = 0;
#define ENC_STEPS_PER_DETENT  2

#define DEBOUNCE_COUNT      5
#define DEBOUNCE_ENC_COUNT  2

static uint8_t key1_deb   = 0;
static uint8_t key2_deb   = 0;
static uint8_t key3_deb   = 0;
static uint8_t enc_sw_deb = 0;

static volatile uint8_t flag_key1   = 0;
static volatile uint8_t flag_key2   = 0;
static volatile uint8_t flag_key3   = 0;
static volatile uint8_t flag_enc_sw = 0;
static volatile int8_t  flag_enc_dir = 0;

void init_inputs(void)
{
    LPC_SYSCON->SYSAHBCLKCTRL |= (1 << 6) | (1 << 16);

    LPC_IOCON->PIO1_25 = IOCON_KEY_CFG;
    LPC_IOCON->PIO1_26 = IOCON_KEY_CFG;
    LPC_IOCON->PIO1_27 = IOCON_KEY_CFG;

    LPC_IOCON->PIO1_13 = IOCON_ENC_CFG;
    LPC_IOCON->PIO1_14 = IOCON_ENC_CFG;
    LPC_IOCON->PIO1_15 = IOCON_ENC_CFG;

    LPC_GPIO->DIR[1] &= ~( (1 << KEY1_PIN)   |
                           (1 << KEY2_PIN)   |
                           (1 << KEY3_PIN)   |
                           (1 << ENC_A_PIN)  |
                           (1 << ENC_B_PIN)  |
                           (1 << ENC_SW_PIN) );

    {
        uint8_t a = (LPC_GPIO->PIN[1] >> ENC_A_PIN) & 1;
        uint8_t b = (LPC_GPIO->PIN[1] >> ENC_B_PIN) & 1;
        enc_prev_state = (uint8_t)((a << 1) | b);
        enc_accum = 0;
    }
}

int key1_pressed(void)
{
    return (LPC_GPIO->PIN[1] & (1 << KEY1_PIN)) ? 1 : 0;
}

int key2_pressed(void)
{
    return (LPC_GPIO->PIN[1] & (1 << KEY2_PIN)) ? 1 : 0;
}

int key3_pressed(void)
{
    return (LPC_GPIO->PIN[1] & (1 << KEY3_PIN)) ? 1 : 0;
}

int enc_button_pressed(void)
{
    return (LPC_GPIO->PIN[1] & (1 << ENC_SW_PIN)) ? 0 : 1;
}

void inputs_tick(void)
{
    if (key1_pressed()) {
        if (key1_deb < DEBOUNCE_COUNT) {
            key1_deb++;
            if (key1_deb == DEBOUNCE_COUNT) flag_key1 = 1;
        }
    } else {
        key1_deb = 0;
    }

    if (key2_pressed()) {
        if (key2_deb < DEBOUNCE_COUNT) {
            key2_deb++;
            if (key2_deb == DEBOUNCE_COUNT) flag_key2 = 1;
        }
    } else {
        key2_deb = 0;
    }

    if (key3_pressed()) {
        if (key3_deb < DEBOUNCE_COUNT) {
            key3_deb++;
            if (key3_deb == DEBOUNCE_COUNT) flag_key3 = 1;
        }
    } else {
        key3_deb = 0;
    }

    if (enc_button_pressed()) {
        if (enc_sw_deb < DEBOUNCE_ENC_COUNT) {
            enc_sw_deb++;
            if (enc_sw_deb == DEBOUNCE_ENC_COUNT) flag_enc_sw = 1;
        }
    } else {
        enc_sw_deb = 0;
    }

    {
        uint8_t a = (LPC_GPIO->PIN[1] >> ENC_A_PIN) & 1;
        uint8_t b = (LPC_GPIO->PIN[1] >> ENC_B_PIN) & 1;
        uint8_t cur_state = (uint8_t)((a << 1) | b);

        if (cur_state != enc_prev_state) {
            int8_t delta = enc_table[(enc_prev_state << 2) | cur_state];
            enc_accum += delta;
            enc_prev_state = cur_state;

            if (enc_accum >= ENC_STEPS_PER_DETENT) {
                flag_enc_dir = +1;
                enc_accum -= ENC_STEPS_PER_DETENT;
            } else if (enc_accum <= -ENC_STEPS_PER_DETENT) {
                flag_enc_dir = -1;
                enc_accum += ENC_STEPS_PER_DETENT;
            }
        }
    }
}

event_t inputs_getEvent(void)
{
    if (flag_enc_sw) {
        flag_enc_sw = 0;
        flag_enc_dir = 0;
        enc_accum = 0;
        return EVT_ENC_SW;
    }

    int8_t dir = flag_enc_dir;
    if (dir != 0) {
        flag_enc_dir = 0;
        if (dir > 0) return EVT_ENC_CW;
        else         return EVT_ENC_CCW;
    }

    if (flag_key1) {
        flag_key1 = 0;
        return EVT_KEY1;
    }

    if (flag_key2) {
        flag_key2 = 0;
        return EVT_KEY2;
    }

    if (flag_key3) {
        flag_key3 = 0;
        return EVT_KEY3;
    }

    return EVT_NONE;
}

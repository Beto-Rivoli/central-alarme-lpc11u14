/*
 * inputs.h - Driver para teclado e encoder rotativo
 */

#ifndef INPUTS_H
#define INPUTS_H

#include <stdint.h>
#include "stateMachine.h"

#define KEY1_PIN    25
#define KEY2_PIN    26
#define KEY3_PIN    27
#define KEY_PORT    1

#define ENC_A_PIN   13
#define ENC_B_PIN   14
#define ENC_SW_PIN  15
#define ENC_PORT    1

void init_inputs(void);
int key1_pressed(void);
int key2_pressed(void);
int key3_pressed(void);
int enc_button_pressed(void);

void inputs_tick(void);
event_t inputs_getEvent(void);

#endif /* INPUTS_H */

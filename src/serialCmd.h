/*
 * serialCmd.h - Parser de comandos seriais
 */

#ifndef SERIALCMD_H
#define SERIALCMD_H

#include "stateMachine.h"

typedef enum {
    PACKET_NONE = 0,
    PACKET_OK,
    PACKET_MALFORMED_NO_START,
    PACKET_MALFORMED_NO_END,
    PACKET_MALFORMED_INVALID_DATA,
    PACKET_MALFORMED_BUFFER_OVERFLOW
} packet_status_t;

extern volatile uint16_t serial_param_value;
extern volatile uint32_t eeprom_alarm_write_count;
extern volatile uint8_t last_packet_status;

void SCMD_Init(void);
event_t SCMD_Poll(void);
void SCMD_SendStatus(uint16_t adcVal, uint8_t alarmOn,
                     uint16_t hi, uint16_t lo, uint8_t lang);

#endif /* SERIALCMD_H */

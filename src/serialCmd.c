/*
 * serialCmd.c - Parser de comandos seriais
 */

#include "serialCmd.h"
#include "serial.h"
#include "var.h"
#include "mcp7940.h"
#include "i2c.h"

#define RX_BUF_SIZE     32

static char rx_buf[RX_BUF_SIZE];
static uint8_t rx_idx = 0;

volatile uint16_t serial_param_value = 0;
volatile uint32_t eeprom_alarm_write_count = 0;
volatile uint8_t last_packet_status = PACKET_NONE;

static uint8_t is_receiving = 0;

static char toUpper(char c)
{
    if (c >= 'a' && c <= 'z') {
        return c - 'a' + 'A';
    }
    return c;
}

static uint16_t parseDecimal(const char *str, uint8_t len)
{
    uint16_t val = 0;
    uint8_t i;
    for (i = 0; i < len; i++) {
        if (str[i] >= '0' && str[i] <= '9') {
            val = val * 10 + (str[i] - '0');
        } else {
            break;
        }
    }
    return val;
}

static void formatDecimal(uint16_t val, char *buf)
{
    buf[0] = '0' + (val / 1000) % 10;
    buf[1] = '0' + (val / 100) % 10;
    buf[2] = '0' + (val / 10) % 10;
    buf[3] = '0' + val % 10;
    buf[4] = '\0';
}

static event_t parseCommand(void)
{
    if (rx_idx < 1) {
        last_packet_status = PACKET_MALFORMED_INVALID_DATA;
        goto error;
    }

    /* Comando L: Armar/Desarmar (L1 / L0) */
    if (rx_idx == 2 && toUpper(rx_buf[0]) == 'L') {
        if (rx_buf[1] == '1') {
            serial_param_value = 1;
            last_packet_status = PACKET_OK;
            serialSendString("[DEBUG] Sistema Armado (L1). Salvo na EEPROM.\r\n");
            return EVT_SERIAL_SET_ARM;
        } else if (rx_buf[1] == '0') {
            serial_param_value = 0;
            last_packet_status = PACKET_OK;
            serialSendString("[DEBUG] Sistema Desarmado (L0). Salvo na EEPROM.\r\n");
            return EVT_SERIAL_SET_ARM;
        }
        last_packet_status = PACKET_MALFORMED_INVALID_DATA;
        goto error;
    }

    /* Comando VAA: Limite Superior */
    if (rx_idx == 7 && toUpper(rx_buf[0]) == 'V' && toUpper(rx_buf[1]) == 'A' && toUpper(rx_buf[2]) == 'A') {
        serial_param_value = parseDecimal(&rx_buf[3], 4);
        if (serial_param_value > 1023) serial_param_value = 1023;
        last_packet_status = PACKET_OK;
        serialSendString("[DEBUG] Limite Superior (VAA) alterado. Salvo na EEPROM.\r\n");
        return EVT_SERIAL_SET_HI;
    }

    /* Comando VAB: Limite Inferior */
    if (rx_idx == 7 && toUpper(rx_buf[0]) == 'V' && toUpper(rx_buf[1]) == 'A' && toUpper(rx_buf[2]) == 'B') {
        serial_param_value = parseDecimal(&rx_buf[3], 4);
        if (serial_param_value > 1023) serial_param_value = 1023;
        last_packet_status = PACKET_OK;
        serialSendString("[DEBUG] Limite Inferior (VAB) alterado. Salvo na EEPROM.\r\n");
        return EVT_SERIAL_SET_LO;
    }

    /* Comando IDI: Idioma (IDIPT / IDIEN) */
    if (rx_idx == 5 && toUpper(rx_buf[0]) == 'I' && toUpper(rx_buf[1]) == 'D' && toUpper(rx_buf[2]) == 'I') {
        if ((toUpper(rx_buf[3]) == 'P') && (toUpper(rx_buf[4]) == 'T')) {
            serial_param_value = 0;
            last_packet_status = PACKET_OK;
            serialSendString("[DEBUG] Idioma (IDI) alterado para PT. Salvo na EEPROM.\r\n");
            return EVT_SERIAL_SET_LANG;
        } else if ((toUpper(rx_buf[3]) == 'E') && (toUpper(rx_buf[4]) == 'N')) {
            serial_param_value = 1;
            last_packet_status = PACKET_OK;
            serialSendString("[DEBUG] Idioma (IDI) alterado para EN. Salvo na EEPROM.\r\n");
            return EVT_SERIAL_SET_LANG;
        }
        last_packet_status = PACKET_MALFORMED_INVALID_DATA;
        goto error;
    }

    /* Comando TEM: Temperatura */
    if (rx_idx == 7 && toUpper(rx_buf[0]) == 'T' && toUpper(rx_buf[1]) == 'E' && toUpper(rx_buf[2]) == 'M') {
        uint16_t temp_val = parseDecimal(&rx_buf[3], 4);
        last_packet_status = PACKET_OK;
        serialSendString("[DEBUG] Temperatura (Tem) configurada para ");
        char tempStr[5];
        formatDecimal(temp_val, tempStr);
        serialSendString(tempStr);
        serialSendString(".\r\n");
        return EVT_NONE;
    }

    /* Comando HOR: Hora */
    if (rx_idx == 7 && toUpper(rx_buf[0]) == 'H' && toUpper(rx_buf[1]) == 'O' && toUpper(rx_buf[2]) == 'R') {
        uint16_t hour_val = parseDecimal(&rx_buf[3], 2);
        uint16_t min_val = parseDecimal(&rx_buf[5], 2);
        if (hour_val < 24 && min_val < 60) {
            mcpSetHours((unsigned char)hour_val);
            mcpSetMinutes((unsigned char)min_val);
            last_packet_status = PACKET_OK;
            serialSendString("[DEBUG] Hora (Hor) configurada para ");
            char hrStr[3], mnStr[3];
            hrStr[0] = '0' + (hour_val / 10);
            hrStr[1] = '0' + (hour_val % 10);
            hrStr[2] = '\0';
            mnStr[0] = '0' + (min_val / 10);
            mnStr[1] = '0' + (min_val % 10);
            mnStr[2] = '\0';
            serialSendString(hrStr);
            serialSendString(":");
            serialSendString(mnStr);
            serialSendString(".\r\n");
        } else {
            last_packet_status = PACKET_MALFORMED_INVALID_DATA;
            serialSendString("[DEBUG] Hora (Hor) invalida.\r\n");
        }
        return EVT_NONE;
    }

    /* Comando STATUS ou TEST */
    if ((rx_idx == 6 && toUpper(rx_buf[0]) == 'S' && toUpper(rx_buf[1]) == 'T' && toUpper(rx_buf[2]) == 'A' && 
         toUpper(rx_buf[3]) == 'T' && toUpper(rx_buf[4]) == 'U' && toUpper(rx_buf[5]) == 'S') ||
        (rx_idx == 4 && toUpper(rx_buf[0]) == 'T' && toUpper(rx_buf[1]) == 'E' && toUpper(rx_buf[2]) == 'S' && toUpper(rx_buf[3]) == 'T')) {
        last_packet_status = PACKET_OK;
        return EVT_SERIAL_STATUS;
    }

    last_packet_status = PACKET_MALFORMED_INVALID_DATA;

error:
    serialSendString("[DEBUG] Pacote rejeitado ou malformado.\r\n");
    return EVT_NONE;
}

void SCMD_Init(void)
{
    rx_idx = 0;
    is_receiving = 0;
    serial_param_value = 0;
    eeprom_alarm_write_count = 0;
    last_packet_status = PACKET_NONE;
}

event_t SCMD_Poll(void)
{
    int ch = serialReadChar();

    while (ch != -1) {
        if (ch == '<') {
            if (is_receiving) {
                last_packet_status = PACKET_MALFORMED_NO_END;
            }
            is_receiving = 1;
            rx_idx = 0;
        } else if (ch == '>') {
            if (is_receiving) {
                is_receiving = 0;
                if (rx_idx > 0) {
                    event_t evt = parseCommand();
                    rx_idx = 0;
                    return evt;
                } else {
                    last_packet_status = PACKET_MALFORMED_INVALID_DATA;
                    serialSendString("[DEBUG] Pacote vazio.\r\n");
                }
            } else {
                last_packet_status = PACKET_MALFORMED_NO_START;
                serialSendString("[DEBUG] Pacote rejeitado ou malformado (sem < inicial).\r\n");
            }
        } else {
            if (is_receiving) {
                if (rx_idx < RX_BUF_SIZE - 1) {
                    rx_buf[rx_idx++] = (char)ch;
                } else {
                    is_receiving = 0;
                    rx_idx = 0;
                    last_packet_status = PACKET_MALFORMED_BUFFER_OVERFLOW;
                    serialSendString("[DEBUG] Pacote rejeitado (estourou buffer).\r\n");
                }
            }
        }

        ch = serialReadChar();
    }

    return EVT_NONE;
}

void SCMD_SendStatus(uint16_t adcVal, uint8_t alarmOn,
                     uint16_t hi, uint16_t lo, uint8_t lang)
{
    char numBuf[5];

    serialSendString("--- STATUS DE VALIDACAO ---\r\n");

    serialSendString("ADC: ");
    formatDecimal(adcVal, numBuf);
    serialSendString(numBuf);
    serialSendString("\r\n");

    serialSendString("Alarm: ");
    serialSendString(alarmOn ? "ON" : "OFF");
    serialSendString("\r\n");

    serialSendString("Hi (LAS): ");
    formatDecimal(hi, numBuf);
    serialSendString(numBuf);
    serialSendString("  Lo (LAI): ");
    formatDecimal(lo, numBuf);
    serialSendString(numBuf);
    serialSendString("\r\n");

    serialSendString("Lang: ");
    serialSendString(lang == 0 ? "PT" : "EN");
    serialSendString("\r\n");

    serialSendString("Armed: ");
    serialSendString(VAR_IsArmed() ? "YES" : "NO");
    serialSendString("\r\n");

    serialSendString("Last Packet: ");
    switch (last_packet_status) {
        case PACKET_NONE:                      serialSendString("NONE"); break;
        case PACKET_OK:                        serialSendString("OK"); break;
        case PACKET_MALFORMED_NO_START:        serialSendString("MALFORMED (No <)"); break;
        case PACKET_MALFORMED_NO_END:          serialSendString("MALFORMED (No >)"); break;
        case PACKET_MALFORMED_INVALID_DATA:    serialSendString("MALFORMED (Invalid content)"); break;
        case PACKET_MALFORMED_BUFFER_OVERFLOW: serialSendString("MALFORMED (Overflow)"); break;
        default:                               serialSendString("UNKNOWN"); break;
    }
    serialSendString("\r\n");

    serialSendString("EEPROM Alarm Writes: ");
    char countBuf[10];
    uint32_t count = eeprom_alarm_write_count;
    int idx = 0;
    if (count == 0) {
        countBuf[idx++] = '0';
    } else {
        char temp[10];
        int t_idx = 0;
        while (count > 0) {
            temp[t_idx++] = '0' + (count % 10);
            count /= 10;
        }
        while (t_idx > 0) {
            countBuf[idx++] = temp[--t_idx];
        }
    }
    countBuf[idx] = '\0';
    serialSendString(countBuf);
    serialSendString("\r\n");

    serialSendString("---------------------------\r\n");
}

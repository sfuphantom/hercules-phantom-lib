/*
 * phal_uart.h
 *
 *  Created on: Jan. 20, 2024
 *      Author: manoj
 *
 *
 */

#ifndef PHALD_UART_PHAL_UART_H_
#define PHALD_UART_PHAL_UART_H_

#include "sci.h"
#include "reg_sci.h"
#include "sys_vim.h"
#include "sys_core.h"
#include <assert.h>

typedef void (*uartRxCallback_t)(char);
typedef void (*uartTxCallback_t)(void);

typedef enum {
    PHAL_UART_SCI_PORT = (uint32)0,
    PHAL_UART_SCILIN_PORT = (uint32)1
} uartPort;

typedef enum  {
    PHAL_UART_WAIT = (uint32)0,
    PHAL_UART_FORCE = (uint32)1
} uartMode;

typedef enum {
    PHAL_UART_INTR_HIGH =  (uint32)0,
    PHAL_UART_INTR_LOW =  (uint32)1,
    PHAL_UART_INTR_DISABLE = (uint32)2
} uartIntrtLvl;

typedef struct {
    uint32 priority_high;
    uint32 priority_low;
    uartIntrtLvl rx_level;
    uartIntrtLvl tx_level;
} uart_intr_setup_t;

void phal_uart_init();
void phal_uart_set_baudrate(uartPort port, uint32 br);
void phal_uart_print(uartPort port, const char* str);
void phal_uart_println(uartPort port, const char* str);
void phal_uart_enable_interrupt(uartPort port, uart_intr_setup_t setup);
void phal_uart_disable_interrupt(uartPort port);
void phal_uart_attach_receive_cb(uartPort port, uartRxCallback_t cb);
void phal_uart_attach_transmit_cb(uartPort port, uartTxCallback_t cb);
void phal_uart_send(uartPort port, uint8* data, uint32 len, uartMode mode);
void phal_uart_receive(uartPort port, uint8* data, uint32 len, uartMode mode);
bool phal_uart_listen_for_transmit(uartPort port, uint8* data, uint32 len);
bool phal_uart_listen_for_receive(uartPort port, volatile uint8* data, uint32 len);

#endif /* PHALD_UART_PHAL_UART_H_ */

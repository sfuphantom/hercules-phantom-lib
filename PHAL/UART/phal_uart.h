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
} uart_port_id;

typedef enum  {
    PHAL_UART_WAIT = (uint32)0,
    PHAL_UART_FORCE = (uint32)1
} uart_mode;

typedef enum {
    PHAL_UART_INTR_HIGH =  (uint32)0,
    PHAL_UART_INTR_LOW =  (uint32)1,
    PHAL_UART_INTR_DISABLE = (uint32)2
} uart_intr_level;

typedef struct {
    uint32 priority_high;
    uint32 priority_low;
    uart_intr_level rx_level;
    uart_intr_level tx_level;
} uart_intr_setup;

void phal_uart_init();
void phal_uart_set_baudrate(uart_port_id port, uint32 br);
void phal_uart_print(uart_port_id port, const char* str);
void phal_uart_println(uart_port_id port, const char* str);
void phal_uart_enable_interrupt(uart_port_id port, uart_intr_setup setup);
void phal_uart_disable_interrupt(uart_port_id port);
void phal_uart_attach_receive_cb(uart_port_id port, uartRxCallback_t cb);
void phal_uart_attach_transmit_cb(uart_port_id port, uartTxCallback_t cb);
void phal_uart_send(uart_port_id port, uint8* data, uint32 len, uart_mode mode);
void phal_uart_receive(uart_port_id port, uint8* data, uint32 len, uart_mode mode);
bool phal_uart_async_send(uart_port_id port, uint8* data, uint32 len);
bool phal_uart_async_receive(uart_port_id port, volatile uint8* data, uint32 len);

#endif /* PHALD_UART_PHAL_UART_H_ */

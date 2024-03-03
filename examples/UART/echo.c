/*
 * echo.cpp
 *
 *  Created on: Jan. 26, 2024
 *      Author: manoj
 */



#include "phal_uart.h"

#define MYSERIAL_INTR_PRIORITY_L  126 /*use only 2-126 and make sure not to overlap*/
#define MYSERIAL_INTR_PRIORITY_H  125 /*use only 2-126 and make sure not to overlap*/

uartPort mySerial = PHAL_UART_SCI_PORT;
uart_intr_setup_t mySerialIntr = {.priority_high = MYSERIAL_INTR_PRIORITY_H,
                                .priority_low = MYSERIAL_INTR_PRIORITY_L,
                                .rx_level = PHAL_UART_INTR_HIGH,
                                .tx_level = PHAL_UART_INTR_LOW};


volatile uint8 message;

char* message1 = "A for Effort!\n\r";

void rxCallback(char data) {
        phal_uart_listen_for_transmit(mySerial, (uint8*)&message, 1);
}

void txCallback() {
        phal_uart_listen_for_receive(mySerial, (uint8*)&message, 1);
}
/**
 * main.c
 */
int main(void)
{
//    _enable_interrupt_();
    int i;
    phal_uart_init(mySerial);
    phal_uart_enable_interrupt(mySerial, mySerialIntr);
    phal_uart_attach_receive_cb(mySerial, &rxCallback);
    phal_uart_attach_transmit_cb(mySerial, &txCallback);
    phal_uart_println(mySerial, "This works??");
    phal_uart_listen_for_receive(mySerial, (uint8*)&message, 1);
    while(1) {
        phal_uart_send(mySerial, (uint8*)message1, 15, PHAL_UART_FORCE);
        for (i = 0; i < 100000000; i++);
    }
    return 0;
}




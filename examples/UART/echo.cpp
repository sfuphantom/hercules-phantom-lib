/*
 * echo.cpp
 *
 *  Created on: Jan. 26, 2024
 *      Author: manoj
 */



//#include "phal_uart.h"
#include "can.h"

#define MYSERIAL_INTR_PRIORITY_L  126 /*use only 2-126 and make sure not to overlap*/
#define MYSERIAL_INTR_PRIORITY_H  125 /*use only 2-126 and make sure not to overlap*/
//auto mySerial = PhalUart(uart_port::SCI);


volatile uint8 message = (char)0xFF;
volatile uint32 async_send_done = 0;
volatile uint32 async_recv_done = 0;
/**
 * main.c
 */
int main(void)
{
//    _enable_interrupt_();
//    mySerial.init(460800);
    canInit();

//    while(1) {
//    mySerial.print("Hello ");
//    mySerial.println("World!");
//    mySerial.attachInterrupt(MYSERIAL_INTR_PRIORITY_H, uart_intr_lvl::HIGH);
//    mySerial.attachInterrupt(MYSERIAL_INTR_PRIORITY_L, uart_intr_lvl::LOW);
//    mySerial.setInterruptLevel(uart_intr_typ::RX, uart_intr_lvl::HIGH);
//    mySerial.setInterruptLevel(uart_intr_typ::TX, uart_intr_lvl::LOW);
//
//    mySerial.asyncReceive(&message, 1, async_recv_done);
    while(1) {
        canTransmit(canREG1, 1, (const uint8 *) message);
    }
    return 0;
}
//
//void uartCallback(uart_port port, uart_intr_typ intr) {
//    if (mySerial.isReceiveInterrupt(port, intr)) {
//        if(async_recv_done) {
//            async_send_done = 0;
//            mySerial.asyncSend((uint8*)&message, 1, async_send_done);
//        }
//    }
//    else if (mySerial.isSendInterrupt(port, intr)) {
//        if(async_send_done) {
//            async_recv_done = 0;
//            mySerial.asyncReceive(&message, 1, async_recv_done);
//        }
//    }
//}



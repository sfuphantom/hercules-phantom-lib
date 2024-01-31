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


enum class uart_port : uint32 {
    SCI = (uint32)sciREG,
    LINSCI = (uint32)scilinREG
};

enum class uart_intr_typ : uint32  {
    RX =  0x00000200,
    TX =  0x00000100
};

enum class uart_intr_lvl : uint32 {
    HIGH = 0,
    LOW = 1
};

enum class uart_sync_mode : uint32 {
    WAIT = 0,
    FORCE = 1
};

//define this function if you want write custom isrHandler
#pragma WEAK
void uartCallback(uart_port base, uart_intr_typ intr);

class PhalUart {
private:
      sciBASE_t* base_reg;
      uint32 mode;
public:
    PhalUart(uart_port base);
//    uint32 getBase();
    void init(uint32 br);
    void setBaudrate(uint32 br);
    void print(const char* str);
    void println(const char* str);
    void attachInterrupt(uint32 priority, uart_intr_lvl level);
    void detachInterrupt(uint32 priority);
    void setInterruptLevel(uart_intr_typ type, uart_intr_lvl level);
    void send(uint8* data, uint32 len, uart_sync_mode mode);
    void receive(uint8* data, uint32 len, uart_sync_mode mode);
    bool asyncSend(uint8* data, uint32 len, volatile uint32& flag);
    bool asyncReceive(volatile uint8* data, uint32 len, volatile uint32& flag);
    bool isReceiveInterrupt(uart_port port, uart_intr_typ intr);
    bool isSendInterrupt(uart_port port, uart_intr_typ intr);
    void close();
private:
    /*dont want users to enable/disable interrupts instead use async send/receive */
    void enableInterrupt(uart_intr_typ type);
    void disableInterrupt(uart_intr_typ type);
};

#endif /* PHALD_UART_PHAL_UART_H_ */

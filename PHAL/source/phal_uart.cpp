/*
 * phal_uart.cpp
 *
 *  Created on: Jan. 20, 2024
 *      Author: manoj
 */

#include <phal_uart.h>

//extern volatile uint32 phal_uart_flag; //bit 0: SCI_RX, bit 1: SCI_TX, bit 2: SCILIN_RX, bit 3: SCILIN_T
typedef struct {
    volatile uint32 len;
    volatile uint8* data;
    volatile uint32* done;
    volatile bool busy;
} uartAsync;

enum phal_uart_intr_t {
    RECEIVE_INT   = 11U,
    TRANSMIT_INT  = 12U
};


const uint32 SCI_HIGH_LEVEL_INT_REQ = 64;
const uint32 SCI_LOW_LEVEL_INT_REQ = 74;
const uint32 LIN_HIGH_LEVEL_INT_REQ = 13;
const uint32 LIN_LOW_LEVEL_INT_REQ = 27;

static volatile uartAsync g_TX[2] = {0};
static volatile uartAsync g_RX[2] = {0};

void isrSciHigh(void);
void isrSciLow(void);
void isrLinHigh(void);
void isrLinLow(void);




PhalUart::PhalUart(uart_port base) {
    base_reg = (sciBASE_t*)(uint32)base;
}

void PhalUart::init(uint32 br) {
    //sciInit() reference
    /** - bring SCI out of reset */
    base_reg->GCR0 = 0U;
    base_reg->GCR0 = 1U;

    /** - Disable all interrupts */
    base_reg->CLEARINT    = 0xFFFFFFFFU;
    base_reg->CLEARINTLVL = 0xFFFFFFFFU;

    base_reg->GCR1 =(uint32)((uint32)1U << 25U)  /* enable transmit */
                  | (uint32)((uint32)1U << 24U)  /* enable receive */
                  | (uint32)((uint32)1U << 5U)   /* internal clock (device has no clock pin) */
                  | (uint32)((uint32)1U << 4U)  /* 2 stop bit */
                  | (uint32)((uint32)0U << 2U)  /* disable parity */
                  | (uint32)((uint32)1U << 1U);  /* asynchronous timing mode */

    sciSetBaudrate(base_reg, br); //set desired baudrate reuse hal driver function

    base_reg->FORMAT = 7U; // data size = 8 bits (set value + 1)

    base_reg->PIO0 = (uint32)((uint32)1U << 2U)  /* tx pin */
                 | (uint32)((uint32)1U << 1U); /* rx pin */

    /** - set SCI pins default output value */
    base_reg->PIO3 = (uint32)((uint32)0U << 2U)  /* tx pin */
                 | (uint32)((uint32)0U << 1U); /* rx pin */

    /** - set SCI pins output direction */
    base_reg->PIO1 = (uint32)((uint32)0U << 2U)  /* tx pin */
                 | (uint32)((uint32)0U << 1U); /* rx pin */

    /** - set SCI pins open drain enable */
    base_reg->PIO6 = (uint32)((uint32)0U << 2U)  /* tx pin */
                 | (uint32)((uint32)0U << 1U); /* rx pin */

    /** - set SCI pins pullup/pulldown enable */
    base_reg->PIO7 = (uint32)((uint32)0U << 2U)  /* tx pin */
                 | (uint32)((uint32)0U << 1U); /* rx pin */

    /** - set SCI pins pullup/pulldown select */
    base_reg->PIO8 = (uint32)((uint32)1U << 2U)  /* tx pin */
                 | (uint32)((uint32)1U << 1U);  /* rx pin */

    /** - Finaly start SCI */
    base_reg->GCR1 |= 0x80U;

//    g_TX.data = NULL;
    uint32 id = ((uint32)base_reg == (uint32)sciREG) ? 0 : 1;
    g_TX[id].len = 0;
    g_TX[id].busy = 0;
    g_RX[id].len = 0;
    g_RX[id].busy = 0;

    mode = 0;
}

void PhalUart::setBaudrate(uint32 br) {
    sciSetBaudrate(base_reg, br);
}

void PhalUart::print(const char* str) {
    for(uint32 i = 0; str[i] != '\0'; i++) {
        while ((base_reg->FLR & (uint32)SCI_TX_INT) == 0U);
        base_reg->TD = (uint32)str[i];
    }
}

void PhalUart::println(const char* str) {
    for(uint32 i = 0; str[i] != '\0'; i++) {
        while ((base_reg->FLR & (uint32)SCI_TX_INT) == 0U);
        base_reg->TD = (uint32)str[i];
    }
    print("\n\r");
}

void PhalUart::attachInterrupt(uint32 priority, uart_intr_lvl lvl)
{
    _enable_IRQ();
    if ((uint32)base_reg == (uint32)sciREG) {
        if (lvl == uart_intr_lvl::HIGH)
           vimChannelMap(SCI_HIGH_LEVEL_INT_REQ, priority, isrSciHigh);
        else
           vimChannelMap(SCI_LOW_LEVEL_INT_REQ, priority, isrSciLow);
    }
    else {
        if (lvl == uart_intr_lvl::HIGH)
           vimChannelMap(LIN_HIGH_LEVEL_INT_REQ, priority, isrLinHigh);
        else
           vimChannelMap(LIN_LOW_LEVEL_INT_REQ, priority, isrLinLow);
    }
    vimEnableInterrupt(priority, SYS_IRQ);
}

void PhalUart::detachInterrupt(uint32 priority) {
    vimDisableInterrupt(priority);
}

void PhalUart::setInterruptLevel(uart_intr_typ type, uart_intr_lvl level){
    mode |= (uint32)type;
    if (level == uart_intr_lvl::HIGH)
        base_reg->CLEARINTLVL = (uint32)type;
    else
        base_reg->SETINTLVL = (uint32)type;
}

void PhalUart::enableInterrupt(uart_intr_typ type) {
    base_reg->SETINT = (uint32)type;
}
void PhalUart::disableInterrupt(uart_intr_typ type) {
    base_reg->CLEARINT = (uint32)type;
}

void PhalUart::close() {
    base_reg->GCR1 &= ~(uint32)0x80U;
}

//uint32 PhalUart::getBase() {
//    return (uint32)base_reg;
//}

void PhalUart::send(uint8* data, uint32 len, uart_sync_mode mode) {
//    while(g_TX.busy); // will wait for previous async send to finish
    uint32 id = ((uint32)base_reg == (uint32)sciREG) ? 0 : 1;

    if (mode == uart_sync_mode::WAIT)
        while(g_TX[id].busy);

    if (g_TX[id].busy)
        disableInterrupt(uart_intr_typ::TX); //will pause any async send
    while((base_reg->FLR & (uint32)SCI_TX_INT) == 0U);
    for(uint32 l = 0; l < len; l++) {
        base_reg->TD = (uint32)data[l];
        while((base_reg->FLR & (uint32)SCI_TX_INT) == 0U);
    }
    if (g_TX[id].busy)
        enableInterrupt(uart_intr_typ::TX); //will resume async send
}

void PhalUart::recieve(uint8* data, uint32 len, uart_sync_mode mode) {
    uint32 id = ((uint32)base_reg == (uint32)sciREG) ? 0 : 1;
    if (mode == uart_sync_mode::WAIT)
        while(g_RX[id].busy);

    if (g_RX[id].busy)
        disableInterrupt(uart_intr_typ::RX); //will pause any async send

    for(uint32 l = 0; l < len; l++) {
        while((base_reg->FLR & (uint32)SCI_RX_INT) == 0U);
        data[l] = (uint8)(base_reg->RD & 0xFF);
    }

    if (g_RX[id].busy)
        enableInterrupt(uart_intr_typ::RX); //will pause any async send
}

bool PhalUart::asyncSend(uint8* data, uint32 len, volatile uint32 &flag) {
    uint32 id = ((uint32)base_reg == (uint32)sciREG) ? 0 : 1;
    if ((mode & (uint32)uart_intr_typ::TX ) == 0)
        return 1; //return 1 if interrupt not setup properly

    if (g_TX[id].busy)
        return 1; //if not finished previous async send

    if (len == 0)
        return 0; //return immediately if length is zero

    g_TX[id].data = data + 1;
    g_TX[id].len = len - 1;
    g_TX[id].done = &flag;
    g_TX[id].busy = 1;
    base_reg->TD = data[0];
    enableInterrupt(uart_intr_typ::TX); // enble TX
    return 0;
}

bool PhalUart::asyncRecieve(volatile uint8* data, uint32 len, volatile uint32 &flag) {
    uint32 id = ((uint32)base_reg == (uint32)sciREG) ? 0 : 1;
    if ((mode & (uint32)uart_intr_typ::RX ) == 0)
        return 1; //return 1 if interrupt not setup properly

    if (g_RX[id].busy)
        return 1; //if not finished previous async send

    if (len == 0)
        return 0; //return immediately if length is zero

    g_RX[id].data = data;
    g_RX[id].len = len;
    g_RX[id].done = &flag;
    g_RX[id].busy = 1;
    enableInterrupt(uart_intr_typ::RX); // enble RX Interrupt
    return 0;
}

bool PhalUart::isRecieveInterrupt(uart_port port, uart_intr_typ type) {
    return (((uint32)base_reg == (uint32)port) && ((uint32)type == (uint32)uart_intr_typ::RX));
}

bool PhalUart::isSendInterrupt(uart_port port, uart_intr_typ type) {
    return (((uint32)base_reg == (uint32)port) && ((uint32)type == (uint32)uart_intr_typ::TX));
}

#pragma CODE_STATE(32)
#pragma INTERRUPT(IRQ)
void isrSciHigh(void) {
    uint32 vec = sciREG->INTVECT0;
    switch(vec) {
    case RECEIVE_INT:
        if (g_RX[0].busy && (g_RX[0].len > 0)) {
            *g_RX[0].data = (uint8)(sciREG->RD & 0xFF);
            g_RX[0].data++;
            g_RX[0].len--;
            if (g_RX[0].len == 0U) {
                *g_RX[0].done = 1;
                g_RX[0].busy = 0;
                sciREG->CLEARINT = (uint32)SCI_RX_INT;
            }
        }
        uartCallback(uart_port::SCI, uart_intr_typ::RX);
        break;
    case TRANSMIT_INT:
        if (g_TX[0].len == 0U) {
            if (g_TX[0].busy) {
                *g_TX[0].done = 1;
                g_TX[0].busy = 0;
                sciREG->CLEARINT = (uint32)SCI_TX_INT;
            }
        }
        else {
            sciREG->TD = (uint32)(*g_TX[0].data) & 0xFF;
            g_TX[0].data++;
            g_TX[0].len--;
        }
        uartCallback(uart_port::SCI, uart_intr_typ::TX);
        break;
    default:
        sciREG->FLR = sciREG->SETINTLVL & 0x07000303U;
        break;
    }

}

#pragma CODE_STATE(32)
#pragma INTERRUPT(IRQ)

void isrSciLow(void) {
    uint32 vec = sciREG->INTVECT1;
    switch(vec) {
    case RECEIVE_INT:
        if (g_RX[0].busy && (g_RX[0].len > 0)) {
            *g_RX[0].data = (uint8)(sciREG->RD & 0xFF);
            g_RX[0].data++;
            g_RX[0].len--;
            if (g_RX[0].len == 0U) {
                *g_RX[0].done = 1;
                g_RX[0].busy = 0;
                sciREG->CLEARINT = (uint32)SCI_RX_INT;
            }
        }
        uartCallback(uart_port::SCI, uart_intr_typ::RX);
        break;
    case TRANSMIT_INT:
        if (g_TX[0].len == 0U) {
            if (g_TX[0].busy) {
                *g_TX[0].done = 1;
                g_TX[0].busy = 0;
                sciREG->CLEARINT = (uint32)SCI_TX_INT;
            }
        }
        else {
            sciREG->TD = (uint32)(*g_TX[0].data) & 0xFF;
            g_TX[0].data++;
            g_TX[0].len--;
        }
        uartCallback(uart_port::SCI, uart_intr_typ::TX);
        break;
    default:
        sciREG->FLR = sciREG->SETINTLVL & 0x07000303U;
        break;
    }
}

#pragma CODE_STATE(32)
#pragma INTERRUPT(IRQ)
void isrLinHigh(void) {
    uint32 vec = scilinREG->INTVECT0;
    switch(vec) {
    case RECEIVE_INT:
        if (g_RX[1].busy && (g_RX[1].len > 0)) {
            *g_RX[1].data = (uint8)(scilinREG->RD & 0xFF);
            g_RX[1].data++;
            g_RX[1].len--;
            if (g_RX[1].len == 0U) {
                *g_RX[1].done = 1;
                g_RX[1].busy = 0;
                scilinREG->CLEARINT = (uint32)SCI_RX_INT;
            }
        }
        uartCallback(uart_port::LINSCI, uart_intr_typ::RX);
        break;
    case TRANSMIT_INT:
        if (g_TX[1].len == 0U) {
            if (g_TX[1].busy) {
                *g_TX[1].done = 1;
                g_TX[1].busy = 0;
                scilinREG->CLEARINT = (uint32)SCI_TX_INT;
            }
        }
        else {
            scilinREG->TD = (uint32)(*g_TX[1].data) & 0xFF;
            g_TX[1].data++;
            g_TX[1].len--;
        }
        uartCallback(uart_port::LINSCI, uart_intr_typ::TX);
        break;
    default:
        scilinREG->FLR = scilinREG->SETINTLVL & 0x07000303U;
        break;
    }

}

#pragma CODE_STATE(32)
#pragma INTERRUPT(IRQ)
void isrLinLow(void) {
    uint32 vec = scilinREG->INTVECT1;
    switch(vec) {
    case RECEIVE_INT:
        if (g_RX[1].busy && (g_RX[1].len > 0)) {
            *g_RX[1].data = (uint8)(scilinREG->RD & 0xFF);
            g_RX[1].data++;
            g_RX[1].len--;
            if (g_RX[1].len == 0U) {
                *g_RX[1].done = 1;
                g_RX[1].busy = 0;
                scilinREG->CLEARINT = (uint32)SCI_RX_INT;
            }
        }
        uartCallback(uart_port::LINSCI, uart_intr_typ::RX);
        break;
    case TRANSMIT_INT:
        if (g_TX[1].len == 0U) {
            if (g_TX[1].busy) {
                *g_TX[1].done = 1;
                g_TX[1].busy = 0;
                scilinREG->CLEARINT = (uint32)SCI_TX_INT;
            }
        }
        else {
            scilinREG->TD = (uint32)(*g_TX[1].data) & 0xFF;
            g_TX[1].data++;
            g_TX[1].len--;
        }

        uartCallback(uart_port::LINSCI, uart_intr_typ::TX);
        break;
    default:
        scilinREG->FLR = scilinREG->SETINTLVL & 0x07000303U;
        break;
    }
}

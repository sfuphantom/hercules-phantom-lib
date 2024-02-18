/*
 * phal_uart.c
 *
 *  Created on: Jan. 20, 2024
 *      Author: manoj
 */

#include <phal_uart.h>

#define RECEIVE_INT 11U
#define TRANSMIT_INT 12U

#define SCI_HIGH_LEVEL_INT_REQ  64U
#define SCI_LOW_LEVEL_INT_REQ  74U
#define LIN_HIGH_LEVEL_INT_REQ  13U
#define LIN_LOW_LEVEL_INT_REQ  27U

void isrSciHigh(void);
void isrSciLow(void);
void isrLinHigh(void);
void isrLinLow(void);


typedef struct {
    volatile uint32 len;
    volatile uint8* data;
    volatile uint32* done;
    volatile bool busy;
} _uartAsync_t;


typedef struct {
    sciBASE_t* base_reg;
    uartRxCallback_t rx_callback;
    uartTxCallback_t tx_callback;
    _uartAsync_t g_TX;
    _uartAsync_t g_RX;
    uart_intr_setup intr_setup;
} _uartPort_t;

static _uartPort_t ports[2];

static void dummyRxCb(char ch) {
    return;
}

static void dummyTxCb(void) {
    return;
}

void phal_uart_init() {
    uint32 port;
    uint32 br = 9600; //default baudrate
    //do initialisation for both ports
    for (port = 0; port < 2; port++) {
        _uartPort_t* myPort = &(ports[port]);

        myPort->base_reg = (port == PHAL_UART_SCI_PORT) ? sciREG : scilinREG;
        myPort->rx_callback = dummyRxCb;
        myPort->tx_callback = dummyTxCb;
        myPort->g_TX.busy = 0;
        myPort->g_RX.busy = 0;
        myPort->g_TX.done = (uint32*)NULL;
        myPort->g_RX.done = (uint32*)NULL;
        myPort->g_TX.len = 0;
        myPort->g_RX.len = 0;
        myPort->g_TX.data = (uint8*)NULL;
        myPort->g_RX.data = (uint8*)NULL;

        myPort->intr_setup.priority_high = (port == PHAL_UART_SCI_PORT) ? SCI_HIGH_LEVEL_INT_REQ : LIN_HIGH_LEVEL_INT_REQ; //default priority doesn't matter, user has to specify for setup
        myPort->intr_setup.priority_low = (port == PHAL_UART_SCI_PORT) ? SCI_LOW_LEVEL_INT_REQ : LIN_LOW_LEVEL_INT_REQ;

        myPort->intr_setup.rx_level = PHAL_UART_INTR_DISABLE; //Interrupts are disabled by default
        myPort->intr_setup.tx_level = PHAL_UART_INTR_DISABLE;

        myPort->base_reg->GCR0 = 0U;
        myPort->base_reg->GCR0 = 1U;

        /** - Disable all interrupts */
        myPort->base_reg->CLEARINT    = 0xFFFFFFFFU;
        myPort->base_reg->CLEARINTLVL = 0xFFFFFFFFU;

        myPort->base_reg->GCR1 =(uint32)((uint32)1U << 25U)  /* enable transmit */
                      | (uint32)((uint32)1U << 24U)  /* enable receive */
                      | (uint32)((uint32)1U << 5U)   /* internal clock (device has no clock pin) */
                      | (uint32)((uint32)1U << 4U)  /* 2 stop bit */
                      | (uint32)((uint32)0U << 2U)  /* disable parity */
                      | (uint32)((uint32)1U << 1U);  /* asynchronous timing mode */

        sciSetBaudrate(myPort->base_reg, br); //set desired baudrate reuse hal driver function

        myPort->base_reg->FORMAT = 7U; // data size = 8 bits (set value + 1)

        myPort->base_reg->PIO0 = (uint32)((uint32)1U << 2U)  /* tx pin */
                     | (uint32)((uint32)1U << 1U); /* rx pin */

        /** - set SCI pins default output value */
        myPort->base_reg->PIO3 = (uint32)((uint32)0U << 2U)  /* tx pin */
                     | (uint32)((uint32)0U << 1U); /* rx pin */

        /** - set SCI pins output direction */
        myPort->base_reg->PIO1 = (uint32)((uint32)0U << 2U)  /* tx pin */
                     | (uint32)((uint32)0U << 1U); /* rx pin */

        /** - set SCI pins open drain enable */
        myPort->base_reg->PIO6 = (uint32)((uint32)0U << 2U)  /* tx pin */
                     | (uint32)((uint32)0U << 1U); /* rx pin */

        /** - set SCI pins pullup/pulldown enable */
        myPort->base_reg->PIO7 = (uint32)((uint32)0U << 2U)  /* tx pin */
                     | (uint32)((uint32)0U << 1U); /* rx pin */

        /** - set SCI pins pullup/pulldown select */
        myPort->base_reg->PIO8 = (uint32)((uint32)1U << 2U)  /* tx pin */
                     | (uint32)((uint32)1U << 1U);  /* rx pin */

        /** - Finaly start SCI */
        myPort->base_reg->GCR1 |= 0x80U;
    }
}

void phal_uart_set_baudrate(uart_port_id port, uint32 br) {
    sciSetBaudrate(ports[port].base_reg, br);
}

void phal_uart_print(uart_port_id port, const char* str) {
    uint32 i;
    for(i = 0; str[i] != '\0'; i++) {
        while ((ports[port].base_reg->FLR & (uint32)SCI_TX_INT) == 0U);
        ports[port].base_reg->TD = (uint32)str[i];
    }
}
////
//////println, prints the string followed by newline and return symbol
void phal_uart_println(uart_port_id port, const char* str) {
    phal_uart_print(port, str);
    phal_uart_print(port, "\n\r");
}

void phal_uart_enable_interrupt(uart_port_id port, uart_intr_setup setup) {
    //Receive Interrupt
    _enable_IRQ();
    ports[port].intr_setup = setup;
    if (setup.rx_level != PHAL_UART_INTR_DISABLE) {
        //High Level
        if(setup.rx_level == PHAL_UART_INTR_HIGH) {
            assert((setup.priority_high > 1) && (setup.priority_high < 128)); // 0 & 1 reserved
            ports[port].base_reg->CLEARINTLVL = (uint32)SCI_RX_INT;
            //Both Ports
            if(port == PHAL_UART_SCI_PORT)
                vimChannelMap(SCI_HIGH_LEVEL_INT_REQ, setup.priority_high, isrSciHigh);
            else
                vimChannelMap(LIN_HIGH_LEVEL_INT_REQ, setup.priority_high, isrLinHigh);
            vimEnableInterrupt(setup.priority_high, SYS_IRQ);
        }
        //Low Level
        else {
            assert((setup.priority_low > 1) && (setup.priority_low < 128)); // 0 & 1 reserved
            ports[port].base_reg->SETINTLVL = (uint32)SCI_RX_INT;
            //Both Ports
            if(port == PHAL_UART_SCI_PORT)
                vimChannelMap(SCI_LOW_LEVEL_INT_REQ, setup.priority_low, isrSciLow);
            else
                vimChannelMap(LIN_LOW_LEVEL_INT_REQ, setup.priority_low, isrLinLow);
            vimEnableInterrupt(setup.priority_low, SYS_IRQ);
        }
    }

    //Transmit Interrupt
    if (setup.tx_level != PHAL_UART_INTR_DISABLE) {
        //High Level
        if(setup.tx_level == PHAL_UART_INTR_HIGH) {
            assert((setup.priority_high > 1) && (setup.priority_high < 128)); // 0 & 1 reserved
            ports[port].base_reg->CLEARINTLVL = (uint32)SCI_TX_INT;
            //Both Ports
            if(port == PHAL_UART_SCI_PORT)
                vimChannelMap(SCI_HIGH_LEVEL_INT_REQ, setup.priority_high, isrSciHigh);
            else
                vimChannelMap(LIN_HIGH_LEVEL_INT_REQ, setup.priority_high, isrLinHigh);
            vimEnableInterrupt(setup.priority_high, SYS_IRQ);
        }
        //Low Level
        else {
            assert((setup.priority_low > 1) && (setup.priority_low < 128)); // 0 & 1 reserved
            ports[port].base_reg->SETINTLVL = (uint32)SCI_TX_INT;
            //Both Ports
            if(port == PHAL_UART_SCI_PORT)
                vimChannelMap(SCI_LOW_LEVEL_INT_REQ, setup.priority_low, isrSciLow);
            else
                vimChannelMap(LIN_LOW_LEVEL_INT_REQ, setup.priority_low, isrLinLow);
            vimEnableInterrupt(setup.priority_low, SYS_IRQ);
        }
    }
}

void phal_uart_disable_interrupt(uart_port_id port) {
    uart_intr_setup* setup = &ports[port].intr_setup;
    // if interrupt level is other than disable then it was enabled, disable all interrupt
    if ((setup->rx_level != PHAL_UART_INTR_DISABLE) || (setup->tx_level != PHAL_UART_INTR_DISABLE)) {
        //Low Level
        if ((setup->rx_level != PHAL_UART_INTR_LOW) || (setup->tx_level != PHAL_UART_INTR_LOW))
            vimDisableInterrupt(setup->priority_low);

        if ((setup->rx_level != PHAL_UART_INTR_HIGH) || (setup->tx_level != PHAL_UART_INTR_HIGH))
            vimDisableInterrupt(setup->priority_high);

        setup->rx_level = PHAL_UART_INTR_DISABLE;
        setup->tx_level = PHAL_UART_INTR_DISABLE;
    }
}

void phal_uart_attach_receive_cb(uart_port_id port, uartRxCallback_t cb) {
    assert(cb);
    ports[port].rx_callback = cb;
}

void phal_uart_attach_transmit_cb(uart_port_id port, uartTxCallback_t cb) {
    assert(cb);
    ports[port].tx_callback = cb;
}
//
//send byte array of len in two modes
//(1) WAIT mode will wait for previous asynchronous send if any
//(2) FORCE mode will pause previous asynchronous send if any
void phal_uart_send(uart_port_id port, uint8* data, uint32 len, uart_mode mode) {
//    while(g_TX.busy); // will wait for previous async send to finish
    if (mode == PHAL_UART_WAIT)
        while(ports[port].g_TX.busy);

    if (ports[port].g_TX.busy)
        ports[port].base_reg->CLEARINT = (uint32)SCI_TX_INT; //will pause any async send

    while((ports[port].base_reg->FLR & (uint32)SCI_TX_INT) == 0U);
    uint32 l;
    for(l = 0; l < len; l++) {
        ports[port].base_reg->TD = (uint32)data[l];
        while((ports[port].base_reg->FLR & (uint32)SCI_TX_INT) == 0U);
    }

    if (ports[port].g_TX.busy)
            ports[port].base_reg->SETINT = (uint32)SCI_TX_INT;
}
//
////receive byte array of len in two modes
////(1) WAIT mode will wait for previous asynchronous receive if any
////(2) FORCE mode will pause previous asynchronous receive if any
void phal_uart_receive(uart_port_id port, uint8* data, uint32 len, uart_mode mode) {
    if (mode == PHAL_UART_WAIT)
        while(ports[port].g_RX.busy);

    if (ports[port].g_RX.busy)
        ports[port].base_reg->CLEARINT = (uint32)SCI_RX_INT; //will pause any async send

    uint32 l;
    for(l = 0; l < len; l++) {
        while((ports[port].base_reg->FLR & (uint32)SCI_RX_INT) == 0U);
        data[l] = (uint8)(ports[port].base_reg->RD & 0xFF);
    }

    if (ports[port].g_RX.busy)
        ports[port].base_reg->SETINT = (uint32)SCI_RX_INT; //will pause any async send
}

//
////send byte array of len asynchronously, the flag will be set once the send is done, make sure to clear it before sending
bool phal_uart_async_send(uart_port_id port, uint8* data, uint32 len, volatile uint32* flag) {
    if (ports[port].intr_setup.tx_level == PHAL_UART_INTR_DISABLE)
        return 1; //return 1 if interrupt not setup properly

    if (ports[port].g_TX.busy)
        return 1; //if not finished previous async send

    if (len == 0)
        return 0; //return immediately if length is zero

    ports[port].g_TX.data = data + 1;
    ports[port].g_TX.len = len - 1;
    ports[port].g_TX.done = flag;
    *ports[port].g_TX.done = 0; //clear the flag
    ports[port].g_TX.busy = 1;
    ports[port].base_reg->TD = data[0];
    ports[port].base_reg->SETINT = (uint32)SCI_TX_INT;// enble TX
    return 0;
}

//receive byte array of len asynchronously, the flag will be set once the receive is done, make sure to clear it before receiving
bool phal_uart_async_receive(uart_port_id port, volatile uint8* data, uint32 len, volatile uint32* flag) {
    if (ports[port].intr_setup.rx_level == PHAL_UART_INTR_DISABLE)
        return 1; //return 1 if interrupt not setup properly

    if (ports[port].g_RX.busy)
        return 1; //if not finished previous async send

    if (len == 0)
        return 0; //return immediately if length is zero

    ports[port].g_RX.data = data;
    ports[port].g_RX.len = len;
    ports[port].g_RX.done = flag;
    *ports[port].g_RX.done = 0; //clear flag
    ports[port].g_RX.busy = 1;
    ports[port].base_reg->SETINT = (uint32)SCI_RX_INT;// enble TX
    return 0;
}

#pragma CODE_STATE(isrSciHigh, 32)
#pragma INTERRUPT(isrSciHigh, IRQ)
void isrSciHigh(void) {
    uint32 vec = sciREG->INTVECT0;
    switch(vec) {
    case RECEIVE_INT:
        if (ports[0].g_RX.busy && (ports[0].g_RX.len > 0)) {
            *ports[0].g_RX.data = (uint8)(sciREG->RD & 0xFF);
            ports[0].g_RX.data++;
            ports[0].g_RX.len--;
            if (ports[0].g_RX.len == 0U) {
                *ports[0].g_RX.done = 1;
                ports[0].g_RX.busy = 0;
                sciREG->CLEARINT = (uint32)SCI_RX_INT;
            }
        }
        ports[0].rx_callback((char)(sciREG->RD & 0xFF));
        break;
    case TRANSMIT_INT:
        if (ports[0].g_TX.len == 0U) {
            if (ports[0].g_TX.busy) {
                *ports[0].g_TX.done = 1;
                ports[0].g_TX.busy = 0;
                sciREG->CLEARINT = (uint32)SCI_TX_INT;
            }
        }
        else {
            sciREG->TD = (uint32)(*ports[0].g_TX.data) & 0xFF;
            ports[0].g_TX.data++;
            ports[0].g_TX.len--;
        }
        ports[0].tx_callback();
        break;
    default:
        sciREG->FLR = sciREG->SETINTLVL & 0x07000303U;
        break;
    }

}

#pragma CODE_STATE(isrSciLow, 32)
#pragma INTERRUPT(isrSciLow, IRQ)

void isrSciLow(void) {
    uint32 vec = sciREG->INTVECT1;
    switch(vec) {
    case RECEIVE_INT:
        if (ports[0].g_RX.busy && (ports[0].g_RX.len > 0)) {
            *ports[0].g_RX.data = (uint8)(sciREG->RD & 0xFF);
            ports[0].g_RX.data++;
            ports[0].g_RX.len--;
            if (ports[0].g_RX.len == 0U) {
                *ports[0].g_RX.done = 1;
                ports[0].g_RX.busy = 0;
                sciREG->CLEARINT = (uint32)SCI_RX_INT;
            }
        }
        ports[0].rx_callback((char)(sciREG->RD & 0xFF));
        break;
    case TRANSMIT_INT:
        if (ports[0].g_TX.len == 0U) {
            if (ports[0].g_TX.busy) {
                *ports[0].g_TX.done = 1;
                ports[0].g_TX.busy = 0;
                sciREG->CLEARINT = (uint32)SCI_TX_INT;
            }
        }
        else {
            sciREG->TD = (uint32)(*ports[0].g_TX.data) & 0xFF;
            ports[0].g_TX.data++;
            ports[0].g_TX.len--;
        }
        ports[0].tx_callback();
        break;
    default:
        sciREG->FLR = sciREG->SETINTLVL & 0x07000303U;
        break;
    }
}

#pragma CODE_STATE(isrLinHigh, 32)
#pragma INTERRUPT(isrLinHigh, IRQ)
void isrLinHigh(void) {
    uint32 vec = scilinREG->INTVECT0;
    switch(vec) {
    case RECEIVE_INT:
        if (ports[1].g_RX.busy && (ports[1].g_RX.len > 0)) {
            *ports[1].g_RX.data = (uint8)(scilinREG->RD & 0xFF);
            ports[1].g_RX.data++;
            ports[1].g_RX.len--;
            if (ports[1].g_RX.len == 0U) {
                *ports[1].g_RX.done = 1;
                ports[1].g_RX.busy = 0;
                scilinREG->CLEARINT = (uint32)SCI_RX_INT;
            }
        }
        ports[1].rx_callback((char)(scilinREG->RD & 0xFF));
        break;
    case TRANSMIT_INT:
        if (ports[1].g_TX.len == 0U) {
            if (ports[1].g_TX.busy) {
                *ports[1].g_TX.done = 1;
                ports[1].g_TX.busy = 0;
                scilinREG->CLEARINT = (uint32)SCI_TX_INT;
            }
        }
        else {
            scilinREG->TD = (uint32)(*ports[1].g_TX.data) & 0xFF;
            ports[1].g_TX.data++;
            ports[1].g_TX.len--;
        }
        ports[1].tx_callback();
        break;
    default:
        scilinREG->FLR = scilinREG->SETINTLVL & 0x07000303U;
        break;
    }

}

#pragma CODE_STATE(isrLinLow, 32)
#pragma INTERRUPT(isrLinLow, IRQ)
void isrLinLow(void) {
    uint32 vec = scilinREG->INTVECT1;
    switch(vec) {
    case RECEIVE_INT:
        if (ports[1].g_RX.busy && (ports[1].g_RX.len > 0)) {
            *ports[1].g_RX.data = (uint8)(scilinREG->RD & 0xFF);
            ports[1].g_RX.data++;
            ports[1].g_RX.len--;
            if (ports[1].g_RX.len == 0U) {
                *ports[1].g_RX.done = 1;
                ports[1].g_RX.busy = 0;
                scilinREG->CLEARINT = (uint32)SCI_RX_INT;
            }
        }
        ports[1].rx_callback((char)(scilinREG->RD & 0xFF));
        break;
    case TRANSMIT_INT:
        if (ports[1].g_TX.len == 0U) {
            if (ports[1].g_TX.busy) {
                *ports[1].g_TX.done = 1;
                ports[1].g_TX.busy = 0;
                scilinREG->CLEARINT = (uint32)SCI_TX_INT;
            }
        }
        else {
            scilinREG->TD = (uint32)(*ports[1].g_TX.data) & 0xFF;
            ports[1].g_TX.data++;
            ports[1].g_TX.len--;
        }
        ports[1].tx_callback();
        break;
    default:
        scilinREG->FLR = scilinREG->SETINTLVL & 0x07000303U;
        break;
    }
}

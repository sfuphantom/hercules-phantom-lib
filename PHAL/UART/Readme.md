### Code Description
```
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
```
#### Enumerations:

1. **uartPort**: Specifies UART ports, where
   - `PHAL_UART_SCI_PORT` represents SCI port
   - `PHAL_UART_SCILIN_PORT` represents LIN SCI port

2. **uartMode**: Defines UART modes
*Note:* This is used to define the behavior of `phal_uart_send`/`phal_uart_receive` when used along with `phal_uart_listen_for_transmit`/`phal_uart_listen_for_receive`.
   - `PHAL_UART_WAIT`: will make `phal_uart_send`/`phal_uart_receive` wait for the `phal_uart_listen_for_transmit`/`phal_uart_listen_for_receive` to finish (if any active).
   - `PHAL_UART_FORCE` will pause `phal_uart_listen_for_transmit`/`phal_uart_listen_for_receive`(if any active) and resume once `phal_uart_send`/`phal_uart_receive` is done.


4. **uartIntrtLvl**: Represents UART interrupt levels, where
  - `PHAL_UART_INTR_HIGH` denotes high priority
  - `PHAL_UART_INTR_LOW` denotes low priority
  - `PHAL_UART_INTR_DISABLE` denotes disabled interrupts

#### Structure:

- **uart_intr_setup_t**: This structure encapsulates UART interrupt setup parameters,
  - `priority_high` and `priority_low` should be unique priorities between 2-127.
  - `rx_level` and `tx_level` sets the rx and tx interrupts to different priority levels (high, low, or disabled).  

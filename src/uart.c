/* UART serial functions
 * www.ece.tufts.edu/ee/14
 * Steven Bell <sbell@ece.tufts.edu>, based almost entirely on work from Joel Grodstein
 * March 2026
 */
#include "ee14lib.h"

#define TX_buffer_len 100

volatile int write_pos = 0;
volatile int read_pos = 0;
volatile char TX_buffer [TX_buffer_len];

void USART2_IRQHandler(void){
    if(read_pos == write_pos){ // if no more data to write
        USART2->CR1 &= ~(1 << 7); //disable the interrupt
    }
    else{
        // read the byte from the circular buffer
        USART2->TDR = TX_buffer[read_pos];
        read_pos = (read_pos + 1) % TX_buffer_len;
    }
}

/**
 * Configure either USART with the "normal" defaults:
 * Set for 8 data bits, 1 start & 1 stop bit, 16x oversampling
 * And by default, we also get no parity, no hardware flow control (USART_CR3),
 * asynch mode (USART_CR2).
 *
 * :param USARTx: Pointer to the CMSIS USART struct, either USART1 or USART2
 * :param tx_en: True to enable TX channel
 * :param rx_en: True to enable RX channel
 * :param baud: Baud rate to use
 */
static void USART_Init (USART_TypeDef *USARTx, bool tx_en, bool rx_en, int baud) {
    // Disable the USART.
    USARTx->CR1 &= ~USART_CR1_UE;

    // The "M" field is two bits, M1 and M0. We're setting it to 00 (which
    // is the reset value anyway), to use 8-bit words and one start bit.
    USARTx->CR1 &= ~USART_CR1_M;

    // Configure stop bits to 1 stop bit (which is the default). Other
    // choices are .5, 1.5 and 2 stop bits.
    USARTx->CR2 &= ~USART_CR2_STOP;   

    // Set baudrate as desired. This is done by dividing down the APB1 clock.
    // E.g., 80MHz/9600 = 8333 = 0x208D.
    // (We're oversampling by 16; the calculation would be slightly
    // different if we were 8x mode).
    extern uint32_t SystemCoreClock;
    uint32_t val = SystemCoreClock / baud;
    USARTx->BRR  = val;

    // Configure oversampling mode: Oversampling by 16 (which is the
    // default). This means that our Rx runs at 16x the nominal baud rate.
    // If we're not enabling the Rx anyway, this step is moot (but harmless).
    USARTx->CR1 &= ~USART_CR1_OVER8;

    // Turn on transmitter and receiver enables. Note that the entire USART
    // is still disabled, though. Turning on the Rx enable kicks off the Rx
    // looking for a stop bit.
    if (tx_en)
        USARTx->CR1  |= USART_CR1_TE;
    if (rx_en)
        USARTx->CR1  |= USART_CR1_RE;
        
    // We originally turned off the USART -- now turn it back on.
    // Note that page 1202 says to turn this on *before* asserting TE and/or RE.
    USARTx->CR1  |= USART_CR1_UE; // USART enable                 
        
    // Verify that the USART is ready to transmit...
    if (tx_en)
        while ( (USARTx->ISR & USART_ISR_TEACK) == 0)
            ;
    // ... and to receive.
    if (rx_en)
        while ( (USARTx->ISR & USART_ISR_REACK) == 0)
            ;

    NVIC_SetPriority(38, 2); // (IRQ number, priority)
    NVIC_EnableIRQ(38);
}


/**
 * Configure the serial connection to the host PC.
 * This will always be on USART 2 and GPIO pins A7 (PA2) and PA15, since those
 * are the wired connections to the host bridge chip on the Nucleo PCB.
 * 
 * :param baud: The baud rate, typically 9600 or 115200.
 */
void host_serial_init(const unsigned int baud) {

    // Enable USART 2 clock
    RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN;  

    // Select SYSCLK as the USART2 clock source. The reset default is PCLK1;
    // we usually set both SYSCLK and PCLK1 to 80MHz anyway.
    RCC->CCIPR &= ~RCC_CCIPR_USART2SEL;
    RCC->CCIPR |=  RCC_CCIPR_USART2SEL_0;

    // Connect the I/O pins to the serial peripheral
    //UART2_GPIO_Init();
    gpio_config_alternate_function(A7, 7); // PA 2, AF7
    gpio_config_alternate_function(VCP_RX, 3); // PA 15, AF3


    USART_Init(USART2, 1, 1, baud);        // Enable both Tx and Rx sides.
}

/**
 * Write a buffer to the serial port.  This function blocks until the
 * transmission is complete.
 *
 * :param USARTx: Pointer to the CMSIS USART struct, either USART1 or USART2
 * :param buffer: Pointer to array of bytes to send
 * :param len: Number of bytes to transmit
 */
void serial_write(USART_TypeDef *USARTx, const char *buffer, int len) {
    // The main flag we use is Tx Empty (TXE). The HW sets it when the
    // transmit data register (TDR) is ready for more data. TXE is then
    // cleared when we write new data in (by a write to the USART_DR reg).
    // When the HW transfers the TDR into the shift register, it sets TXE=1.
    for (unsigned int i = 0; i < len; i++) {
        // Wait until the "Transmit data register empty" flag is set
        // This is waiting to send each bit of the byte and the stop start bytes
        while (!(USARTx->ISR & USART_ISR_TXE)) {}

        // Put the next byte into the buffer; this will automatically clear the TXE flag
        USARTx->TDR = buffer[i];
    }

    // RM0394 page 1203 says that you must wait for ISR.TC=1 before you shut
    // off the USART. We never shut off the USART... but we'll wait anyway.
    while (!(USARTx->ISR & USART_ISR_TC)); // wait for it to be on
    USARTx->ISR &= ~USART_ISR_TC; // now turn it off
}

/**
 * Write a buffer to the serial port.  Returns quickly, and pushes bytes to
 * the serial peripheral asynchronously.
 * 
 * Should only really be used if the TXEIE bit is written
 * 
 * :param USARTx: Pointer to the CMSIS USART struct, either USART1 or USART2
 * :param buffer: Pointer to array of bytes to send
 * :param len: Number of bytes to transmit
 *
 * :return: The number of bytes successfully pushed to the UART TX buffer.  This
 * will equal `len` if the entire buffer was written.
 *
 * For the purposes of Lab 8, it's ok if you only implement this for UART2.
 * A more general solution would need multiple circular buffers and select
 * between them based on the value of USARTx.
 */
int serial_write_nonblocking(USART_TypeDef *USARTx, const char *buffer, int len) {
    int iters = len;
    if (len > TX_buffer_len){
        iters = TX_buffer_len-1;
    }

    for(int i=0; i < iters;i++){
        TX_buffer[write_pos] = buffer[i]; // copy byte to TX_buffer at write index
        write_pos = (write_pos + 1) % TX_buffer_len; // looped increment
    }
    if(len > TX_buffer_len){
        return TX_buffer_len;
    }
    return len;
}

/**
 * Read a byte from the serial port.  This blocks until a byte comes in and
 * then returns that value.
 * Wouldn't it be nice if this were buffered and asynchronous too?
 *
 * :param USARTx: Pointer to the CMSIS USART struct, either USART1 or USART2
 *
 * :return: Returns a byte read from the serial port. 
 */
char serial_read (USART_TypeDef *USARTx) {
    // The SR_RXNE (Read data register not empty) bit is set by hardware.
    // We spin wait until that bit is set
    while (!(USARTx->ISR & USART_ISR_RXNE)) {}

    // Reading USART_DR automatically clears the RXNE flag 
    return ((char)(USARTx->RDR & 0xFF));
}

#include <stm32l432xx.h>
#include "ee14lib.h"

// Configures DMA for Memory to Peripheral mode for DAC1
// Need to call dma_set_memaddr before use works
// Always returns OK signal
// No params
EE14Lib_Err dma_dac_config(){
    DMA1_Channel1->CCR &= 0b00 << 10; // 8 bit mem size
    DMA1_Channel1->CCR &= 0b00 << 8; // 8 bit peripheral size
    DMA1_Channel1->CCR |= 0b1 << 4; // read from peripheral
    DMA1_Channel1->CCR |= 0b1 << 7; // set memory incremement mode
    DMA1_Channel1->CCR |= 0b1 << 5; // set circular mode
    DMA1_Channel1->CPAR = &(DAC1->DHR8R1); // move data to DAC's Data Hold Register
    // DOESN'T SET CMAR. CALL dma_set_memaddr()
    // DAC CHANNEL 1 IS DMA1 Channel 3 CxS[3:0] == 0110
    DMA1_CSELR->CSELR &= ~(0b1001 << 8); 
    DMA1_CSELR->CSELR |= 0b0110 << 8; 
    return EE14Lib_Err_OK;
}


// Sets the dma memory address to a particular location
// Params: a pointer to an array of 8-bit values
// Values will stream out via DAC at ~ 44.1 kHz, circularly
// Always returns OK signal
EE14Lib_Err dma_set_memaddr(uint8_t *buffer){
    DMA1_Channel1->CMAR = buffer;
    return EE14Lib_Err_OK;
}
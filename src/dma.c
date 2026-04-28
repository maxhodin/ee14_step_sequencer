#include <stm32l432xx.h>
#include "ee14lib.h"

// Configures DMA for Memory to Peripheral mode for DAC1
// Need to call dma_set_memaddr before use works
// Always returns OK signal
// No params
EE14Lib_Err dma_dac_config(uint16_t len_buffer){
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;

    DMA1_Channel3->CCR &= ~DMA_CCR_EN;
    while (DMA1_Channel3->CCR & DMA_CCR_EN);

    // clear interrupt flags
    DMA1->IFCR |= DMA_IFCR_CGIF3  |   
                  DMA_IFCR_CTCIF3 |   
                  DMA_IFCR_CHTIF3 |   
                  DMA_IFCR_CTEIF3;    

    
    DMA1_CSELR->CSELR &= ~DMA_CSELR_C3S;           // clear channel 3 selection
    DMA1_CSELR->CSELR |= (6U << DMA_CSELR_C3S_Pos); // set to request 6 to DAC_CH1

    // put data in data hold register for 8 bits right
    DMA1_Channel3->CPAR = (uint32_t)&(DAC1->DHR8R1);

    DMA1_Channel3->CNDTR = len_buffer;

    /* Configure channel control register */ // -- this blurb from claude
    DMA1_Channel3->CCR =
        DMA_CCR_CIRC    |   // Circular mode: auto-reload CNDTR
        DMA_CCR_MINC    |   // Memory increment: walk through LUT
                            // No PINC: peripheral address fixed at DHR8R1
        DMA_CCR_DIR     |   // Direction: Memory -> Peripheral
                            // MSIZE = 00: 8-bit memory width (default)
                            // PSIZE = 00: 8-bit peripheral width (default)
        DMA_CCR_PL_1;       // Priority: High

    DMA1_Channel3->CCR |= DMA_CCR_EN;

    // Enable the half and complete transfer interrupt
    DMA1_Channel3->CCR |= DMA_CCR_HTIE;
    DMA1_Channel3->CCR |= DMA_CCR_TCIE;  

    return EE14Lib_Err_OK;
}


// Sets the dma memory address to a particular location
// Params: a pointer to an array of 8-bit values
// Values will stream out via DAC at ~ 44.1 kHz, circularly
// Always returns OK signal
EE14Lib_Err dma_set_memaddr(uint8_t *buffer){
    DMA1_Channel3->CMAR = (uint32_t) buffer;
    return EE14Lib_Err_OK;
}

// Enables the DMA controller
// No params, always returns OK signal
// Call this to roughly mark the timing of the beginning of the buffer shifting
// First element of the buffer should appear on the DAC channel 1-2 clk cycles later
EE14Lib_Err dma_enable(){
    DMA1_Channel3->CCR |= 0b1;
    return EE14Lib_Err_OK;
}
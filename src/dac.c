#include <stm32l432xx.h>
#include "ee14lib.h"

// DRIVER FOR DAC
// BUILT WITH AUDIO OUTPUT IN MIND
// EE14, MAX HODIN 4/5/26

// Pins for DAC1_OUT1 and DAC1_OUT2 respectively
#define DAC_PIN1 A3
#define DAC_PIN2 A4

volatile uint32_t *data_ptr;

// Wakes up the DAC from clock gating sleep
// DAC is left disabled, need to call a config function before using
// Can be called at any time
// No return no arguments
static void dac_wakeup(void){
    // Mirrors convention from ADC.C to disable the DAC before enabling the clock on the DAC
    DAC1->CR &= ~DAC_CR_EN1;  

    // Enable the clock of DAC
    // RCC_AHB1ENR is the AHB1 peripheral-clock-enable register. 
    // DAC1EN enables the clock to the only DAC on the chip.
    RCC->AHB1ENR  |= RCC_APB1ENR1_DAC1EN;

    // RCC_AHB2RSTR is the AHB2 peripheral-reset register.
    // Put the ADC into reset, wait, and take it out, using RCC_AHB2RSTR.ADCRST
    RCC->AHB1RSTR    |=  RCC_APB1RSTR1_DAC1RST; // Go into reset.
    for (volatile int i=0; i<5; ++i) {}       // Wait till we're really in reset.
    RCC->AHB1RSTR    &= ~RCC_APB1RSTR1_DAC1RST; // Come out of reset.
    for (volatile int i=0; i<5; ++i) {}       // Wait till we're really out of reset

    // DAC starts in STOP mode after reset

}

// Configures the DAC on Channel 1 by setting the data register global var
// NOT DMA, Wakes up DAC is needed
// Returns error signal
// Argument: integer (expects 0-2) 
//    0: 8-bit right alignment
//    1: 12-bit left alignment
//    2: 12-bit right alignment
EE14Lib_Err dac_config_single(int alignment_mode){
    // if DAC is sleeping, wake up
    if(!(RCC->AHB1ENR & RCC_APB1ENR1_DAC1EN)){
        dac_wakeup();
    }

    // manually disable the DAC as we change settings
    DAC1->CR &= ~DAC_CR_EN1;

    // set alignment mode and store result in global scope
    if(alignment_mode==0){
        data_ptr = &DAC1->DHR8RD;
    }
    else if(alignment_mode==1){
        data_ptr = &DAC1->DHR12LD;
    }
    else if(alignment_mode==2){
        data_ptr = &DAC1->DHR12RD;
    }
    else{
       return EE14Lib_ERR_INVALID_CONFIG; 
    }

    // DAC Channel 1 is connected to external pin with buffer enabled in normal mode
    DAC1->MCR &= 0b000;

    //enable the DAC
    DAC1->CR |= DAC_CR_EN1;
    
    return EE14Lib_Err_OK;
}

// Writes value to the DAC to be turned into an analog voltage
// Returns error signal
// Arugment: 
//  integer val: should be [0,255] for 8-bit mode or [0,4096] for 12-bit mode
EE14Lib_Err dac_write(int val){
    if(val > 255){ // val is not 8 bits but config is
        if(data_ptr == &DAC1->DHR8RD){
            return EE14Lib_ERR_INVALID_CONFIG;
        }
    }
    else if(val > 4096){ // val is not 12 bits
        return EE14Lib_ERR_INVALID_CONFIG;
    }
    
    //write val to data_ptr's register
    *data_ptr = val;
    return EE14Lib_Err_OK;
}

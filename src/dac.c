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
    RCC->APB1ENR1  |= RCC_APB1ENR1_DAC1EN;

    // RCC_AHB2RSTR is the AHB2 peripheral-reset register.
    // Put the ADC into reset, wait, and take it out, using RCC_AHB2RSTR.ADCRST
    RCC->APB1RSTR1    |=  RCC_APB1RSTR1_DAC1RST; // Go into reset.
    for (volatile int i=0; i<5; ++i) {}       // Wait till we're really in reset.
    RCC->APB1RSTR1    &= ~RCC_APB1RSTR1_DAC1RST; // Come out of reset.
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
    if(!(RCC->APB1ENR1 & RCC_APB1ENR1_DAC1EN)){
        dac_wakeup();
    }

    // manually disable the DAC as we change settings
    DAC1->CR &= ~DAC_CR_EN1;

    //Set the GPIO port mode to analog to allow analogness
    gpio_config_mode(A3,ANALOG);
    gpio_config_pullup(A3, PULL_OFF);

    // set alignment mode and store result in global scope
    if(alignment_mode==0){
        data_ptr = &DAC1->DHR8R1;
    }
    else if(alignment_mode==1){
        data_ptr = &DAC1->DHR12L1;
    }
    else if(alignment_mode==2){
        data_ptr = &DAC1->DHR12R1;
    }
    else{
       return EE14Lib_ERR_INVALID_CONFIG; 
    }

    // DAC Channel 1 is connected to external pin with buffer enabled in normal mode
    DAC1->MCR &= 0b000;

    // enable DMA request
    DAC1->CR |= DAC_CR_DMAEN1; 

    // Turns triggers on 
    DAC1->CR |= DAC_CR_TEN1;

    //100 in TSEL[2:0] is TIM2 trigger
    DAC1->CR &= (0b111 << 3);
    DAC1->CR |= 0b100 << 3;

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
        if(data_ptr == &DAC1->DHR8R1){
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

// inits the dac for tim6 based triggers and DMA data feeding
// no return
// param: sample rate in Hz. Should probably be something like 44118
void dac_tim6trig_init(uint32_t sample_rate)
{
    // clock gating reenable in case theyre not
    RCC->APB1ENR1 |= RCC_APB1ENR1_DAC1EN;
    RCC->APB1ENR1 |= RCC_APB1ENR1_TIM6EN;
    RCC->AHB2ENR  |= RCC_AHB2ENR_GPIOAEN;

    // init pin mode in case it wasn't
    GPIOA->MODER  |=  (3U << GPIO_MODER_MODE4_Pos);  // 11 = analog
    GPIOA->PUPDR  &= ~(3U << GPIO_PUPDR_PUPD4_Pos);  // no pull


    TIM6->PSC  = 0; // can change if necessary but i think that will hurt our performance
    TIM6->ARR  = (SystemCoreClock / sample_rate) - 1;
    TIM6->CR2 |= TIM_CR2_MMS_1;   // MMS = 010: Update -> TRGO
    TIM6->CR1 |= TIM_CR1_CEN;

    DAC1->CR = DAC_CR_DMAEN1  |   // enable DMA request
               DAC_CR_TEN1    |   // trigger enable
               (0U << DAC_CR_TSEL1_Pos) |  // TSEL = 000 = TIM6 on L432
               DAC_CR_EN1;        // enable CH1
}
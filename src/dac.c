#include <stm32l432xx.h>
#include "ee14lib.h"

// DRIVER FOR DAC
// BUILT WITH AUDIO OUTPUT IN MIND
// EE14, MAX HODIN 4/5/26

// Pins for DAC1_OUT1 and DAC1_OUT2 respectively
#define DAC_PIN1 A3
#define DAC_PIN2 A4

void dac_wakeup(void){
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



#include "ee14lib.h"
#include <string.h>

// Lookup table for waveforms

// SINE is a sine wave
// TRIANGLE is a ramp to 255 then a decay back to 0
// Ramp is a ramp to 255 then an abrupt switch down to 0
// Exp pulse is (e^x - 1) / (e - 1) from 0-127 and (e^-x - 1) / (e - 1) from 128-255

/* Sine LUT - 64 steps, 8-bit unsigned */
const uint8_t sine_LUT[64] = {
    128, 140, 152, 165, 176, 188, 199, 209,
    218, 226, 234, 240, 245, 249, 252, 254,
    255, 254, 252, 249, 245, 240, 234, 226,
    218, 209, 199, 188, 176, 165, 152, 140,
    128, 115, 103,  90,  79,  67,  56,  46,
     37,  29,  21,  15,  10,   6,   3,   1,
      0,   1,   3,   6,  10,  15,  21,  29,
     37,  46,  56,  67,  79,  90, 103, 115
};

/* Triangle LUT - 64 steps, 8-bit unsigned */
const uint8_t triangle_LUT[64] = {
      0,   8,  16,  24,  32,  40,  48,  56,
     64,  72,  80,  88,  96, 104, 112, 120,
    128, 136, 144, 152, 160, 168, 176, 184,
    192, 200, 208, 216, 224, 232, 240, 248,
    255, 248, 240, 232, 224, 216, 208, 200,
    192, 184, 176, 168, 160, 152, 144, 136,
    128, 120, 112, 104,  96,  88,  80,  72,
     64,  56,  48,  40,  32,  24,  16,   8
};

/* Ramp (sawtooth) LUT - 64 steps, 8-bit unsigned */
const uint8_t ramp_LUT[64] = {
      0,   4,   8,  12,  16,  20,  24,  28,
     32,  36,  40,  44,  48,  52,  56,  60,
     64,  68,  72,  76,  80,  84,  88,  92,
     96, 100, 104, 108, 112, 116, 120, 124,
    128, 132, 136, 140, 144, 148, 152, 156,
    160, 164, 168, 172, 176, 180, 184, 188,
    192, 196, 200, 204, 208, 212, 216, 220,
    224, 228, 232, 236, 240, 244, 248, 252
};

/* Exponential pulse LUT - 64 steps, 8-bit unsigned */
const uint8_t exp_pulse_LUT[64] = {
      0,   0,   1,   2,   4,   6,   9,  13,
     17,  22,  28,  35,  42,  51,  60,  70,
     81,  93, 106, 120, 134, 149, 164, 179,
    194, 209, 223, 236, 246, 253, 255, 253,
    246, 236, 223, 209, 194, 179, 164, 149,
    134, 120, 106,  93,  81,  70,  60,  51,
     42,  35,  28,  22,  17,  13,   9,   6,
      4,   2,   1,   0,   0,   0,   0,   0
};

// buffer with two 256 sample halves.
uint8_t sound_buffer[512];
uint16_t write_idx = 0;

uint32_t phase_accumulator = 0;
uint32_t phase_step;


typedef enum {a,as,b,c,cs,d,ds,e,f,fs,g,gs} NOTE;
const uint32_t note_phase_LUT[12] = {45960056,48692591,51588075,54655908,57906538,61349364,64996921,68861744,72957412,77295414,81891420,86761097};

// Here write half transfer and FT
void DMA1_Channel3_IRQHandler(void) {
    // Handle Half transfer
    if (DMA1->ISR & DMA_ISR_HTIF3) {
        DMA1->IFCR |= DMA_IFCR_CHTIF3; // Clear the flag
        for (int i = 0; i < 256; i++) {
            phase_accumulator += phase_step;
            
            // turn 32 bits to 6 bits by shifting by 26
            uint8_t index = (uint32_t)(phase_accumulator >> 26);
            
            // 3. Write to the DMA buffer
            sound_buffer[i] = sine_LUT[index];
        }

    }
    // Handle Complete Transfer
    if (DMA1->ISR & DMA_ISR_TCIF3) {
        DMA1->IFCR |= DMA_IFCR_CTCIF3; // Clear the flag
        for (int i = 0; i < 256; i++) {
            phase_accumulator += phase_step;
            
            // turn 32 bits to 6 bits by shifting by 26
            uint8_t index = (uint32_t)(phase_accumulator >> 26);
            
            //Write to the DMA buffer
            sound_buffer[i+256] = sine_LUT[index];
        }
    }
}


// This function is called by printf() to handle the text string
// We want it to be sent over the serial terminal, so we just delegate to that function
int _write(int file, char *data, int len) {
    // Eventually you'll want to change this to serial_write_nonblocking
    serial_write(USART2, data, len);
    return len;
}

int main(){
    //RCC-> CR[7:4] == 1011 for maximum 48 MHz
    uint32_t oldClk = RCC->CR;
    oldClk |= 0b1011 << 4;
    oldClk &= ~(0b100 << 4);
    //CRITICAL SECTION 
    __disable_irq();
    RCC->CR = oldClk;
    __enable_irq();

    NVIC_SetPriority(DMA1_Channel3_IRQn, 1);  // set priority 
    NVIC_EnableIRQ(DMA1_Channel3_IRQn);       // enable in NVIC

    // Wait for clk to be ready 
    while(!(RCC->CR & RCC_CR_MSIRDY));
    
    host_serial_init(9600);


    DAC_TIM6_Init(41118);
    dma_dac_config(512);
    dma_set_memaddr(sound_buffer);
    dma_enable();

    phase_step = note_phase_LUT[0]*2; // times octave
    while(1){
        
    }
    return 0;
}

#include "ee14lib.h"
#include <string.h>

#define AUDIO_HALF 64
#define AUDIO_LEN  128
#define NUM_NOTES 7


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

// buffer with two sample halves.
uint8_t sound_buffer[AUDIO_LEN];
uint16_t write_idx = 0;


// uint32_t phase_accumulator_arr[NUM_NOTES] = {0};
// volatile uint8_t note_active[NUM_NOTES] = {0};

uint32_t phase_accumulator = 0;
uint32_t phase_step;


typedef enum {a,as,b,c,cs,d,ds,e,f,fs,g,gs} NOTE;
const uint32_t note_phase_LUT[12] = {45960056,48692591,51588075,54655908,57906538,61349364,64996921,68861744,72957412,77295414,81891420,86761097};

//Set up interrupt for when clock line goes low (PA0)
//PA1 reads bits (DATA) (Input with pull up)

//Read low start bit
//Collect 8 data bits
//Read parity bit (?) count num of 1s, high if even, low if odd
//Read Stop Bit (High)

volatile uint16_t ps2_shift = 0;
volatile uint8_t ps2_bitcount = 0;

volatile uint8_t ps2_data = 0;
volatile uint8_t ps2_ready = 0;
volatile bool break_seen = false;

const char *ps2_codes[256][4] = {};


volatile bool note_on = false;
volatile int8_t current_note = -1;

int8_t ps2_note_map[256];

void initialize_codes(){
    for(int i = 0; i < 4; i++){
        ps2_codes[28][i] = "A"; //A
        ps2_codes[27][i] = "B"; //S
        ps2_codes[35][i] = "C"; //D
        ps2_codes[43][i] = "D"; //F
        ps2_codes[52][i] = "E"; //G
        ps2_codes[51][i] = "F"; //H
        ps2_codes[59][i] = "G"; //J

        // ps2_codes[21][i] = "A#"; //Q
        // ps2_codes[29][i] = "B#"; //W
        // ps2_codes[36][i] = "C#"; //E
        // ps2_codes[45][i] = "D#"; //R
        // ps2_codes[44][i] = "E#"; //T
        // ps2_codes[53][i] = "F#"; //Y
        // ps2_codes[60][i] = "G#"; //U

        // ps2_codes[26][i] = "Ab"; //Z
        // ps2_codes[34][i] = "Bb"; //X
        // ps2_codes[33][i] = "Cb"; //C
        // ps2_codes[42][i] = "Db"; //V
        // ps2_codes[50][i] = "Eb"; //B
        // ps2_codes[49][i] = "Fb"; //N
        // ps2_codes[58][i] = "Gb"; //M
    }
}

void initialize_note_map(void){
    for(int i = 0; i < 256; i++){
        ps2_note_map[i] = -1;
    }

    // white-key row
    ps2_note_map[28] = a;   // A key -> A note
    ps2_note_map[27] = b;   // S key -> B note
    ps2_note_map[35] = c;   // D key -> C note
    ps2_note_map[43] = d;   // F key -> D note
    ps2_note_map[52] = e;   // G key -> E note
    ps2_note_map[51] = f;   // H key -> F note
    ps2_note_map[59] = g;   // J key -> G note

    // // sharp row
    // ps2_note_map[21] = as;  // Q key -> A#
    // ps2_note_map[36] = cs;  // E key -> C#
    // ps2_note_map[45] = ds;  // R key -> D#
    // ps2_note_map[53] = fs;  // Y key -> F#
    // ps2_note_map[60] = gs;  // U key -> G#
}


void ps2_init(void)
{
    //Enable clocks
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    //A0 = CLK (input)
    gpio_config_mode(A0, INPUT);
    gpio_config_pullup(A0, PULL_UP);

    //A1 = DATA (input)
    gpio_config_mode(A1, INPUT);
    gpio_config_pullup(A1, PULL_UP);

    //Route EXTI0 → PA0
    SYSCFG->EXTICR[0] &= ~SYSCFG_EXTICR1_EXTI0;
    SYSCFG->EXTICR[0] |=  SYSCFG_EXTICR1_EXTI0_PA;

    //Falling edge trigger
    EXTI->FTSR1 |= EXTI_FTSR1_FT0;
    EXTI->RTSR1 &= ~EXTI_RTSR1_RT0;

    //Unmask interrupt
    EXTI->IMR1 |= EXTI_IMR1_IM0;

    //Clear pending
    EXTI->PR1 |= EXTI_PR1_PIF0;

    //Enable NVIC
    NVIC_SetPriority(EXTI0_IRQn, 0);
    NVIC_EnableIRQ(EXTI0_IRQn);
    initialize_codes();
    initialize_note_map();
}

void EXTI0_IRQHandler(void)
{
    if (EXTI->PR1 & EXTI_PR1_PIF0)
    {
        //clear interrupt
        EXTI->PR1 |= EXTI_PR1_PIF0;
        //read DATA line
        uint8_t bit = gpio_read(A1);

        //Shift in bit (LSB first)
        ps2_shift |= (bit << ps2_bitcount);
        ps2_bitcount++;

        if (ps2_bitcount == 11)
        {
            //Optional frame validation
            uint8_t start = ps2_shift & 1;
            uint8_t stop  = (ps2_shift >> 10) & 1;

            if (start == 0 && stop == 1)
            {
                ps2_data = (ps2_shift >> 1) & 0xFF;
                ps2_ready = 1;
            }

            //Reset for next frame
            ps2_shift = 0;
            ps2_bitcount = 0;
        }
    }
}


void fill_audio_half(int start){
    if(!note_on){
        for(int i = 0; i < AUDIO_HALF; i++){
            sound_buffer[start + i] = 128;
        }
        return;
    }

    for(int i = 0; i < AUDIO_HALF; i++){
        phase_accumulator += phase_step;
        uint8_t index = phase_accumulator >> 26;
        sound_buffer[start + i] = sine_LUT[index];
    }
}


void DMA1_Channel3_IRQHandler(void) {
    if (DMA1->ISR & DMA_ISR_HTIF3) {
        DMA1->IFCR |= DMA_IFCR_CHTIF3;
        fill_audio_half(0);
    }

    if (DMA1->ISR & DMA_ISR_TCIF3) {
        DMA1->IFCR |= DMA_IFCR_CTCIF3;
        fill_audio_half(AUDIO_HALF);
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
    ps2_init();

    dac_tim6trig_init(41118);
    dma_dac_config(AUDIO_LEN);
    dma_set_memaddr(sound_buffer);
    dma_enable();

    int octave = 1;
    phase_step = note_phase_LUT[0] * octave; // times octave
    while(1){
        if(ps2_ready){
            ps2_ready = false;

            uint8_t code = ps2_data;

            // PS2 ends 0xF0 before a release code
            if(code == 0xF0){
                break_seen = true;
            }
            else{
                int8_t note = ps2_note_map[code];

                if(note >= 0){
                    const char *out = ps2_codes[code][0];

                    if(out == 0){
                        out = "?";
                    }

                    if(break_seen){
                        break_seen = false;


                        if(note == current_note){
                            note_on = false;
                            phase_step = 0;
                        }

                        printf("Released: code=%d, note=%s\n", code, out);
                    }
                    else{
                        current_note = note;
                        phase_step = note_phase_LUT[note];
                        note_on = true;

                        printf("Pressed: code=%d, note=%s\n", code, out);
                    }
                }
                else{
                    // unknown key 
                    break_seen = false;
                }
            }
        }
    }
    return 0;
}





// // Here write half transfer and FT
// void DMA1_Channel3_IRQHandler(void) {
//     // Handle Half transfer
//     if (DMA1->ISR & DMA_ISR_HTIF3) {
//         DMA1->IFCR |= DMA_IFCR_CHTIF3; // Clear the flag
//         for (int i = 0; i < 256; i++) {
//             phase_accumulator += phase_step;
            
//             // turn 32 bits to 6 bits by shifting by 26
//             uint8_t index = (uint32_t)(phase_accumulator >> 26);
            
//             // 3. Write to the DMA buffer
//             sound_buffer[i] = sine_LUT[index];
//         }

//     }
//     // Handle Complete Transfer
//     if (DMA1->ISR & DMA_ISR_TCIF3) {
//         DMA1->IFCR |= DMA_IFCR_CTCIF3; // Clear the flag
//         for (int i = 0; i < 256; i++) {
//             phase_accumulator += phase_step;
            
//             // turn 32 bits to 6 bits by shifting by 26
//             uint8_t index = (uint32_t)(phase_accumulator >> 26);
            
//             //Write to the DMA buffer
//             sound_buffer[i+256] = sine_LUT[index];
//         }
//     }
// }


#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <math.h>

#include "stm32l432xx.h"
#include "ee14lib.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"

#define NUM_WAVEFORMS 4
#define NUM_BARS      32

const uint8_t waveform_bar_LUT[NUM_WAVEFORMS][NUM_BARS] = {
// sine
{ 2, 3, 5, 7, 9,11,13,15,17,18,19,20,21,20,19,18,
 17,15,13,11, 9, 7, 5, 3, 2, 3, 5, 7, 9,11,13,15},

// triangle
{ 2, 4, 6, 8,10,12,14,16,18,20,21,21,20,18,16,14,
 12,10, 8, 6, 4, 2, 4, 6, 8,10,12,14,16,18,20,21},

// ramp
{ 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,
 18,19,20,21,21,20,19,18,17,16,15,14,13,12,11,10},

// exp_pulse
{ 2, 2, 2, 3, 4, 6, 8,11,14,17,20,22,21,19,16,13,
 10, 8, 6, 4, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2}
};

uint8_t waveform_num;


void delay(int time) {
for (volatile int i = 0; i < time; i++) {}
}

int _write(int file, char *data, int len) {
(void)file;
serial_write(USART2, data, len);
return len;
}

bool i2c1_write_bytes_debug(uint8_t addr7, const uint8_t *data, uint8_t len) {
I2C_TypeDef *I2Cx = I2C1;
int timeout;

timeout = 100000;
while ((I2Cx->ISR & I2C_ISR_BUSY) && --timeout) {}
if (timeout == 0) return false;

I2Cx->ICR = I2C_ICR_STOPCF |
I2C_ICR_NACKCF |
I2C_ICR_BERRCF |
I2C_ICR_ARLOCF |
I2C_ICR_OVRCF |
I2C_ICR_TIMOUTCF |
I2C_ICR_ALERTCF;

I2Cx->CR2 = 0;
I2Cx->CR2 |= ((uint32_t)addr7 << 1);
I2Cx->CR2 |= ((uint32_t)len << 16);
I2Cx->CR2 |= I2C_CR2_AUTOEND;
I2Cx->CR2 |= I2C_CR2_START;

for (uint8_t i = 0; i < len; i++) {
timeout = 100000;

while (!(I2Cx->ISR & I2C_ISR_TXIS)) {
if (I2Cx->ISR & I2C_ISR_NACKF) {
I2Cx->ICR = I2C_ICR_NACKCF | I2C_ICR_STOPCF;
return false;
}

if (--timeout == 0) {
return false;
}
}

I2Cx->TXDR = data[i];
}

timeout = 100000;
while (!(I2Cx->ISR & I2C_ISR_STOPF) && --timeout) {}
if (timeout == 0) return false;

I2Cx->ICR = I2C_ICR_STOPCF;
return true;
}

bool oled_i2c_write(uint8_t control, const uint8_t *data, size_t len) {
uint8_t buf[129];

if (len > 128) return false;

buf[0] = control;

for (size_t i = 0; i < len; i++) {
buf[i + 1] = data[i];
}

return i2c1_write_bytes_debug(0x3C, buf, (uint8_t)(len + 1));
}


void draw_note_bars(int note, bool isPlaying, int offset) {
ssd1306_Fill(Black);

int baseline = 36;
int bar_width = 3;
int gap = 1;

for (int i = 0; i < NUM_BARS; i++) {
int x = i * (bar_width + gap);

int lut_index = (i + offset) % NUM_BARS;

int height;
if (isPlaying) {
height = note_bar_LUT[note][lut_index];
} else {
height = 2;
}

for (int bx = 0; bx < bar_width; bx++) {
for (int y = baseline; y > baseline - height; y--) {
ssd1306_DrawPixel(x + bx, y, White);
}
}
}

int icon_x = 60;
int icon_y = 55;

if (!isPlaying) {
// play icon
for (int dx = 0; dx < 6; dx++) {
int half_height = dx / 2 + 1;
for (int dy = -half_height; dy <= half_height; dy++) {
ssd1306_DrawPixel(icon_x + dx, icon_y + dy, White);
}
}
} else {
// pause icon
for (int y = 0; y < 6; y++) {
ssd1306_DrawPixel(icon_x, icon_y - y, White);
ssd1306_DrawPixel(icon_x + 1, icon_y - y, White);
ssd1306_DrawPixel(icon_x + 4, icon_y - y, White);
ssd1306_DrawPixel(icon_x + 5, icon_y - y, White);
}
}

ssd1306_UpdateScreen();
}

int main(void) {
host_serial_init(9600);
delay(2000000);

printf("OLED bar waveform with icon test start\r\n");

i2c_init(I2C1, D5, D4); // D5 = SCL, D4 = SDA
delay(1000000);

bool ok = ssd1306_Init();
printf("ssd1306_Init = %d\r\n", ok ? 1 : 0);

float phase = 0.0f;
int offset = 0;
int note = 0;
bool isPlaying = true;

gpio_config_mode(A0,INPUT);
gpio_config_mode(A1, INPUT);
gpio_config_pullup(A0,PULL_DOWN);
gpio_config_pullup(A1,PULL_DOWN);

while (1) {
    uint8_t waveform_num_new = 2*((int)A0) + ((int)A1);
    if(waveform_num != waveform_num_new){
        waveform_num = waveform_num_new;
        draw_note_bars(note, 1, 0);
    }

delay(80000);
}

}

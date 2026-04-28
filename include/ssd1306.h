#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "ssd1306_conf.h"
#include "ssd1306_fonts.h"

typedef enum {
    Black = 0x00,
    White = 0x01
} SSD1306_COLOR;

bool ssd1306_Init(void);
void ssd1306_Fill(SSD1306_COLOR color);
void ssd1306_UpdateScreen(void);
void ssd1306_SetCursor(uint8_t x, uint8_t y);
char ssd1306_WriteChar(char ch, FontDef Font, SSD1306_COLOR color);
char ssd1306_WriteString(const char* str, FontDef Font, SSD1306_COLOR color);
void ssd1306_DrawPixel(uint8_t x, uint8_t y, SSD1306_COLOR color);

#endif
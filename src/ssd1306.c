#include "ssd1306.h"
#include <string.h>

#define OLED_CHUNK 2

extern void delay(int time);
extern bool oled_i2c_write(uint8_t control, const uint8_t *data, size_t len);

static uint8_t SSD1306_Buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];
static uint8_t CurrentX = 0;
static uint8_t CurrentY = 0;

void ssd1306_Reset(void) {
    // do nothing for I2C module
}

void ssd1306_WriteCommand(uint8_t byte) {
    oled_i2c_write(0x00, &byte, 1);
}

void ssd1306_WriteData(uint8_t* buffer, size_t buff_size) {
    oled_i2c_write(0x40, buffer, buff_size);
}

bool ssd1306_Init(void) {
    ssd1306_Reset();
    delay(100000);

    ssd1306_WriteCommand(0xAE); // display off

    ssd1306_WriteCommand(0xD5);
    ssd1306_WriteCommand(0x80);

    ssd1306_WriteCommand(0xA8);
    ssd1306_WriteCommand(0x3F);

    ssd1306_WriteCommand(0xD3);
    ssd1306_WriteCommand(0x00);

    ssd1306_WriteCommand(0x40);

    ssd1306_WriteCommand(0x8D);
    ssd1306_WriteCommand(0x14);

    ssd1306_WriteCommand(0x20);
    ssd1306_WriteCommand(0x00);

    ssd1306_WriteCommand(0xA1);
    ssd1306_WriteCommand(0xC8);

    ssd1306_WriteCommand(0xDA);
    ssd1306_WriteCommand(0x12);

    ssd1306_WriteCommand(0x81);
    ssd1306_WriteCommand(0xCF);

    ssd1306_WriteCommand(0xD9);
    ssd1306_WriteCommand(0xF1);

    ssd1306_WriteCommand(0xDB);
    ssd1306_WriteCommand(0x40);

    ssd1306_WriteCommand(0xA4);
    ssd1306_WriteCommand(0xA6);

    ssd1306_WriteCommand(0xAF); // display on

    ssd1306_Fill(Black);
    ssd1306_UpdateScreen();

    CurrentX = 0;
    CurrentY = 0;

    return true;
}

void ssd1306_Fill(SSD1306_COLOR color) {
    memset(SSD1306_Buffer, (color == Black) ? 0x00 : 0xFF, sizeof(SSD1306_Buffer));
}

void ssd1306_UpdateScreen(void) {
    for (uint8_t page = 0; page < 8; page++) {
        for (uint8_t col = 0; col < 128; col++) {
            ssd1306_WriteCommand(0xB0 + page);

            ssd1306_WriteCommand(0x00 | (col & 0x0F));
            ssd1306_WriteCommand(0x10 | ((col >> 4) & 0x0F));

            ssd1306_WriteData(&SSD1306_Buffer[page * 128 + col], 1);
        }
    }
}

void ssd1306_DrawPixel(uint8_t x, uint8_t y, SSD1306_COLOR color) {
    if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) {
        return;
    }

    if (color == White) {
        SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] |= 1 << (y % 8);
    } else {
        SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y % 8));
    }
}

void ssd1306_SetCursor(uint8_t x, uint8_t y) {
    CurrentX = x;
    CurrentY = y;
}

char ssd1306_WriteChar(char ch, FontDef Font, SSD1306_COLOR color) {
    uint32_t i, b, j;

    if (SSD1306_WIDTH <= (CurrentX + Font.FontWidth) ||
        SSD1306_HEIGHT <= (CurrentY + Font.FontHeight)) {
        return 0;
    }

    for (i = 0; i < Font.FontHeight; i++) {
        b = Font.data[(ch - 32) * Font.FontHeight + i];
        for (j = 0; j < Font.FontWidth; j++) {
            if ((b << j) & 0x8000) {
                ssd1306_DrawPixel(CurrentX + j, CurrentY + i, color);
            } else {
                ssd1306_DrawPixel(CurrentX + j, CurrentY + i, (SSD1306_COLOR)!color);
            }
        }
    }

    CurrentX += Font.FontWidth;
    return ch;
}

char ssd1306_WriteString(const char* str, FontDef Font, SSD1306_COLOR color) {
    while (*str) {
        if (ssd1306_WriteChar(*str, Font, color) != *str) {
            return *str;
        }
        str++;
    }

    return *str;
}

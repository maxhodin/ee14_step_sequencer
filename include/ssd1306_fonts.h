#ifndef SSD1306_FONTS_H
#define SSD1306_FONTS_H

#include <stdint.h>

typedef struct {
    uint8_t FontWidth;
    uint8_t FontHeight;
    const uint16_t *data;
    const uint32_t *char_offset;
} SSD1306_Font_t;

typedef SSD1306_Font_t FontDef;

extern const SSD1306_Font_t Font_7x10;
extern const SSD1306_Font_t Font_11x18;

#endif
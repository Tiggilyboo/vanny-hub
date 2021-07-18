#include <pico/stdlib.h>

typedef struct FontDef
{
    uint8_t font_width;
    uint8_t font_height;
    const uint8_t *data;
} FontDef_t;

typedef struct FontSize
{
    uint16_t length;
    uint16_t height;
} FontSize_t;

extern FontDef_t FontNormal;
extern FontDef_t FontTitle;


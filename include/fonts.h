#ifndef __FONTS_H__
#define __FONTS_H__
int print_xy(char *st, int x, int y);

void setFontColour(uint8_t red, uint8_t green, uint8_t blue);
int getFontHeight();
void gprintf(const char *fmt, ...);
typedef struct {
  uint16_t bitmapOffset; ///< Pointer into GFXfont->bitmap
  uint8_t width;         ///< Bitmap dimensions in pixels
  uint8_t height;        ///< Bitmap dimensions in pixels
  uint8_t xAdvance;      ///< Distance to advance cursor (x axis)
  int8_t xOffset;        ///< X dist from cursor pos to UL corner
  int8_t yOffset;        ///< Y dist from cursor pos to UL corner
} GFXglyph;

typedef struct {
  uint8_t *bitmap;  ///< Glyph bitmaps, concatenated
  GFXglyph *glyph;  ///< Glyph array
  uint16_t first;   ///< ASCII extents (first char)
  uint16_t last;    ///< ASCII extents (last char)
  uint8_t yAdvance; ///< Newline distance (y axis)
} GFXfont;

void setFont(GFXfont font);

#define PROGMEM

extern const GFXfont FreeSans9pt7b,FreeSans12pt7b,FreeSans18pt7b,FreeSans24pt7b,Font_5x7_practical8pt7b;
extern const GFXfont FreeSansBold9pt7b,FreeSansBold12pt7b,FreeSansBold18pt7b,FreeSansBold24pt7b,Font_5x7_practical8pt7b;
extern const GFXfont DejaVuSans24,DejaVuSans18,Ubuntu16,def_small;

#define FONT_SMALL def_small
#define FONT_UBUNTU16 Ubuntu16
#define FONT_DEJAVU18 DejaVuSans18
#define FONT_DEJAVU24 DejaVuSans24

#endif
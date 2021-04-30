#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "graphics.h"

extern unsigned char tft_Dejavu18[];
extern unsigned char tft_Dejavu24[];
extern unsigned char tft_def_small[];
extern unsigned char tft_Ubuntu16[];

typedef struct {
    uint8_t *font;
    uint8_t x_size;
    uint8_t y_size;
    uint8_t offset;
    uint16_t numchars;
    uint16_t size;
    uint8_t max_x_size;
    uint8_t bitmap;
    uint16_t color;
} Font;
Font tft_cfont = {
    .font = tft_def_small,
    .x_size = 0,
    .y_size = 0x8,
    .offset = 4,
    .numchars = 95,
    .bitmap = 1,
};
typedef struct {
    uint8_t charCode;
    int adjYOffset;
    int width;
    int height;
    int xOffset;
    int xDelta;
    uint16_t dataPtr;
} propFont;

static uint16_t ptrlookup[256];

uint16_t fontColour = -1;

void loadfont() {
    propFont fontChar;
    uint16_t tempPtr = 4;  // point at first char data
    do {
        fontChar.charCode = tft_cfont.font[tempPtr++];
        if (fontChar.charCode == 0xFF) return;
        ptrlookup[fontChar.charCode] = tempPtr - 1;
        fontChar.adjYOffset = tft_cfont.font[tempPtr++];
        fontChar.width = tft_cfont.font[tempPtr++];
        fontChar.height = tft_cfont.font[tempPtr++];
        fontChar.xOffset = tft_cfont.font[tempPtr++];
        fontChar.xOffset = fontChar.xOffset < 0x80 ? fontChar.xOffset
                                                   : -(0xFF - fontChar.xOffset);
        fontChar.xDelta = tft_cfont.font[tempPtr++];

        if (fontChar.charCode != 0xFF) {
            if (fontChar.width != 0) {
                // packed bits
                tempPtr += (((fontChar.width * fontChar.height) - 1) / 8) + 1;
            }
        }
    } while (fontChar.charCode != 0xFF);
}

const unsigned char * const all_fonts[] = {tft_def_small, tft_Ubuntu16, tft_Dejavu18,
                              tft_Dejavu24};

void setFont(uint8_t font) {
    if (font > sizeof(all_fonts) / sizeof(all_fonts[0])) return;
    tft_cfont.font = (unsigned char *)all_fonts[font];
    tft_cfont.bitmap = 1;
    tft_cfont.x_size = tft_cfont.font[0];
    tft_cfont.y_size = tft_cfont.font[1];
    tft_cfont.offset = 4;
    loadfont();
}

int getFontHeight() {
    return tft_cfont.y_size;
}

void setFontColour(uint16_t red, uint16_t green, uint16_t blue) {
    uint16_t v = ((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3);
    fontColour = v;
}

static void getCharPtr(uint8_t c, propFont *fontChar) {
    uint16_t tempPtr = 4;  // point at first char data
    if (ptrlookup[c] == 0) loadfont();
    tempPtr = ptrlookup[c];
    fontChar->charCode = tft_cfont.font[tempPtr++];
    fontChar->adjYOffset = tft_cfont.font[tempPtr++];
    fontChar->width = tft_cfont.font[tempPtr++];
    fontChar->height = tft_cfont.font[tempPtr++];
    fontChar->xOffset = tft_cfont.font[tempPtr++];
    fontChar->xOffset = fontChar->xOffset < 0x80 ? fontChar->xOffset
                                                 : -(0xFF - fontChar->xOffset);
    fontChar->xDelta = tft_cfont.font[tempPtr++];
    fontChar->dataPtr = tempPtr;
}

int printProportionalChar(int x, int y, propFont *fontChar) {
    uint8_t ch = 0;
    int i, j, char_width;

    char_width = ((fontChar->width > fontChar->xDelta) ? fontChar->width
                                                       : fontChar->xDelta);
    int cx, cy;

    // draw Glyph
    uint8_t mask = 0x80;
    for (j = 0; j < fontChar->height; j++) {
        for (i = 0; i < fontChar->width; i++) {
            if (((i + (j * fontChar->width)) % 8) == 0) {
                mask = 0x80;
                ch = tft_cfont.font[fontChar->dataPtr++];
            }

            if ((ch & mask) != 0) {
                cx = (uint16_t)(x + fontChar->xOffset + i);
                cy = (uint16_t)(y + j + fontChar->adjYOffset);
                draw_pixel(cx, cy, fontColour);
            }
            mask >>= 1;
        }
    }
    return char_width;
}

int lasty=0;
int lastx=0;
int print_xy(char *st, int x, int y) {
    int stl;
    uint8_t ch;
    int startx=x;
    int maxx=x;
    if(y==CENTER) {
        y=display_height/2-tft_cfont.y_size/2;
    }
    if(x==CENTER) {
        x=display_width/2-print_xy(st,0,-1)/2;
    }
    if(y>=LASTY)
        y=y-LASTY+lasty;
    propFont fontChar;
    if (tft_cfont.bitmap == 0) return 0;  // wrong font selected
    stl = strlen(st);
    for (int i = 0; i < stl; i++) {
        ch = st[i];
		if(ch=='\n') {
			x=startx;
			y=y+tft_cfont.y_size;
		} else {
            getCharPtr(ch, &fontChar);
            if(y>=0)
                x += printProportionalChar(x, y, &fontChar) + 1;
            else
                x += ((fontChar.width > fontChar.xDelta) ? fontChar.width
                                                        : fontChar.xDelta);
            if(x>maxx) maxx=x;
        }
    }
    if(y>0)
        lasty=y;
    lastx=x;
    return maxx-startx;
}

void gprintf(const char *fmt, ...) {
	char mesg[256];
	va_list args;
	va_start(args, fmt);
	vsnprintf(mesg,256,fmt,args);
	va_end(args);
	print_xy(mesg,lastx,lasty);
}

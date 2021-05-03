#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "graphics.h"
#include <fonts.h>

/*
#include "FreeSans9pt7b.h"
#include "FreeSans12pt7b.h"
#include "FreeSans18pt7b.h"
#include "FreeSans24pt7b.h"
#include "FreeSansBold9pt7b.h"
#include "FreeSansBold12pt7b.h"
#include "FreeSansBold18pt7b.h"
#include "FreeSansBold24pt7b.h"
#include "Font_5x7_practical.h"
*/

#include "DejaVuSans24.h"
#include "DejaVuSans18.h"
#include "Ubuntu16.h"
#include "def_small.h"

GFXfont cfont;

uint16_t fontColour = -1;

void setFont(GFXfont font) {
    cfont=font;
}

int getFontHeight() {
    return cfont.yAdvance;
}
int fcred=0;
int fcgreen=0;
int fcblue=0;
void setFontColour(uint8_t red, uint8_t green, uint8_t blue) {
    uint16_t v = ((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3);
    fontColour = v;
    fcred=red;
    fcgreen=green;
    fcblue=blue;
}
int anitalias=0;
int charWidth(char c) {
    if(c>cfont.last) return 0;
    int ch=c-cfont.first;
    if(ch<0) return 0;
    GFXglyph *glyph = cfont.glyph+ch;
    if(anitalias) return glyph->xAdvance/2;
    return glyph->xAdvance;
}

int printProportionalChar(int x, int y, char c) {
    

    if(c>cfont.last) return 0;
    int ch=c-cfont.first;
    if(ch<0) return 0;
    GFXglyph *glyph = cfont.glyph+ch;
    uint16_t bo = glyph->bitmapOffset;
    uint8_t w = glyph->width, h = glyph->height;
    int8_t xo = glyph->xOffset,
           yo = cfont.yAdvance/2+glyph->yOffset;
    uint8_t xx, yy, bits = 0, bit = 0;

    if (anitalias) {
        u_int16_t pixel=frame_buffer[display_width*y+x];
        int r= (pixel&0xf800)>>8;
        int g= (pixel&0x07e0)>>3;
        int b= (pixel&0x001f)<<3;

        int cols[5];//{0,rgbToColour(64,64,64),rgbToColour(128,128,128),rgbToColour(192,192,192),-1};
        for(int i=0;i<5;i++) {
            float fgm=i/4.0f;
            float bgm=1-fgm;
            cols[i]=rgbToColour(fgm*fcred+bgm*r,fgm*fcgreen+bgm*g,fgm*fcblue+bgm*b);
        }
        xo=xo/2;
        yo= cfont.yAdvance/2+glyph->yOffset+3;
        for (yy = 0; yy < h; yy+=2) {
            uint8_t line[w/2+1];
            for(int i=0;i<w/2+1;i++)
                line[i]=0;
            for(int yi=yy;yi<yy+2 && yi<h;yi++) {
            for (xx = 0; xx < w; xx++) {
                if (!(bit++ & 7)) {
                    bits = cfont.bitmap[bo++];
                }
                if (bits & 0x80) {
                    line[xx/2]++;
                }
                bits <<= 1;
            }
            }
            for(int i=0;i<w/2+1;i++) {
                if(line[i]) draw_pixel(x + xo + i, y + (yo + yy)/2, cols[line[i]]);
            }
        }
        return glyph->xAdvance/2;
    }

    for (yy = 0; yy < h; yy++) {
      for (xx = 0; xx < w; xx++) {
        if (!(bit++ & 7)) {
          bits = cfont.bitmap[bo++];
        }
        if (bits & 0x80) {
            draw_pixel(x + xo + xx, y + yo + yy, fontColour);
        }
        bits <<= 1;
      }
    }
    return glyph->xAdvance;
}

int lasty=0;
int lastx=0;
int print_xy(char *st, int x, int y) {
    int stl;
    uint8_t ch;
    int startx=x;
    int maxx=x;
    if(y==CENTER) {
        y=display_height/2-cfont.yAdvance/4;
    }
    if(x==CENTER) {
        x=display_width/2-print_xy(st,0,-1)/2;
    }
    if(y>=LASTY)
        y=y-LASTY+lasty;
    stl = strlen(st);
    for (int i = 0; i < stl; i++) {
        ch = st[i];
		if(ch=='\n') {
			x=startx;
			y=y+cfont.yAdvance/2;
		} else {
            if(y>=0)
                x += printProportionalChar(x, y, ch);
            else
                x += charWidth(ch);
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

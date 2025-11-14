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
int antialias=0;

void setFont(GFXfont font) {
    cfont=font;
}

void setAntialias(int v) {
    antialias=v;
}

int getFontHeight() {
    return cfont.yAdvance;
}

void setFontColour(uint8_t red, uint8_t green, uint8_t blue) {
    uint16_t v = ((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3);
    fontColour = v;
}

int charWidth(char c) {
    if(c>cfont.last) return 0;
    int ch=c-cfont.first;
    if(ch<0) return 0;
    GFXglyph *glyph = cfont.glyph+ch;
    if(antialias) return glyph->xAdvance/2;
    return glyph->xAdvance;
}

static int printProportionalChar(int x, int y, char c) {
    if(c>cfont.last) return 0;
    int ch=c-cfont.first;
    if(ch<0) return 0;
    GFXglyph *glyph = cfont.glyph+ch;
    uint16_t bo = glyph->bitmapOffset;
    uint8_t w = glyph->width, h = glyph->height;
    int8_t xo = glyph->xOffset,
           yo = cfont.yAdvance+glyph->yOffset;
    int maxy = cfont.yAdvance+glyph->yOffset+h+1;
    uint8_t xx, yy, bits = 0, bit = 0;
    

    if (antialias) {
        static uint16_t *buffer=0;
        static int buffer_size=0;
        if(maxy*glyph->xAdvance*2>buffer_size) {
            buffer_size=maxy*glyph->xAdvance*2;
            if(buffer!=0) free(buffer);
            buffer=malloc(maxy*glyph->xAdvance*2);
        }
        if(buffer==0) return 0;
        for(yy=0;yy<maxy;yy++) {
            for(xx=0;xx<glyph->xAdvance;xx++) {
                if((y+yy/2)<display_height && (x+xx/2)<display_width)
                    buffer[yy*glyph->xAdvance+xx]=frame_buffer[display_width*(y+yy/2)+x+xx/2];
            }
        }
        for (yy = 0; yy < h; yy++) {
            for (xx = 0; xx < w; xx++) {
                if (!(bit++ & 7)) 
                    bits = cfont.bitmap[bo++];
                if (bits & 0x80) {
                    if((yo+yy)*glyph->xAdvance+xo + xx<maxy*glyph->xAdvance)
                        buffer[(yo+yy)*glyph->xAdvance+xo + xx]= fontColour;
                }                 
                bits <<= 1;
            }
        }
       
        for(yy=0;yy<maxy/2;yy++) {
            for(xx=0;xx<glyph->xAdvance/2;xx++) {
                int r=0,g=0,b=0;
                for(int j=0;j<2;j++)
                    for(int i=0;i<2;i++) {
                        uint16_t pixel=buffer[(yy*2+j)*glyph->xAdvance+xx*2+i];
                        r+= (pixel&0xf800)>>8;
                        g+= (pixel&0x07e0)>>3;
                        b+= (pixel&0x001f)<<3;
                    }
                draw_pixel(x+xx,y+yy,rgbToColour(r/4,g/4,b/4));
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
        y=display_height/2-cfont.yAdvance/2;
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
            if(!antialias)
    			y=y+cfont.yAdvance;
            else
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

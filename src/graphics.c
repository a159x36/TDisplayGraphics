#include "graphics.h"
#include <freertos/FreeRTOS.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "st7789v.h"

// simple graphics library for TTGO T-Display used in 159236,
// Martin Johnson 2020

uint16_t fgc=0xffff;
uint16_t bgc=0;

int display_width=240;
int display_height=135;

inline void draw_pixel(uint16_t x, uint16_t y, uint16_t colour) {
    if(y>=display_height || x>=display_width) return;
    uint offset=y * display_width + x;
    frame_buffer[offset] = colour;
}
uint16_t *frame_buffer;

inline int clamp(int x,int min,int max) {
    if(x<min) return min;
    if(x>max) return max;
    return x;
}

void draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t colour) {
    x0=clamp(x0,0,display_width-1);
    x1=clamp(x1,0,display_width-1);
    y0=clamp(y0,0,display_height-1);
    y1=clamp(y1,0,display_height-1);
    int dx =  abs(x1-x0);
    int sx = x0<x1 ? 1 : -1;
    int dy = -abs(y1-y0);
    int sy = y0<y1 ? 1 : -1;
    int err = dx+dy;  /* error value e_xy */
    while (true) {   /* loop */
        draw_pixel(x0, y0, colour);
        if (x0==x1 && y0==y1) break;
        int e2 = 2*err;
        if (e2 >= dy) { /* e_xy+e_x > 0 */
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {/* e_xy+e_y < 0 */
            err += dx;
            y0 += sy;
        }
    }
}

// only RGBA for now
void draw_image(image_header *im, uint16_t x, uint16_t y) {
    int width=im->width;
    int height=im->height;
    uint32_t *pixel_data=(uint32_t *)(im);
    pixel_data+=3;
    for(uint16_t i=0;i<height;i++) {
        uint16_t imy=y+i-height/2;
        if(imy<display_height) {
            uint16_t *linestart=imy*display_width+frame_buffer;
            for(uint16_t j=0;j<width;j++) {
                uint16_t imx=x+j-width/2;
                if(imx<display_width) {
                    uint32_t pixel=*pixel_data++;
                    uint16_t trans=pixel>>24;
                    if(trans!=0) {
                        uint16_t *dest=linestart+imx;
                        uint16_t blue=(pixel>>16)&255;
                        uint16_t green=(pixel>>8)&255;
                        uint16_t red=pixel&255;
                        if(trans!=255) {
                            uint16_t dpixel=*dest;
                            uint16_t dred=(dpixel>>8) & 0xf8;
                            uint16_t dgreen=(dpixel>>3) & 0xfc;
                            uint16_t dblue=(dpixel<<3) & 0xf8;
                            red=(trans*red+(255-trans)*dred)>>8;
                            green=(trans*green+(255-trans)*dgreen)>>8;
                            blue=(trans*blue+(255-trans)*dblue)>>8;
                        }
                        *dest=((red<<8)&0xf800) | ((green<<3)&0x7e00) |  ((green>>3)& 0x1f);
                    }
                } else pixel_data++;
            }
        }
    }
}
void graphics_init() {
    frame_buffer =
        heap_caps_malloc(240*135 * sizeof(uint16_t), MALLOC_CAP_DMA);
    if (frame_buffer == 0) {
        printf("Error: Can't allocate frame buffer");
        return;
    }
    lcd_init();
}

void draw_rectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t colour) {
    if((x+w)>display_width) w=display_width-x;
    if((y+h)>display_height) h=display_height-y;
    for (int j=y; j<y+h;j++) {
        uint16_t *p=frame_buffer+j*display_width+x;
        for(int i=0; i<w;i++) {
            *p++=colour;
        }
    }
}

void cls(uint16_t colour) {
    if((colour>>8) == (colour&255)) {
        memset(frame_buffer,colour&255,240*135*2);
    }
    draw_rectangle(0,0,display_width,display_height,colour);
}

int frame_no=0;
void flip_frame() {
    send_frame();
    wait_frame();
}

inline uint16_t rgbToColour(uint16_t red, uint16_t green, uint16_t blue) {
    uint16_t v = ((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3);
    return v;
}



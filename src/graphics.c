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
uint16_t *fb1;
uint16_t *fb2;

inline int clamp(int x,int min,int max) {
    const int t = x < min ? min : x;
    return t > max ? max : t;
}

#define SWAP(a,b) {int16_t tx=x ## a;int16_t ty=y ## a;x ## a=x ## b;y ## a=y ## b;x ## b=tx;y ## b=ty;}

void draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t colour) {
    x0=clamp(x0,0,display_width-1);
    x1=clamp(x1,0,display_width-1);
    y0=clamp(y0,0,display_height-1);
    y1=clamp(y1,0,display_height-1);
    if(x0>x1) SWAP(0,1);
    int dx =  x1-x0;
    int dy = -abs(y1-y0);
    if(dy==0) {
        uint16_t *fb=frame_buffer+(y0*display_width)+x0;
        for(int i=0;i<dx+1;i++)
            *fb++=colour;
        return;
    }
    int sy = y0<y1 ? 1 : -1;
    int err = dx+dy;  /* error value e_xy */
    while (true) {   /* loop */
        draw_pixel(x0, y0, colour);
        if (x0==x1 && y0==y1) break;
        int e2 = 2*err;
        if (e2 >= dy) { /* e_xy+e_x > 0 */
            err += dy;
            x0++;
        }
        if (e2 <= dx) {/* e_xy+e_y < 0 */
            err += dx;
            y0 += sy;
        }
    }
}

void draw_triangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2 ,uint16_t colour) {
    x0=clamp(x0,0,display_width-1);
    x1=clamp(x1,0,display_width-1);
    x2=clamp(x2,0,display_width-1);
    y0=clamp(y0,0,display_height-1);
    y1=clamp(y1,0,display_height-1); 
    y2=clamp(y2,0,display_height-1);

    if(y1<y0) SWAP(0,1);
    if(y2<y0) SWAP(0,2);
    if(y2<y1) SWAP(1,2);

    int dx =  abs(x2-x0);
    int dy = -abs(y2-y0);
    int dx1 =  abs(x1-x0);
    int dy1 = -abs(y1-y0);
    int dx2 =  abs(x2-x1);
    int dy2 = -abs(y2-y1);
    int sx = x0<x2 ? 1 : -1;
    int sx1 = x0<x1 ? 1 : -1;
    int sx2 = x1<x2 ? 1 : -1;
    int err = dx+dy;
    int err1 = dx1+dy1;
    int err2 = dx2+dy2;
    int xx0=x0;
    int yy0=y0;
    /*
        Draw the dringle in two pieces
           (x0,y0) o
                  / \
                 /   \
       (x1,y1)  o.----\
                   .   \
                      . \
                         o (x2,y2)
    */
    while (true) {   /* for top of triangle*/
        if (yy0==y1 ) break;
        int e2 = 2*err;
        if (e2 >= dy) {
            err += dy;
            x0+=sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0++;
            while(true) {
                int e2 = 2*err1;
                if (e2 >= dy1) {
                    err1 += dy1;
                    xx0+=sx1;
                }
                if (e2 <= dx1) {
                    err1 += dx1;
                    yy0++;
                    if(x0<xx0) {
                        uint16_t *fb=frame_buffer+(y0*display_width)+x0;
                        for(int i=x0;i<=xx0;i++)
                            *fb++=colour;
                    } else {
                        uint16_t *fb=frame_buffer+(y0*display_width)+xx0;
                        for(int i=xx0;i<=x0;i++)
                            *fb++=colour;
                    }
                    break;
                }
            }
        }
    }
    xx0=x1;
    while (true) { // for bottom of triangle
        if (yy0==y2) break;
        int e2 = 2*err;
        if (e2 >= dy) { 
            err += dy;
            x0+=sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 ++;
            while(true) {
                int e2 = 2*err2;
                if (e2 >= dy2) {
                    err2 += dy2;
                    xx0+=sx2;
                }
                if (e2 <= dx2) {
                    err2 += dx2;
                    yy0++;
                    if(x0<xx0) {
                        uint16_t *fb=frame_buffer+(y0*display_width)+x0;
                        for(int i=x0;i<=xx0;i++)
                            *fb++=colour;
                    } else {
                        uint16_t *fb=frame_buffer+(y0*display_width)+xx0;
                        for(int i=xx0;i<=x0;i++)
                            *fb++=colour;
                    }
                    break;
                }
            }
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
        } else pixel_data+=width;
    }
}
void graphics_init() {
    fb1 =
        heap_caps_malloc(240*135 * sizeof(uint16_t), MALLOC_CAP_DMA);
    fb2 =
        heap_caps_malloc(240*135 * sizeof(uint16_t), MALLOC_CAP_DMA);
    frame_buffer=fb1;
    if (fb1 == 0 || fb2==0) {
        printf("Error: Can't allocate frame buffer");
        return;
    }
    lcd_init();
}

void draw_rectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t colour) {
    if(x>=display_width || y>=display_height) return;
    if((x+w)>display_width) w=display_width-x;
    if((y+h)>display_height) h=display_height-y;
    for (int j=y; j<y+h;j++) {
        uint16_t *p=frame_buffer+j*display_width+x;
        for(int i=0; i<w;i++) {
            *p++=colour;
        }
    }
}
extern int lastx, lasty;
void cls(uint16_t colour) {
    if((colour>>8) == (colour&255)) {
        memset(frame_buffer,colour&255,240*135*2);
    }
    draw_rectangle(0,0,display_width,display_height,colour);
    lastx=0;
    lasty=0;
}

int frame_no=0;
void flip_frame() {
    wait_frame();
    send_frame();
}

inline uint16_t rgbToColour(uint16_t red, uint16_t green, uint16_t blue) {
    uint16_t v = ((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3);
    return v;
}



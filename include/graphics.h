#include <stdio.h>
#include <st7789v.h>

// simple graphics library for TTGO T-Display used in 159236,
// Martin Johnson 2020
extern uint16_t *frame_buffer;
extern uint16_t *fb1;
extern uint16_t *fb2;

void draw_pixel( uint16_t x, uint16_t y, uint16_t colour);
void graphics_init();
void draw_rectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t colour);
void cls(uint16_t colour);
void flip_frame();
void send_frame();
void wait_frame();
uint16_t rgbToColour(uint16_t red, uint16_t green, uint16_t blue);
void set_landscape();
void set_portrait();

extern int display_width;
extern int display_height;

// === Special coordinates constants ===
#define CENTER	-9003
#define RIGHT	-9004
#define BOTTOM	-9004

#define LASTX	7000
#define LASTY	8000
typedef struct {
  unsigned int   width;
  unsigned int   height;
  unsigned int   bytes_per_pixel; /* 2:RGB16, 3:RGB, 4:RGBA */ 
} image_header;
void draw_image(image_header *im, uint16_t x, uint16_t y);
void draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t colour);
void draw_triangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2 ,uint16_t colour);

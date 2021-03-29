#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <graphics.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// st7789v driver for TTGO T-Display used in 159236, 
// based on spi_master code in the espressif examples 
// Martin Johnson 2020

#define NELEMS(x)  (sizeof(x) / sizeof((x)[0]))

#define U_CMD ((void *)0)
#define U_DATA ((void *)1)

#define USE_POLLING 0

int display_width_offset=40;
int display_height_offset=53;
/*
 The LCD needs a bunch of command/argument values to be initialized. They are
 stored in this struct.
*/
typedef struct {
    uint8_t cmd;
    uint8_t data[16];
    uint8_t databytes;  // No of data in data; bit 7 = delay after set; 0xFF =
                        // end of cmds.
} lcd_init_cmd_t;

spi_device_handle_t spi_device;

// Place data into DRAM. Constant data gets placed into DROM by default, which
// is not accessible by DMA.
DRAM_ATTR static const lcd_init_cmd_t st_init_cmds[] = {
    /* Memory Data Access Control, MX=MV=1, MY=ML=MH=0, RGB=0 */
    /* Lansdcape mode */
    {ST7789_MADCTL, {TFT_MAD_MX | TFT_MAD_MV}, 1}, 
    /* Interface Pixel Format, 16bits/pixel for RGB/MCU interface */
    {ST7789_COLMOD, {0x55}, 1},
    /* Porch Setting */
    {ST7789_PORCTRL, {0x0c, 0x0c, 0x00, 0x33, 0x33}, 5},
    /* Gate Control, Vgh=13.65V, Vgl=-10.43V */
    {ST7789_GCTRL, {0x45}, 1},
    /* VCOM Setting, VCOM=1.175V */
    {ST7789_VCOMS, {0x2B}, 1},
    /* LCM Control, XOR: BGR, MX, MH */
    {ST7789_LCMCTRL, {0x2C}, 1},
    /* VDV and VRH Command Enable, enable=1 */
    {ST7789_VDVVRHEN, {0x01, 0xff}, 2},
    /* VRH Set, Vap=4.4+... */
    {ST7789_VRHS, {0x11}, 1},
    /* VDV Set, VDV=0 */
    {ST7789_VDVSET, {0x20}, 1},
    /* Frame Rate Control, 60Hz, inversion=0 */
    {ST7789_FRCTR2, {0x0f}, 1},
    /* Power Control 1, AVDD=6.8V, AVCL=-4.8V, VDDS=2.3V */
    {ST7789_PWCTRL1, {0xA4, 0xA1}, 2},
    /* Positive Voltage Gamma Control */
    {ST7789_PVGAMCTRL,
     {0xD0, 0x00, 0x05, 0x0E, 0x15, 0x0D, 0x37, 0x43, 0x47, 0x09, 0x15, 0x12,
      0x16, 0x19},
     14},
    /* Negative Voltage Gamma Control */
    {ST7789_NVGAMCTRL,
     {0xD0, 0x00, 0x05, 0x0D, 0x0C, 0x06, 0x2D, 0x44, 0x40, 0x0E, 0x1C, 0x18,
      0x16, 0x19},
     14},
    /* Invert On */
    {ST7789_INVON, {0}, 0x80},
    /* Little Endian */
    {ST7789_RAMCTRL,{0,0xf8},2},
    /* Sleep Out */
    {ST7789_SLPOUT, {0}, 0x80},
    /* Display On */
    {ST7789_DISPON, {0}, 0x80},
    {0, {0}, 0xff}};

void lcd_cmd(const uint8_t cmd, const uint8_t *data, int len) {
    esp_err_t ret;
    spi_transaction_t t[2] = {{.length = 8, .tx_buffer = &cmd, .user = U_CMD},
                {.length = len * 8, .tx_buffer = data, .user = U_DATA}};
    ret = spi_device_transmit(spi_device, &t[0]); 
    assert(ret == ESP_OK); 
    if (len == 0) return;                      
    ret = spi_device_transmit(spi_device, &t[1]);
    assert(ret == ESP_OK); 
}
// This function is called (in irq context!) just before a transmission starts.
// It will set the D/C line to the value indicated in the user field.
void lcd_spi_pre_transfer_callback(spi_transaction_t *t) {
    int dc = (int)t->user;
    gpio_set_level(PIN_NUM_DC, dc);
}

// Initialize the display
void lcd_init() {
    int cmd = 0;
    const lcd_init_cmd_t *lcd_init_cmds;

    esp_err_t ret;
    spi_bus_config_t buscfg = {.miso_io_num = PIN_NUM_MISO,
                               .mosi_io_num = PIN_NUM_MOSI,
                               .sclk_io_num = PIN_NUM_CLK,
                               .quadwp_io_num = -1,
                               .quadhd_io_num = -1,
                               .max_transfer_sz = display_height * display_width * 2 + 8};
    // this does overclock the st7789v but some of the displays even work at 80MHz
    // so it should be OK, if not, change the 40 to 26.
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 40 * 1000 * 1000,  // Clock out at 40MHz can only be a divisor of 80MHz
        .mode = 0,                   // SPI mode 0
        .spics_io_num = PIN_NUM_CS,  // CS pin
        .queue_size = 8,  // We want to be able to queue 8 transactions at a time
        .pre_cb = lcd_spi_pre_transfer_callback,  // Specify pre-transfer
                                                  // callback to handle D/C line
        .flags=SPI_DEVICE_NO_DUMMY | SPI_DEVICE_HALFDUPLEX
    };
    // Initialize the SPI bus
    ret = spi_bus_initialize(HSPI_HOST, &buscfg, 1);
    ESP_ERROR_CHECK(ret);
    // Attach the LCD to the SPI bus
    ret = spi_bus_add_device(HSPI_HOST, &devcfg, &spi_device);
    ESP_ERROR_CHECK(ret);
    // Initialize the LCD

    // Initialize non-SPI GPIOs
    gpio_set_direction(PIN_NUM_DC, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_NUM_RST, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_NUM_BCKL, GPIO_MODE_OUTPUT);

    // Reset the display
    gpio_set_level(PIN_NUM_RST, 0);
    vTaskDelay(100 / portTICK_RATE_MS);
    gpio_set_level(PIN_NUM_RST, 1);
    vTaskDelay(100 / portTICK_RATE_MS);

    lcd_init_cmds = st_init_cmds;

    // Send all the commands
    while (lcd_init_cmds[cmd].databytes != 0xff) {
        lcd_cmd(lcd_init_cmds[cmd].cmd, lcd_init_cmds[cmd].data,
                 lcd_init_cmds[cmd].databytes & 0x1F);
        if (lcd_init_cmds[cmd].databytes & 0x80) {
            vTaskDelay(100 / portTICK_RATE_MS);
        }
        cmd++;
    }
    /// Enable backlight
    gpio_set_level(PIN_NUM_BCKL, 1);
}

/* To send a set of lines we have to send a command, 4 data bytes, another
 * command, 4 more data bytes and another command before sending the line data
 * itself; a total of 6 transactions. (We can't put all of this in just one
 * transaction because the D/C line needs to be toggled in the middle.) This
 * routine queues these commands up as interrupt transactions so they get sent
 * faster (compared to calling spi_device_transmit several times), and meanwhile
 * the lines for next transactions can get calculated.
 */

int frame_sent=0;

void send_frame() {
    esp_err_t ret;
    int x;
    // Transaction descriptors. Declared static so they're not allocated on the
    // stack; we need this memory even when this function is finished because
    // the SPI driver needs access to it even while we're already calculating
    // the next line.
    static spi_transaction_t trans[6]={
        {.tx_data={ST7789_CASET},.length=8,.user=U_CMD, .flags=SPI_TRANS_USE_TXDATA},
        {.length=32,.user=U_DATA, .flags=SPI_TRANS_USE_TXDATA},
        {.tx_data={ST7789_RASET},.length=8,.user=U_CMD, .flags=SPI_TRANS_USE_TXDATA},
        {.length=32,.user=U_DATA, .flags=SPI_TRANS_USE_TXDATA},
        {.tx_data={ST7789_RAMWR},.length=8,.user=U_CMD, .flags=SPI_TRANS_USE_TXDATA},
        {.length=240 * 2 * 8 * 135,.user=U_DATA, .flags=0}
        };
    trans[1].tx_data[1]=display_width_offset;
    trans[1].tx_data[2]=(display_width_offset + display_width - 1) >> 8;
    trans[1].tx_data[3]=(display_width_offset + display_width - 1) & 0xff;
    trans[3].tx_data[1]=display_height_offset;
    trans[3].tx_data[2]=(display_height_offset + display_height - 1) >> 8;
    trans[3].tx_data[3]=(display_height_offset + display_height - 1) & 0xff;
    trans[5].tx_buffer = frame_buffer;         

    // Queue all transactions.
    for (x = 0; x < NELEMS(trans); x++) {
        if(!USE_POLLING)
            ret = spi_device_queue_trans(spi_device, &trans[x], portMAX_DELAY);
        else
            ret = spi_device_polling_transmit(spi_device, &trans[x]);
        assert(ret == ESP_OK);
    }
    frame_sent=1;
    if(frame_buffer==fb1) frame_buffer=fb2;
    else frame_buffer=fb1;
}

void wait_frame() {
    if(!frame_sent)
        return;
    spi_transaction_t *rtrans;
    esp_err_t ret;
    // Wait for all 6 transactions to be done and get back the results.
    if(!USE_POLLING)
        for (int x = 0; x < 6; x++) {
            ret = spi_device_get_trans_result(spi_device, &rtrans, portMAX_DELAY);
            assert(ret == ESP_OK);
        }
    frame_sent=0;
}
int get_orientation() {
    return display_width<display_height;
}
void set_orientation(int portrait) {
    wait_frame();
    if(portrait) {
        lcd_cmd(ST7789_MADCTL, (uint8_t[1]){0}, 1);
        display_width=135;
        display_height=240;
        display_width_offset=52;
        display_height_offset=40;
    } else {
        lcd_cmd(ST7789_MADCTL, (uint8_t[1]){TFT_MAD_MX | TFT_MAD_MV}, 1);
        display_width=240;
        display_height=135;
        display_width_offset=40;
        display_height_offset=53;
    }
}

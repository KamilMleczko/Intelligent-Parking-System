
#include "driver/i2c_master.h"

//command for oled communication datasheet(pages 28-32) https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf
#define CONTROL_BYTE_CMD_SINGLE    0x80 //single command byte  
#define CONTROL_BYTE_CMD_STREAM    0x00 //Command Stream 	
#define CONTROL_BYTE_DATA_SINGLE   0xC0 //Single Data byte
#define CONTROL_BYTE_DATA_STREAM   0x40 //Data Stream	

// Fundamental commands 
#define CMD_SET_CONTRAST           0x81    // set contrast value from 0 to 255
#define CMD_DISPLAY_RAM            0xA4    // resume ram content display
#define CMD_DISPLAY_NORMAL         0xA6
#define CMD_DISPLAY_OFF            0xAE
#define CMD_DISPLAY_ON             0xAF

// Addressing Command Table 
#define CMD_SET_MEMORY_ADDR_MODE   0x20	//follow by either horizontal or vertical addressing mode
#define CMD_SET_HORI_ADDR_MODE     0x00    // Horizontal Addressing Mode
#define CMD_SET_PAGE_ADDR_MODE     0x02    // Page Addressing Mode
 
// Hardware Config 
#define CMD_SET_DISPLAY_START_LINE 0x40   //sets start line from 0 to 63
#define CMD_SET_SEGMENT_REMAP_0    0xA0    //A0 for flipped
#define CMD_SET_SEGMENT_REMAP_1    0xA1    //A1 for normal display
#define CMD_SET_MUX_RATIO          0xA8    // after command pass in 0x3F for 64 display
#define CMD_SET_COM_SCAN_MODE      0xC8    
#define CMD_SET_DISPLAY_OFFSET     0xD3    // vertical shift follow with 0x00
#define CMD_SET_COM_PIN_MAP        0xDA    //Set COM pins hardware configuration
#define CMD_NOP                    0xE3    // Command for no operation
// Timing & Driving Scheme
#define CMD_SET_DISPLAY_CLK_DIV    0xD5    //Set Display Clock divide and oscilator frequency, follow with 0x80  to set proper values
												
#define CMD_SET_PRECHARGE          0xD9    // follow with 0xF1
#define CMD_SET_VCOMH_DESELCT      0xDB    // follow with 0x30
#define CMD_SET_CHARGE_PUMP        0x8D    // follow with 0x14
#define CMD_DEACTIVE_SCROLL        0x2E
//Scrolling
#define CMD_ACTIVE_SCROLL          0x2F



#define I2C_ADDRESS 0x3C
//strcuts declarations here
typedef struct { //information about OLED pins, clock and display
    uint8_t i2c_addr;          // I2C address of display 
    i2c_port_t i2c_port;       // I2C port number
    int16_t scl_gpio;          // SCL GPIO number
    int16_t sda_gpio;          // SDA GPIO number
    int16_t reset_gpio;         // Reset GPIO number (-1 if not used)
    uint32_t clock_speed;      // I2C clock frequency
    int display_width;     // Display width in pixels
    int display_height;    // Display height in pixels
    int ticks_to_wait;     //ticks to wait
} ssd1306_config_t;   // Maximum ticks to wait before issuing a timeout.

typedef struct {
	uint8_t segment[128];//128 bajtów, każdy reprezentuje 8 bitów , czyli 128 * 8 = 1024 pikseli  (1/8 ekranu oleda)
} page_t;

typedef struct{
	int pages_num;
    bool flip_display;
    page_t pages[8]; //8 * 1024 = 64 * 128  = 8192 pikseli (cały ekran oleda)
}oled_display_t;

typedef struct{
    i2c_master_bus_handle_t i2c_mst_bus_handle;
    i2c_master_dev_handle_t i2c_mst_dev_handle;
}i2c_handler_t;


#ifdef __cplusplus
extern "C"
{
#endif
//function declarations here
ssd1306_config_t create_config(void);
void i2c_master_init(const ssd1306_config_t* config, i2c_handler_t* handler, oled_display_t* oled_display);
void oled_cmd_init(oled_display_t* oled_display, const ssd1306_config_t* config, i2c_handler_t* handler);
void clear_oled_display_struct(oled_display_t * oled_display);
void flip_buffer(uint8_t *buffer, size_t size);
uint8_t byte_rotation(uint8_t ch1) ;
void clear_screen(const ssd1306_config_t* config, oled_display_t* oled_display, i2c_handler_t* handler);
void clear_page(const ssd1306_config_t* config, oled_display_t* oled_display, i2c_handler_t* handler, int page);
void delete_line(const ssd1306_config_t* config, oled_display_t* oled_display, i2c_handler_t* handler, int page, char* text);
void show_text(const ssd1306_config_t* config, oled_display_t* oled_display, i2c_handler_t* handler, int page, char* text);
void display_bit_pattern(const ssd1306_config_t* config, oled_display_t* oled_display, i2c_handler_t* handler, int page, int col_start, uint8_t* bit_pattern, int width);
void show_text_large(const ssd1306_config_t* config, oled_display_t* oled_display, i2c_handler_t* handler, int page, char * text);
void clear_large_page(const ssd1306_config_t* config, oled_display_t* oled_display, i2c_handler_t* handler, int page);
void set_brightness(const ssd1306_config_t* config, i2c_handler_t* handler, int value);
#ifdef __cplusplus
}
#endif


#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "my_ssd1306.h"



#if CONFIG_I2C_PORT_0  //provide configuration via sdkconfig file (ssd1306 configuration) //default options should work
#define I2C_NUM I2C_NUM_0
#elif CONFIG_I2C_PORT_1
#define I2C_NUM I2C_NUM_1
#endif

#define TAG "SSD1306"
#define I2C_MASTER_FREQ_HZ 400000 
#define I2C_TICKS_TO_WAIT 100	 
#define I2C_ADDRESS 0x3C



//do not change configs here !!! instead change them in sdkconfig file
ssd1306_config_t create_config(void) {
    return (ssd1306_config_t){
       	.i2c_port = I2C_NUM,           //this is library only for i2c 128x64 oled display
    	.sda_gpio = CONFIG_SDA_GPIO,
    	.scl_gpio = CONFIG_SCL_GPIO,
    	.reset_gpio = CONFIG_RESET_GPIO,
    	.display_width = 128,
    	.display_height = 64,
    	.clock_speed = I2C_MASTER_FREQ_HZ,
    	.i2c_addr = I2C_ADDRESS,
    	.ticks_to_wait = I2C_TICKS_TO_WAIT
    };
}

void i2c_master_init(const ssd1306_config_t* config, i2c_handler_t* handler, oled_display_t* oled_display) {
    ESP_LOGI(TAG, "i2c master init started");

    //master bus config
    i2c_master_bus_config_t i2c_mst_config = {
		.clk_source = I2C_CLK_SRC_DEFAULT,//  Default I2C source clock. 
		.glitch_ignore_cnt = 7, //typically value is 7.
		.i2c_port = config->i2c_port,
		.scl_io_num = config->scl_gpio,
		.sda_io_num = config->sda_gpio,
		.flags.enable_internal_pullup = true,
	};
	
	//master -> device config
    i2c_device_config_t mst_dev_cfg = {
		.dev_addr_length = I2C_ADDR_BIT_LEN_7,
		.device_address = config->i2c_addr,
		.scl_speed_hz = config->clock_speed,
	};


    i2c_master_bus_handle_t bus_handle;
    esp_err_t ret = i2c_new_master_bus(&i2c_mst_config, &bus_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2C master bus: %s", esp_err_to_name(ret));
        return;
    }
    
	i2c_master_dev_handle_t mst_dev_handle;
    esp_err_t ret2 = i2c_master_bus_add_device(bus_handle, &mst_dev_cfg, &mst_dev_handle);
    if (ret2 != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add device to I2C bus: %s", esp_err_to_name(ret2));
        return;
    }

    //if no errors occured save to i2c_handler struct
    handler->i2c_mst_bus_handle = bus_handle;
    handler->i2c_mst_dev_handle = mst_dev_handle;

    //reset back to default state
    if (config->reset_gpio >= 0) {
		gpio_reset_pin(config->reset_gpio); 
		gpio_set_direction(config->reset_gpio, GPIO_MODE_OUTPUT);
		gpio_set_level(config->reset_gpio, 0);
		vTaskDelay(50 / portTICK_PERIOD_MS); // 50 ms - minimal time specified in datasheet
		gpio_set_level(config->reset_gpio, 1);
	}


	oled_display->flip_display = false;
	oled_display->pages_num = 8; //for 64 display
	ESP_LOGI(TAG, "i2c master init succesfully completed");
}

void oled_cmd_init(oled_display_t* oled_display, const ssd1306_config_t* config, i2c_handler_t* handler) { //init oled commands
    uint8_t stream_buf[27]; 
	int idx = 0;
	stream_buf[idx++] = CONTROL_BYTE_CMD_STREAM;
	stream_buf[idx++] = CMD_DISPLAY_OFF;				
	stream_buf[idx++] = CMD_SET_MUX_RATIO;	//multiplexing
    stream_buf[idx++] = 0x3F; //0x3F for 64 high display
    stream_buf[idx++] = CMD_SET_DISPLAY_OFFSET;	
	stream_buf[idx++] = 0x00; //no offset                    
    stream_buf[idx++] = CMD_SET_DISPLAY_START_LINE;	//40~7F we zero out all meaningful bits(last 6) for start line at 0 (0x40)
    if (oled_display->flip_display) {
		stream_buf[idx++] = CMD_SET_SEGMENT_REMAP_0; // 0xA0 for flipped
	} else {
		stream_buf[idx++] = CMD_SET_SEGMENT_REMAP_1;	//0xA1 for normal dsiplay
	}
    stream_buf[idx++] = CMD_SET_COM_SCAN_MODE;		// 0xC8 for scan direction
	stream_buf[idx++] = CMD_SET_DISPLAY_CLK_DIV;		// 0xD5
	stream_buf[idx++] = 0x80; // follow with 0x80 =1000 0000 . 0000 nibble for divide ratio=1 // and 1000 nibble for oscilator freequency 
	stream_buf[idx++] = CMD_SET_COM_PIN_MAP;  
    stream_buf[idx++] = 0x12; // to disable com left/right remap we set 5,4th bits to 1,0
    stream_buf[idx++] = CMD_SET_CONTRAST;			
	stream_buf[idx++] = 0xFF;  //follow with 0xFF for 255 (maximal) brightness as default
	stream_buf[idx++] = CMD_DISPLAY_RAM;				// A4 for resume ram content display
	stream_buf[idx++] = CMD_SET_VCOMH_DESELCT;		// set vcc level
	stream_buf[idx++] = 0x40;  //value = 6,5,4 bits
	stream_buf[idx++] = CMD_SET_MEMORY_ADDR_MODE;	// 20 followe up with addresing mode definition
    stream_buf[idx++] = CMD_SET_PAGE_ADDR_MODE;		// 02 for Page Addressing Mode (only 0,1 bits count)
    stream_buf[idx++] = 0x00; // set lower column start address
    stream_buf[idx++] = 0x10;  // set higheer column start address
	stream_buf[idx++] = CMD_SET_CHARGE_PUMP;			// 8D
	stream_buf[idx++] = 0x14; // 0x14 to enable charge pump
	stream_buf[idx++] = CMD_DEACTIVE_SCROLL;			// 2E
	stream_buf[idx++] = CMD_DISPLAY_NORMAL;			// A6 for normal
	stream_buf[idx++] = CMD_DISPLAY_ON;				// AE for display on (AF for off)

    esp_err_t ret;
	ret = i2c_master_transmit(handler->i2c_mst_dev_handle, stream_buf, idx, config->ticks_to_wait);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Could not write to device, %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "OLED commands sent successfully");
}



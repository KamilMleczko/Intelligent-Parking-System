

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "my_ssd1306.h" 
#include "font8x8_basic.h" //wybrany styl fontu
#define TAG "SSD1306"



void clear_oled_display_struct(oled_display_t* oled_display)
{
	for (int i=0; i < oled_display->pages_num; i++) { //setting pixel value to 0 on entire oled dsiplay struct
		memset(oled_display->pages[i].segment, 0, 128);
	}
}

void clear_screen(const ssd1306_config_t* config, oled_display_t* oled_display, i2c_handler_t* handler){
    char empty[16];
	memset(empty, 0x00, sizeof(empty)); //set all bits to 0 in empty array
    for (int page = 0; page < oled_display->pages_num; page++) {
        delete_line(config, oled_display, handler,  page, empty);
	}
}
void clear_page(const ssd1306_config_t* config, oled_display_t* oled_display, i2c_handler_t* handler, int page)
{
	if (page >= oled_display->pages_num || page < 0 ){
		ESP_LOGE(TAG, "Invalid page number %d. Page number for large page should be in range [0,7] ", page);
		return;
	} 
	char empty[16];
	memset(empty, 0, sizeof(empty));
	delete_line(config, oled_display, handler, page, empty);
}

void clear_large_page(const ssd1306_config_t* config, oled_display_t* oled_display, i2c_handler_t* handler, int page)
{
	if (page > 4 || page < 0 ){
		ESP_LOGE(TAG, "Invalid page number %d. Page number for large page should be in range [0,4] ", page);
		return;
	} 
    char empty[16];
	memset(empty, 0, sizeof(empty)); //set all bits to 0 in empty array
    int lower_limit = page+3;
    for (int page_ = page; page_ < lower_limit; page_++) {
        delete_line(config, oled_display, handler,  page_, empty);
	}
}


void show_text(const ssd1306_config_t* config, oled_display_t* oled_display, i2c_handler_t* handler, int page, char* text){
    if (page >=  oled_display->pages_num || page < 0) {
        ESP_LOGE(TAG, "Invalid page %d (should be in range [0, 7]", page);
        return; 
    }
    // 
    int text_len = strlen(text);
    if (text_len > 16) { //16 is max text length in one line (128px/8px_font = 16) 
		ESP_LOGW(TAG, "Size of text was too large %d, it was cut down to maximum size = 16", text_len);
		text_len = 16;
	}
	int col_start = 0; //idx of last column in segment
	uint8_t bit_pattern[8];
	for (int i = 0; i < text_len; i++) {
		memcpy(bit_pattern, font8x8_basic_tr[(uint8_t)text[i]], 8); //lookup table for ASCII characters to bit representations
		if (oled_display->flip_display){
			flip_buffer(bit_pattern, 8);
		} 
		display_bit_pattern(config, oled_display, handler, page, col_start, bit_pattern, 8);
		col_start = col_start + 8; //go to next segment
	}    
}

void delete_line(const ssd1306_config_t* config, oled_display_t* oled_display, i2c_handler_t* handler, int page, char* text){
    if (page >=  oled_display->pages_num || page < 0 ) {
        ESP_LOGE(TAG, "Invalid page %d (should be in range [0, 7])", page);
        return; //16 is max text length in one line (128px/8px_font = 16) 
    } 
	int col_start = 0; //idx of last column in segment
	uint8_t bit_pattern[8];
	for (int i = 0; i < 16; i++) {
		memcpy(bit_pattern, font8x8_basic_tr[(uint8_t)text[i]], 8); //lookup table for ASCII characters to bit representations
		if (oled_display->flip_display){
			flip_buffer(bit_pattern, 8);
		} 
		display_bit_pattern(config, oled_display, handler, page, col_start, bit_pattern, 8);
		col_start = col_start + 8; //go to next segment
	}    
}

void display_bit_pattern(const ssd1306_config_t* config, oled_display_t* oled_display, i2c_handler_t* handler, int page, int col_start, uint8_t* bit_pattern, int width) {
   
    if (page >= oled_display->pages_num || col_start >= config->display_width) {
        ESP_LOGE(TAG, "Invalid page %d (valid range [0, %d]) or column start %d (valid range [0, %d])", page, oled_display->pages_num - 1,  col_start, config->display_width-1);
        return;
    }

	//we need to split 8 bit number into two nibbles (4 bits each) to conform to the SSD1306's command interface.
	int col = col_start;
	uint8_t lower_nibble = col & 0x0F; ///00001111 //lower nibble last 4 bits of [0,128] pixel range
	uint8_t higher_nibble = (col >> 4) & 0x0F; // to extract first 4 significant bits

	int adjusted_page = page;
    //adjustment for flipped display - bottom-to-top iteration
	if (oled_display->flip_display) {
        int page_number = oled_display->pages_num;
		adjusted_page = page_number - page - 1;
	}

	uint8_t *stream_buf;
	stream_buf = malloc(width < 4 ? 4 : width + 1);
	if (stream_buf == NULL) {
		ESP_LOGE(TAG, "Memory allocation error: Failed to allocate buffer in display_bit_pattern().");
		return;
	}
    //Addressing Setting Command Table (str30)
	stream_buf[0] = CONTROL_BYTE_CMD_STREAM; // information about incoming command stream 
	stream_buf[1] = 0x00 + lower_nibble;  //00~0F is range of lower nibble 
	stream_buf[2] = 0x10 + higher_nibble; //10~1F is range of higher nibblee
	stream_buf[3] = 0xB0 | adjusted_page; // B0~B7 is range for page adressing mode 

	esp_err_t ret;
	ret = i2c_master_transmit(handler->i2c_mst_dev_handle, stream_buf, 4,  config->ticks_to_wait);
	if (ret != ESP_OK){
		ESP_LOGE(TAG, "I2C transmission error: Failed to write data stream to OLED. Error: %s", esp_err_to_name(ret));
        free(stream_buf);
        return;
    }

    stream_buf[0] = CONTROL_BYTE_DATA_STREAM; //information about incoming data stream
	memcpy(&stream_buf[1], bit_pattern, width); //data stream

    esp_err_t ret2;
	ret2 = i2c_master_transmit(handler->i2c_mst_dev_handle, stream_buf, width + 1, config->ticks_to_wait);
	if (ret2 != ESP_OK)
		ESP_LOGE(TAG, "Could not write to device %s", esp_err_to_name(ret2));
	free(stream_buf);

    memcpy(&oled_display->pages[page].segment[col_start], bit_pattern, width);
}

void show_text_large(const ssd1306_config_t* config, oled_display_t* oled_display, i2c_handler_t* handler, int page, char *text) {  
	// the font is 3x larger so max characters in a row is 5 now
	int _text_len = strlen(text);
    if (_text_len > 5) {
		ESP_LOGW(TAG, "Size of text was too large %d, it was cut down to maximum size = 5", _text_len);
        _text_len = 5;
    }
	if (page >= oled_display->pages_num){
		return;
	}
	
    int col_start = 0;
    for (int char_index = 0; char_index < _text_len; char_index++) {
        const uint8_t* character_bitmap = font8x8_basic_tr[(uint8_t)text[char_index]];
        uint32_t expanded_columns[8] = {0}; //same as memset

        //expand the character vertically by 3x
        for (int col_index = 0; col_index < 8; col_index++) { 
            uint32_t input_mask = 0x01;  //input bits (font8x8_basic_tr)
            uint32_t output_mask = 0x07; //expanded output (3 bits per input bit)

            for (int row = 0; row < 8; row++) { // uterate over each bit (y-direction)
                if (character_bitmap[col_index] & input_mask) {
                    expanded_columns[col_index] |= output_mask; 
                }
                input_mask <<= 1;  // move to the next input bit
                output_mask <<= 3; // move to the next set of 3 output bits
            }
        }

        // render the expanded bit pattern in groups of 8 pixels high
        for (int group = 0; group < 3; group++) { 
            uint8_t display_buffer[24]; 

            for (int col = 0; col < 8; col++) {
                uint8_t extracted_byte = (expanded_columns[col] >> (group * 8)) & 0xFF; // Extract one byte 
                //repeat the byte for 3x horizontal scaling
                display_buffer[col * 3 + 0] = extracted_byte;
                display_buffer[col * 3 + 1] = extracted_byte;
                display_buffer[col * 3 + 2] = extracted_byte;
            }
            if (oled_display->flip_display) {
                flip_buffer(display_buffer, 24);
            }

            display_bit_pattern(config, oled_display, handler, page + group, col_start, display_buffer, 24);
        }

        //move to the next segment for the next character
        col_start += 24;
    }
}

void set_brightness(const ssd1306_config_t* config, i2c_handler_t* handler, int value) {
    if (value < 0x0 || value > 0xFF){
        ESP_LOGE(TAG, "Contrast %d out of range, should be between 0 and 255", value);
        return;
    }
    uint8_t _contrast = value;
	uint8_t stream_buf[3]; 
	stream_buf[0] = CONTROL_BYTE_CMD_STREAM;
	stream_buf[1] = CMD_SET_CONTRAST; 
	stream_buf[2] = _contrast;

	esp_err_t ret = i2c_master_transmit(handler->i2c_mst_dev_handle, stream_buf, 3, config->ticks_to_wait);
	if (ret != ESP_OK){
        ESP_LOGE(TAG, "Could not write to device. %s",esp_err_to_name(ret));
    }
		
}

void flip_buffer(uint8_t *buffer, size_t size)
{
	for(int i=0; i < size; i++){
		buffer[i] = byte_rotation(buffer[i]);
	}
}


uint8_t byte_rotation(uint8_t ch1) {
	uint8_t ch2 = 0;
	for (int j=0;j<8;j++) {
		ch2 = (ch2 << 1) + (ch1 & 0x01);
		ch1 = ch1 >> 1;
	}
	return ch2;
}



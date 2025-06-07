#include "sg90.h"
#include "esp_task_wdt.h"

static const char TAG[8] = "pwm_tag";


 void servo_init(void)
{
    
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LEDC_FREQUENCY,  
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = 26,
        .duty           = 0,
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}
 void servo_swerve(void)
{
    static int servo_duty = 600;
   
    {
        while(servo_duty <= 1000)
        {
            servo_duty++;
            //ESP_LOGI(TAG,"Duty is %d",servo_duty);
            ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, servo_duty);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
            vTaskDelay(10/ portTICK_PERIOD_MS);  // Small delay
        }
        
        while(servo_duty >= 200)
        {
            
            servo_duty--;
              ESP_LOGI(TAG,"Duty is %d",servo_duty);
            ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, servo_duty);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
            ESP_LOGI(TAG, "180-0 sweep");
            vTaskDelay(10/ portTICK_PERIOD_MS);  // Small delay
        }
    }
}


 void servo_pos(int pos)
{
	
	if(pos==1)
	{
	ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 205);
	ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);	
	}
	else if (pos==2) 
	{
	ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 242);
	ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);		
	}
	else if (pos==3) 
	{
	ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 284);
	ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);		
	}else if (pos==4) 
	{
	ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 326);
	ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);		
	}else if (pos==5) 
	{
	ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 368);
	ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);		
	}else if (pos==6) 
	{
	ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 410);
	ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);		
	}else if (pos==7) 
	{
	ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 452);
	ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);		
	}else if (pos==8) 
	{
	ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 494);
	ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);		
	}else if (pos==9) 
	{
	ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 536);
	ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);		
	}else if (pos==10) 
	{
	ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 578);
	ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);		
	} 
	else if (pos==11) 
	{
	ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 600);
	ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);		
	} else if (pos==12) 
	{
	ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 662);
	ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);		
	} else if (pos==13) 
	{
	ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 704);
	ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);		
	} else if (pos==14) 
	{
	ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 746);
	ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);		
	} else if (pos==15) 
	{
	ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 788);
	ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);		
	} else if (pos==16) 
	{
	ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 830);
	ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);		
	} else if (pos==17) 
	{
	ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 872);
	ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);		
	} else if (pos==18) 
	{
	ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 914);
	ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);		
	} else if (pos==19) 
	{
	ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 956);
	ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);	
	}else if (pos==20) 
	{
	ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 1000);
	ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);		
	} 
	else
	{
		ESP_LOGI(TAG,"Invalid position");
	}

}

void servo_open_gate(void)
{
	servo_init();
    const int start_duty = 605;
    const int mid_duty = 1000;
    int duty = start_duty;
    int timeout_counter = 0;
    const int MAX_TIMEOUT = 500; // Maximum iterations to prevent infinite loops

    // Move from start to 90°
    ESP_LOGI(TAG, "OPENING GATE");
    for (; duty <= mid_duty && timeout_counter < MAX_TIMEOUT; duty++) {
        ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL);
        vTaskDelay(pdMS_TO_TICKS(10));
        timeout_counter++;
    }
    
    // Force final position regardless of loop completion
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, mid_duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL);
    
    if (timeout_counter >= MAX_TIMEOUT) {
        ESP_LOGW(TAG, "Gate opening timed out, forcing to open position");
    } else {
        ESP_LOGI(TAG, "GATE OPENED");
    }
}

void servo_close_gate(void)
{
	servo_init();
    const int start_duty = 605;
    const int mid_duty = 1000;
    int duty = mid_duty;
    int timeout_counter = 0;
    const int MAX_TIMEOUT = 500; // Maximum iterations to prevent infinite loops

    // Move back to initial position
    ESP_LOGI(TAG, "CLOSING GATE");
    for (; duty >= start_duty && timeout_counter < MAX_TIMEOUT; duty--) {
        ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL);
        vTaskDelay(pdMS_TO_TICKS(10));
        timeout_counter++;
    }
    
    // Force final position regardless of loop completion
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, start_duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL);
    
    if (timeout_counter >= MAX_TIMEOUT) {
        ESP_LOGW(TAG, "Gate closing timed out, forcing to closed position");
    } else {
        ESP_LOGI(TAG, "GATE CLOSED");
    }
}
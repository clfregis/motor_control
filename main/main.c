/**
 * Motor Control:
 *
 * Code created by engineer Claudio Regis in 06 of April of 2020, Quarantine day 12.
 * This code contains the microcontroller commands and sensor readings of the motor 
 * project.
 *
 * It consists of two outputs (a LED and a relay), one input switch and a sensor reading
 * using serial communication.
 * 
 * The motor_supervisor_task stays gathering the information of the DHT22 sensor and motor
 * data like continuous operating time, operation time and status. It stores the values in 
 * a buffer every minute or if the motor change its state.
 * 
 * An ISR handler takes care of sending events to the queue whenever an interrupt occurs.
 * Hardware interrupts are attached to the two input pins (NC contact switch and a button)
 * 
 * Interrupts will occur if the NC switch is pressed or released, calling events
 * responsible for starting/stopping the motor and recording the running time of the motor.
 * Also a hardware interrupt will be triggered whenever the button to reset the motor is pressed
 *
 * When the running time of the motor is greater than a sp time, it turns on the LED and stops the motor
 * It changes the variable state to value 2, which means halted. (0 is stopped and 1 is running)
 * 
 * Hopefully, but not mandatory, we will be able to reset the motor online.
 *
**/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "dht11.h"
#include <time.h>
#include <sys/time.h>
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"
#include "esp_sntp.h"

#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "tcpip_adapter.h"
#include "esp_tls.h"
#include "esp_http_client.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "cjson.h"

/**
 * Brief:
 *
 * GPIO status:
 * GPIO4:   data
 * GPIO12:  input DEPRECATED. It is not possible to use pin 12 during bootup since it is connected to flash chip
 * GPIO27:  input
 * GPIO14:  input, pulled up, interrupt from rising edge and falling edge
 * GPIO18:  output
 * GPIO19:  output
 *
 * Connection:
 * Connect GPIO4 with DHT sensor
 * Connect GPIO27 Reset Button
 * Connect GPIO14 with NC switch
 * Connect GPIO18 with relay (Motor)
 * Connect GPIO19 with LED Overflow
 *
 **/


#define GPIO_OUTPUT_IO_0        18  // Motor pin (will control a relay)
#define GPIO_OUTPUT_IO_1        19  // LED pin 
#define GPIO_OUTPUT_PIN_SEL     ((1ULL<<GPIO_OUTPUT_IO_0) | (1ULL<<GPIO_OUTPUT_IO_1))
#define GPIO_INPUT_IO_0         14  // NC Switch pin
#define GPIO_INPUT_IO_1         27  // Reset Button
#define GPIO_INPUT_PIN_SEL      ((1ULL<<GPIO_INPUT_IO_0) | (1ULL<<GPIO_INPUT_IO_1))
#define GPIO_DATA_0             4   // DHT11 Sensor on the final code, try to use a DHT22 library
#define ESP_INTR_FLAG_DEFAULT   0


// Create a global variable of type xQueueHandle, which is the type we need to reference a FreeRTOS queue.
static xQueueHandle gpio_evt_queue = NULL;
#if CONFIG_debug
	static const char *TAG = "LwIP SNTP";
	static const char *TAG_1 = "HTTP Request";
	static const char *TAG_2 = "Hour/Date";
	static const char *TAG_3 = "Queue Creation";
	static const char *TAG_4 = "Update values";
#endif

//==========================
// Function definitions
//==========================
static void update_sntp_time(void);
static void get_last_value(void);
static void get_frontEnd_status(char *motorStatusAddress);
static void get_sp_time(char *motorSPAddress);
void time_sync_notification_cb(struct timeval *tv);
static void obtain_time(time_t *local_now, struct tm *local_timeinfo); // only need this function to check if it is midnight
static void wifi_connection_start(void);
static void wifi_connection_begin(void);
static void wifi_connection_end(void);
esp_err_t _http_event_handler(esp_http_client_event_t *evt);
static void IRAM_ATTR gpio_isr_handler(void* arg);
//==========================
// End Function definitions
//==========================


//==========================
// Variable definitions
//==========================
// Create a global variable of type uint8_t to store temperature and another to store humidity
uint8_t temperature = 0;
uint8_t humidity = 0;
// Global variables to handle the status of the motor
uint8_t motorStatus = 1; // It starts on
time_t runningTime = 0;
time_t continuousRunningTime = 0;

time_t now;
struct tm timeInfo;

char url[62];
char bufferData[60][120];	//  { "T" : 28, "H" : "78", "S" : "ON", "D" : 12314554, "DO" : 1588033061, "CO" : 358 }
							// T=Temperature, H = Humidity, S=Status, D=Date, DO=Daily Operation, CO=Continuous Operation
							// Example
							/*{
								"CO" : 414,
								"D" : 1588033061,
								"DO" : 414,
								"H" : 91,
								"S": 1,
								"T" : 26
							}*/
uint8_t bufferCounter=0;
int size;
char strftime_db[26];
static const char *motorAddress = CONFIG_motorValue;
static const char *firebaseAddress = CONFIG_firebaseAddress;
uint32_t spTimesec = CONFIG_SPTimesec;
uint8_t frontEndStatus = 2;

//==========================
// End Variable definitions
//==========================


//==========================
// Tasks
//==========================

// Task that controls the motor operation, turning it on, off
void motor_control_task(void* arg) {
    uint8_t io_num;
    while(1) {
        // When receives an event on the gpio queue
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            if(io_num==GPIO_INPUT_IO_0 && gpio_get_level(io_num)==1 && motorStatus!=2) {
            	// NC not pressed
                // Turn on the motor
                gpio_set_level(GPIO_OUTPUT_IO_0, 1);
                // Update status
                motorStatus = 1;
            }
            if (io_num==GPIO_INPUT_IO_0 && gpio_get_level(io_num)==0 && motorStatus!=2) {
            	// NC pressed
                // Turn off the motor
                gpio_set_level(GPIO_OUTPUT_IO_0, 0);
                // Update status
                motorStatus = 0;
            }
            if(io_num==GPIO_INPUT_IO_1 && gpio_get_level(io_num)==0 && motorStatus==2 ) {
                // Reset Motor
                gpio_set_level(GPIO_OUTPUT_IO_0, 1);
                // Reset its state
                motorStatus = 1;
                // Turn off Alert LED
                gpio_set_level(GPIO_OUTPUT_IO_1, 0);
                // Reset continuous time operation
                continuousRunningTime=0;
                frontEndStatus=2;
            }
        }
    }
}

// Task that maintain and configure the time and date 
void clock_task(void *arg) {

	while(1){
        // Every other second, check if is midnight
        vTaskDelay(1000/portTICK_RATE_MS);

        obtain_time(&now, &timeInfo);

        // Correct the drift every night at midnight
        if (timeInfo.tm_hour==0 && timeInfo.tm_min==0 && timeInfo.tm_sec==0){
        	#if CONFIG_debug
        		ESP_LOGI(TAG, "Correct the drift, daily");
        	#endif
	        update_sntp_time();
	        time(&now);
	        // Update running time to the server
	        runningTime=0;
	        continuousRunningTime=0;
        }
	}
}

// Task that supervise the motor, time operation and status
void motor_supervisor_task(void *arg) {
    DHT11_init(GPIO_DATA_0);
    uint8_t internalCounter=0;
    uint8_t currentState=1;
    uint8_t lastState=1;

	while(1){

		if (motorStatus==1){
			// Update times
			runningTime += 1;
			continuousRunningTime += 1;
			currentState=motorStatus;
            if (continuousRunningTime>=spTimesec){
                // Turn on the Alert LED
                gpio_set_level(GPIO_OUTPUT_IO_1, 1);
                // Turn off the motor
                gpio_set_level(GPIO_OUTPUT_IO_0, 0);
                // Set status to halted
                motorStatus=2;
                // Reset time counting
                continuousRunningTime = 0;
            }
		}
		else if (motorStatus==0){
			// Clear times
			continuousRunningTime = 0;
			currentState=motorStatus;
		}
		else if (motorStatus==2 && frontEndStatus==1){
			// Turn off the Alert LED
            gpio_set_level(GPIO_OUTPUT_IO_1, 0);
            // Turn on the motor
            gpio_set_level(GPIO_OUTPUT_IO_0, 1);
            // Set status to operating
            motorStatus=1;
		}
    	// Update temperature
   		temperature = DHT11_read().temperature;
    	// Update humidity
    	humidity = DHT11_read().humidity;

    	// Only store data if it is valid and if the motor changed its state
    	if(temperature<=70 && humidity<=100 && (internalCounter==59 || currentState!=lastState)){
    		// Reset internalCounter
    		internalCounter=0;
    		// obtain_time(&now, &timeInfo);
	     	// strftime(strftime_db, sizeof(strftime_db), "%c", &timeInfo);
    		time(&now); // Does not take into consideration the time zone

	        size = sprintf (bufferData[bufferCounter],
	        		"{ \"T\" : %d, \"H\" : %d, \"S\" : %d, \"D\" : %ld, \"DO\" : %ld, \"CO\" : %ld }",
	        		temperature, humidity, motorStatus, now, runningTime, continuousRunningTime);
	        bufferCounter++;
	        lastState=currentState;
	    }

	    internalCounter++;

		// Wait for another update
        vTaskDelay(1000/portTICK_RATE_MS);
	}
}


// for inside a while to update the buffer
void database_task(void *pvParameters){

    // Wait for for the first update
    vTaskDelay(20000/portTICK_RATE_MS);

    while(1){
    	wifi_connection_start();
    	get_frontEnd_status(motorAddress);

    	// bufferCounter is always 1 ahead of the number of data stored in buffer
	    for(int i=0; i<bufferCounter; i++){

	        // ======================================
	        // Start POST request
	        // ======================================

	        // Create url
			sprintf(url, "https://%s/%s.json",firebaseAddress, motorAddress);

	        // Struct which contains the HTTP configuration
	        // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/esp_http_client.html#_CPPv424esp_http_client_config_t
	        esp_http_client_config_t config = {
	        .url = url,
	        .event_handler = _http_event_handler,
	        };

	        // Call the client init passing the config struct to start a HTTP session
	        // It returns a esp_http_client_handle_t that you must use as input to other functions in the interface
	        // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/esp_http_client.html#_CPPv420esp_http_client_initPK24esp_http_client_config_t
	        esp_http_client_handle_t client = esp_http_client_init(&config);

	        esp_http_client_set_method(client, HTTP_METHOD_POST);
	        // client_open will open the connection, write all header strings and return ESP_OK if all went well
	        // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/esp_http_client.html#_CPPv420esp_http_client_open24esp_http_client_handle_ti
	        if (esp_http_client_open(client, size) == ESP_OK) {
	        	#if CONFIG_debug
	        		ESP_LOGI(TAG_1, "Connection opened");
	        	#endif
	            esp_http_client_write(client, bufferData[i], size);
	            // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/esp_http_client.html#_CPPv429esp_http_client_fetch_headers24esp_http_client_handle_t
	            esp_http_client_fetch_headers(client);
	            //ESP_LOGI(TAG_1, "HTTP POST Status = %d, content_length = %d", esp_http_client_get_status_code(client), esp_http_client_get_content_length(client));
	            if (esp_http_client_get_status_code(client)==200){
	            	#if CONFIG_debug
	                	ESP_LOGI(TAG_1, "Message successfuly sent!");
	                #endif
	            }
	            else {
	            	#if CONFIG_debug
	            	   	ESP_LOGI(TAG_1, "Sending message failed!");
	            	#endif
	            }
	            //============
	            // CONFIG_debug, but, in the future, perform a verification that the message was sent
	            // char valor[77];
	            // // Read the stream of data
	            // esp_http_client_read(client, valor, 77);
	            // printf("%s\n",valor);
	            // CONFIG_debug
	            //============

	            esp_http_client_close(client);
	        }
	        else {
	        	#if CONFIG_debug
	        	    ESP_LOGE(TAG_1, "Connection failed");
	        	#endif
	        }
	        // ======================================
	        // End of POST request
	        // ======================================

	        esp_http_client_cleanup(client);
	        vTaskDelay(10/portTICK_RATE_MS);
    	}

    	// Reset bufferCounter
    	bufferCounter=0;
    	// Clean buffer? I don't think so, we overwrite old data

    	get_sp_time(motorAddress);
        wifi_connection_end();

        // Wait for another update: 1 minute
        vTaskDelay(60000/portTICK_RATE_MS);

    }
    
}


//==========================
// End of Tasks
//==========================


void app_main() {
    //=============================
    // GPIO configuration
    //=============================

    gpio_config_t io_conf;

    // Disable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    // Set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    // Bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    // Disable pull-down mode
    io_conf.pull_down_en = 0;
    // Disable pull-up mode
    io_conf.pull_up_en = 0;
    // Configure GPIO with the given settings
    gpio_config(&io_conf);
    // Start the gpio in High value
    gpio_set_level(GPIO_OUTPUT_IO_0, 1);

    // Interrupt in any edge
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    // Bit mask of the pins, use GPIO4 here
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    // Set as input mode    
    io_conf.mode = GPIO_MODE_INPUT;
    // Disable pull-down mode, using external pullup
    io_conf.pull_down_en = 0;
    // Disable pull-up mode, using external pullup
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    //=============================
    // End of GPIO configuration
    //=============================

    wifi_connection_begin();

    //=============================
    // RTC configuration
    //=============================

    // https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
    #ifdef Brazil 
        char *timeZone = "<-03>3";
    #else
        char *timeZone = "CST6CDT,M4.1.0,M10.5.0";
    #endif

    // Set timezone to Region
    setenv("TZ", timeZone, 1);
    // Apply
    tzset();

    obtain_time(&now, &timeInfo);

    // Is time set? If not, tm_year will be (1970 - 1900).
    if (timeInfo.tm_year < (2020 - 1900)) {
    	#if CONFIG_debug
    	    ESP_LOGI(TAG, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
    	#endif
        update_sntp_time();
        time(&now);
        // obtain_time(&now, &timeInfo);
    }

    //=============================
    // End of RTC configuration
    //=============================

    get_last_value();



    /*  So, we will create a queue that can hold a maximum of 10 elements and since it will 
    *   contain integers, we can get the size of an integer in bytes with a call to the 
    *   sizeof function.
    */
    // Create a queue to handle gpio event from ISR
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));

    if(gpio_evt_queue == NULL) {
    	#if CONFIG_debug
    		ESP_LOGE(TAG_3, "Error creating the queue");
    	#endif
    }

    // Install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    // Hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler, (void*) GPIO_INPUT_IO_0);
    gpio_isr_handler_add(GPIO_INPUT_IO_1, gpio_isr_handler, (void*) GPIO_INPUT_IO_1);

    //=============================
    // Tasks initializations
    //=============================

    // Start motor supervisor task
    xTaskCreate(motor_control_task, "motor_control_task", 2048, NULL, 4, NULL);
    // Start motor supervisor task
    xTaskCreate(&motor_supervisor_task, "motor_supervisor_task", 2048, NULL, 3, NULL);
    // Start the database task
    xTaskCreate(&database_task, "database_task", 4096, NULL, 2, NULL);
    // Start clock task
    xTaskCreate(&clock_task, "clock_task", 3072, NULL, 1, NULL);

    //=============================
    // End of Tasks initializations
    //=============================

}

//==========================
// Functions
//==========================

static void get_last_value(void){
	wifi_connection_start();

	#if CONFIG_debug
		ESP_LOGI(TAG_4, "Initializing Update");
	#endif

    // ======================================
    // Start GET request
    // ======================================

    // Get last value from firebase
    // obatin time (now, timeInfo)
    // Check if last value was in current day
    // If so update runningTime variable
    // Else, set it to zero

    // ======================================
    // End of GET request
    // ======================================

	// Connect to firebase, gg
	wifi_connection_end();

}
static void get_sp_time(char *motorSPAddress){

	#if CONFIG_debug
		ESP_LOGI(TAG_4, "Initializing Update");
	#endif

	// ======================================
    // Start GET request
    // ======================================

    // Create url
	sprintf(url, "https://%s/sp_time.json",firebaseAddress);

    // Struct which contains the HTTP configuration
    // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/esp_http_client.html#_CPPv424esp_http_client_config_t
    esp_http_client_config_t config = {
    .url = url,
    .event_handler = _http_event_handler,
    };

    // Call the client init passing the config struct to start a HTTP session
    // It returns a esp_http_client_handle_t that you must use as input to other functions in the interface
    // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/esp_http_client.html#_CPPv420esp_http_client_initPK24esp_http_client_config_t
    esp_http_client_handle_t client = esp_http_client_init(&config);

    // client_open will open the connection, write all header strings and return ESP_OK if all went well
    // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/esp_http_client.html#_CPPv420esp_http_client_open24esp_http_client_handle_ti
    if (esp_http_client_open(client, 0) == ESP_OK) {
    	#if CONFIG_debug
    		ESP_LOGI(TAG_1, "Connection opened");
    	#endif
        // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/esp_http_client.html#_CPPv429esp_http_client_fetch_headers24esp_http_client_handle_t
        esp_http_client_fetch_headers(client);
        //ESP_LOGI(TAG_1, "HTTP POST Status = %d, content_length = %d", esp_http_client_get_status_code(client), esp_http_client_get_content_length(client));
        if (esp_http_client_get_status_code(client)==200){
        	#if CONFIG_debug
            	ESP_LOGI(TAG_1, "Message successfuly sent!");
            #endif
        }
        else {
        	#if CONFIG_debug
        	   	ESP_LOGI(TAG_1, "Sending message failed!");
        	#endif
        }

        char valor[esp_http_client_get_content_length(client)];
        // Read the stream of data
        esp_http_client_read(client, valor, esp_http_client_get_content_length(client));

        cJSON *root = cJSON_Parse(valor);
        spTimesec = cJSON_GetObjectItem(root,motorSPAddress)->valueint;
        //============
        // CONFIG_debug, but, in the future, perform a verification that the message was sent
        // char valor[77];
        // // Read the stream of data
        // esp_http_client_read(client, valor, 77);
        // printf("%s\n",valor);
        // CONFIG_debug
        //============

        esp_http_client_close(client);
    }
    else {
    	#if CONFIG_debug
    	    ESP_LOGE(TAG_1, "Connection failed");
    	#endif
    }
    // ======================================
    // End of GET request
    // ======================================

}

static void get_frontEnd_status(char *motorStatusAddress){

	if (motorStatus == 2){
		// Create url
		sprintf(url, "https://%s/status.json",firebaseAddress);

	    // Struct which contains the HTTP configuration
	    // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/esp_http_client.html#_CPPv424esp_http_client_config_t
	    esp_http_client_config_t config = {
	    .url = url,
	    .event_handler = _http_event_handler,
	    };

	    // Call the client init passing the config struct to start a HTTP session
	    // It returns a esp_http_client_handle_t that you must use as input to other functions in the interface
	    // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/esp_http_client.html#_CPPv420esp_http_client_initPK24esp_http_client_config_t
	    esp_http_client_handle_t client = esp_http_client_init(&config);

	    // client_open will open the connection, write all header strings and return ESP_OK if all went well
	    // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/esp_http_client.html#_CPPv420esp_http_client_open24esp_http_client_handle_ti
	    if (esp_http_client_open(client, 0) == ESP_OK) {
	    	#if CONFIG_debug
	    		ESP_LOGI(TAG_1, "Connection opened");
	    	#endif
	        // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/esp_http_client.html#_CPPv429esp_http_client_fetch_headers24esp_http_client_handle_t
	        esp_http_client_fetch_headers(client);
	        //ESP_LOGI(TAG_1, "HTTP POST Status = %d, content_length = %d", esp_http_client_get_status_code(client), esp_http_client_get_content_length(client));
	        if (esp_http_client_get_status_code(client)==200){
	        	#if CONFIG_debug
	            	ESP_LOGI(TAG_1, "Message successfuly sent!");
	            #endif
	        }
	        else {
	        	#if CONFIG_debug
	        	   	ESP_LOGI(TAG_1, "Sending message failed!");
	        	#endif
	        }

	        char valor[esp_http_client_get_content_length(client)];
	        // Read the stream of data
	        esp_http_client_read(client, valor, esp_http_client_get_content_length(client));

	        cJSON *root = cJSON_Parse(valor);
	        frontEndStatus = cJSON_GetObjectItem(root,motorStatusAddress)->valueint;
	        //============
	        // CONFIG_debug, but, in the future, perform a verification that the message was sent
	        // char valor[77];
	        // // Read the stream of data
	        // esp_http_client_read(client, valor, 77);
	        // printf("%s\n",valor);
	        // CONFIG_debug
	        //============

	        esp_http_client_close(client);
	    }
	    else {
	    	#if CONFIG_debug
	    	    ESP_LOGE(TAG_1, "Connection failed");
	    	#endif
	    }
	}

}


static void update_sntp_time(void) {
	wifi_connection_start();

	#if CONFIG_debug
		ESP_LOGI(TAG, "Initializing SNTP");
	#endif
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_setservername(1, "europe.pool.ntp.org");
    sntp_setservername(2, "uk.pool.ntp.org ");
    sntp_setservername(3, "us.pool.ntp.org");
    sntp_setservername(4, "time1.google.com");
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    sntp_init();

    int retry = 0;
    const int retry_count = 30;
    // Hide in the deep of the documentation, we found that inside sntp_get_sync_status
    // When connected and getting the time information
    // There is a function called adjtime(), which updates the system time
    // Then, when we call time(), it gets the updated system time.
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
    	#if CONFIG_debug
    	    ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
    	#endif
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    sntp_stop();
    wifi_connection_end();
}

void time_sync_notification_cb(struct timeval *tv) {
	#if CONFIG_debug
		ESP_LOGI(TAG, "Notification of a time synchronization event");
	#endif
}

static void obtain_time(time_t *local_now, struct tm *local_timeinfo){
    #if CONFIG_debug
    	// For CONFIG_debug, to show in terminal the update time
		char strftime_buf[64];
	#endif

    // Store in `now` the UNIX timestamp
    time(local_now);
    // Store in `timeInfo` a human readable format including the Time Zone configured in main
    localtime_r(local_now, local_timeinfo);

    #if CONFIG_debug
	    // Print local time (CONFIG_debug Only)
	    strftime(strftime_buf, sizeof(strftime_buf), "%c", local_timeinfo);
	    ESP_LOGI(TAG_2, "The current date/time is: %s", strftime_buf);
	#endif
}


static void wifi_connection_begin(void){
    ESP_ERROR_CHECK( nvs_flash_init() );
    tcpip_adapter_init();
    ESP_ERROR_CHECK( esp_event_loop_create_default() );
}

static void wifi_connection_start(void){
	/* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());
}

static void wifi_connection_end(void){
    ESP_ERROR_CHECK( example_disconnect() );
}

// When hardware interrupts occurs on the pre-determined pin, it calls this functions which inserts and event on the queue
static void IRAM_ATTR gpio_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

esp_err_t _http_event_handler(esp_http_client_event_t *evt) {
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
        	#if CONFIG_debug
                ESP_LOGD(TAG_1, "HTTP_EVENT_ERROR");
            #endif
            break;
        case HTTP_EVENT_ON_CONNECTED:
        	#if CONFIG_debug
                ESP_LOGD(TAG_1, "HTTP_EVENT_ON_CONNECTED");
            #endif
            break;
        case HTTP_EVENT_HEADER_SENT:
        	#if CONFIG_debug
            	ESP_LOGD(TAG_1, "HTTP_EVENT_HEADER_SENT");
            #endif
            break;
        case HTTP_EVENT_ON_HEADER:
        	#if CONFIG_debug
            	ESP_LOGD(TAG_1, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            #endif
            break;
        case HTTP_EVENT_ON_DATA:
        	#if CONFIG_debug
        	    ESP_LOGD(TAG_1, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        	#endif
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // Write out data
                // printf("%.*s", evt->data_len, (char*)evt->data);
            }
            break;
        case HTTP_EVENT_ON_FINISH:
        	#if CONFIG_debug
                ESP_LOGD(TAG_1, "HTTP_EVENT_ON_FINISH");
            #endif
            break;
        case HTTP_EVENT_DISCONNECTED:
        	#if CONFIG_debug
            	ESP_LOGI(TAG_1, "HTTP_EVENT_DISCONNECTED");
            int mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error(evt->data, &mbedtls_err, NULL);
            if (err != 0) {
        	    ESP_LOGI(TAG_1, "Last esp error code: 0x%x", err);
            	ESP_LOGI(TAG_1, "Last mbedtls failure: 0x%x", mbedtls_err);
            }
            #endif
            break;
    }
    return ESP_OK;
}
//==========================
// End of Functions
//==========================

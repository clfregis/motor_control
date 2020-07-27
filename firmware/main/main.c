/**
 * Motor Control:
 *
 * Code created by engineer Claudio Regis in 06 of April of 2020, Quarantine day 12.
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
#include <dht11.h>
#include <time.h>
#include <sys/time.h>
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "nvs_flash.h"
//#include <protocol_examples_common.h>
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

//==========================
// Function definitions
//==========================
/*!
 * @function    bufferUpdate
 * @abstract    Update the buffer with current values
 * @discussion  This function take the values of tempertature, 
 *              humidity, now, runningTime, motorStatus and 
 *              continuousRunningTime and fetch to buffer
 * @result      bufferData with a new value
*/
static void bufferUpdate(void);
/*!
 * @function    update_sntp_time
 * @abstract    Connect to SNTP server and update system time
 * @discussion  This function connects to WiFi, then connects
 *              to a SNTP server, update the system time
 *              then disconnets from WiFi
 * @result      System time synchronized
*/
static void update_sntp_time(void);
/*!
 * @function    get_last_value
 * @abstract    Updates the last operation times for the current day
 * @discussion  This function connects to WiFi, then connects
 *              to a firebase server, take the last update operation
 *              times, compare its timestamp with current time. If
 *              the last value is from the current day, update runningTime
                and continuousRunningTime
 * @result      continuousRunningTime and runningTime updated
*/
static void get_last_value(void);
/*!
 * @function    update_frontEndStatus
 * @abstract    Retrieve the Front End Status
 * @discussion  This function connects to WiFi, then connects
 *              to a firebase server directly on the specific
 *              child that contains the front end status and
 *              store its value on frontEndReset
 * @param       motorStatusAddress      Pointer to motorAddress
 * @result      continuousRunningTime and runningTime updated
*/
static void update_frontEndStatus(char *motorStatusAddress);
/*!
 * @function    get_sp_time
 * @abstract    Retrieve the SP time
 * @discussion  This function connects to WiFi, then connects
 *              to a firebase server directly on the specific
 *              child that contains the SP time and
 *              store its value on spTimesec
 * @param       motorSPAddress      Pointer to motorAddress
 * @result      spTimesec with a new value
*/
static void get_sp_time(char *motorSPAddress);
/*!
 * @function    time_sync_notification_cb
 * @abstract    Notification of time synchronization
 * @discussion  For Debug purposes. Log on screen
 *              when the time is synchronized
 * @param       tv      Pointer to a timeval struct??
 * @result      Logs on screen
*/
void time_sync_notification_cb(struct timeval *tv);

/*!
 * @function    obtain_time
 * @abstract    Get system time 
 * @discussion  Update now and timeInfo variables with 
 *              current system time
 * @param       local_now           pointer to now
 * @param       local_timeinfo      pointers to timeInfo
 * @result      now and timeInfo with updated values
*/
static void obtain_time(time_t *local_now, struct tm *local_timeinfo); // only need this function to check if it is midnight
/*!
 * @function    wifi_connection_start
 * @abstract    Configures the WiFi variables
 * @discussion  This helper function configures Wi-Fi 
 *              or Ethernet, as selected in menuconfig.
 *              Read "Establishing Wi-Fi or Ethernet Connection" 
 *              section in examples/protocols/README.md for
 *              more information about this function.
 * @result      Returns error if configuration fails
*/
static void wifi_connection_start(void);
/*!
 * @function    wifi_connection_begin
 * @abstract    Connect to WiFi in previous configured SSID
 * @discussion  This helper function configures Wi-Fi 
 *              or Ethernet, as selected in menuconfig.
 *              Read "Establishing Wi-Fi or Ethernet Connection" 
 *              section in examples/protocols/README.md for
 *              more information about this function.
 * @result      Returns error if connection fails
*/
static void wifi_connection_begin(void);
/*!
 * @function    wifi_connection_end
 * @abstract    Close the connection from WiFi network
 * @discussion  This helper function configures Wi-Fi 
 *              or Ethernet, as selected in menuconfig.
 *              Read "Establishing Wi-Fi or Ethernet Connection" 
 *              section in examples/protocols/README.md for
 *              more information about this function. 
 * @result      Returns error if it fails
*/
static void wifi_connection_end(void);
/*!
 * @function    _http_event_handler
 * @abstract    Handles HTTP event queue    
 * @discussion  Logs and some stuff based on HTTP event
 * @param       evt           pointer to HTTP event 
 * @result      Log accordingly to the content of event ID
*/
esp_err_t _http_event_handler(esp_http_client_event_t *evt);
/*!
 * @function    gpio_isr_handler
 * @abstract    Handles hardware interrupts   
 * @discussion  Runs every time an interrupt occurs, 
 *              sending events to a queue
 * @param       arg           contains the number of gpio at wich the hardware interrupt happened
 * @result      Send events to gpio event queue
*/
static void IRAM_ATTR gpio_isr_handler(void* arg);
/*!
 * @function    update_daily
 * @abstract    Sends data to database  
 * @discussion  Runs at midnight to update the daily operation time
 * @param       motorDailyAddress      Pointer to motorAddress
 * @result      database with updated daily operation information
*/
static void update_daily(char *motorDailyAddress);
static esp_err_t event_handler(void *ctx, system_event_t *event);
int startsWith(const char *a, const char *b);
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
uint8_t motorStatus = 0; // It starts off
time_t runningTime = 0;
time_t continuousRunningTime = 0;

time_t now;
struct tm timeInfo;

char url[65];
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
char strftime_db[26];
char *motorAddress = CONFIG_motorValue;
char *firebaseAddress = CONFIG_firebaseAddress;
uint32_t spTimesec = CONFIG_SPTimesec;
uint8_t frontEndReset = 0;
uint8_t updateBufferCounter=0;
bool updateDailyFlag = false;
// Create a global variable of type xQueueHandle, which is the type we need to reference a FreeRTOS queue.
static xQueueHandle gpio_evt_queue = NULL;
static EventGroupHandle_t wifi_event_group;
const int CONNECTED_BIT = BIT0;
#if CONFIG_debug
    static const char *TAG = "LwIP SNTP";
    static const char *TAG_1 = "HTTP Request";
    static const char *TAG_2 = "Hour/Date";
    static const char *TAG_3 = "Queue Creation";
    static const char *TAG_4 = "Update values";
    static const char *TAG_DEBUG = "DEBUG";
    // For CONFIG_debug, to show in terminal the update time
    char strftime_buf[64];
#endif

//==========================
// End Variable definitions
//==========================


//==========================
// Tasks
//==========================

// Task that controls the motor operation
// Regarding to the NC switch values, motor status and reset button
void motor_control_task(void* arg) {	
    uint8_t io_num;
    // First we check the state of the button.
    if(gpio_get_level(GPIO_INPUT_IO_0)){
    	// NC not pressed
        // Turn on the motor
    	gpio_set_level(GPIO_OUTPUT_IO_0, 1);
    	// Update status
        motorStatus = 1;
    }
    else {
		// NC pressed
	    // Turn off the motor
    	gpio_set_level(GPIO_OUTPUT_IO_0, 0);
    	// Update status
        motorStatus = 0;
    }
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
            if(io_num==GPIO_INPUT_IO_1 && gpio_get_level(io_num)==0 && motorStatus==2) {
                // Reset Motor to its normal operation
                if(gpio_get_level(GPIO_INPUT_IO_0)==1){
                    // NC not pressed
                    // Turn on the motor
                    gpio_set_level(GPIO_OUTPUT_IO_0, 1);
                    // Update status
                    motorStatus = 1;
                }
                else if(gpio_get_level(GPIO_INPUT_IO_0)==0){
                    // NC pressed
                    // Turn off the motor
                    gpio_set_level(GPIO_OUTPUT_IO_0, 0);
                    // Update status
                    motorStatus = 0;
                }
                // Turn off Alert LED
                gpio_set_level(GPIO_OUTPUT_IO_1, 0);
                // Reset continuous time operation
                continuousRunningTime=0;
                frontEndReset=2;
            }
        }
    }
}

// Task that maintain and configure the time and date 
void clock_task(void *arg) {
    bool flagClock=false;

	while(1){
        // Every ten minutes, check if is midnight
        vTaskDelay(600000/portTICK_RATE_MS);

        obtain_time(&now, &timeInfo);

        // Correct the drift every night
        if (timeInfo.tm_hour==23 && timeInfo.tm_min>=30 && timeInfo.tm_min<=59){
            if (!flagClock){
                flagClock=true;
                updateDailyFlag = true;
                #if CONFIG_debug
                    ESP_LOGI(TAG, "Correct the drift, daily");
                #endif
                update_sntp_time();
                time(&now);
               
            }
        }
        else {
            flagClock=false;
        }
	}
}

// Task that supervise the motor, time operation and status
void motor_supervisor_task(void *arg) {
    DHT11_init(GPIO_DATA_0);
    uint8_t currentState=1;
    uint8_t lastState=1;

	while(1){

        #if CONFIG_debug
            ESP_LOGI(TAG_DEBUG, "Motor Status: %d", motorStatus);
            ESP_LOGI(TAG_DEBUG, "Buffer Counter: %d", bufferCounter);
            ESP_LOGI(TAG_DEBUG, "Update Buffer Counter: %d", updateBufferCounter);
            ESP_LOGI(TAG_DEBUG, "Current State: %d", currentState);
            ESP_LOGI(TAG_DEBUG, "Last State: %d", lastState);
            ESP_LOGI(TAG_DEBUG, "update Flag: %d", updateDailyFlag);
            ESP_LOGI(TAG_DEBUG, "Free Heap Size: %d", esp_get_free_heap_size());
            ESP_LOGI(TAG_DEBUG, "Minimum Heap Size: %d", esp_get_minimum_free_heap_size());
            printf("\n");
        #endif
        

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
		else if (motorStatus==2 && frontEndReset==1){
			// Turn off the Alert LED
            gpio_set_level(GPIO_OUTPUT_IO_1, 0);
            // Turn on the motor
            gpio_set_level(GPIO_OUTPUT_IO_0, 1);
            // Set status to operating
            motorStatus=1;
            // Reset the frontEndReset
            frontEndReset=0;
            //Update frontEndReset to firebase
		}
    	// Update temperature
   		temperature = DHT11_read().temperature;
    	// Update humidity
    	humidity = DHT11_read().humidity;

    	// Only store data if it is valid and if the motor changed its state
    	if(temperature<=70 && humidity<=100 && (updateBufferCounter>=59 || currentState!=lastState)){
    		// Reset updateBufferCounter if it overflowed
            if(updateBufferCounter>=59){
                updateBufferCounter=0;
            }
    		bufferUpdate();
    		lastState=currentState;
	    }

	    updateBufferCounter++;

		// Wait for another update
        vTaskDelay(1000/portTICK_RATE_MS);
	}
}


// for inside a while to update the buffer
void database_task(void *pvParameters){

    // Wait for the first reading
    if(updateBufferCounter==0){
        vTaskDelay(250/portTICK_RATE_MS);
    }

    while(1){
    	// bufferCounter is always 1 ahead of the number of data stored in buffer, so we use < instead of <=
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
	        if (esp_http_client_open(client, strlen(bufferData[i])) == ESP_OK) {
	        	#if CONFIG_debug
	        		ESP_LOGI(TAG_1, "Connection opened");
	        	#endif
	            esp_http_client_write(client, bufferData[i], strlen(bufferData[i]));
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
	            // Read the stream of data
	            // esp_http_client_read(client, valor, 77);
	            // printf("%s\n",valor);
	            // CONFIG_debug
	            //============

	            esp_http_client_close(client);
                esp_http_client_cleanup(client);
	        }
	        else {
	        	#if CONFIG_debug
	        	    ESP_LOGE(TAG_1, "Connection failed");
	        	#endif
	        }
	        // ======================================
	        // End of POST request
	        // ======================================

    	}

    	// Reset bufferCounter
    	bufferCounter=0;
        // Reset updateBufferCounter
        updateBufferCounter=0;
    	// Clean buffer? Don't need to, we overwrite old data

    	get_sp_time(motorAddress);
    	update_frontEndStatus(motorAddress);
    	if (updateDailyFlag){
    		update_daily(motorAddress);
    		updateDailyFlag=false;
    	}

    	//wifi_connection_end();
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
    // Start the gpio in Low value
    gpio_set_level(GPIO_OUTPUT_IO_0, 0);

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
    wifi_connection_start();

    //=============================
    // RTC configuration
    //=============================

    // https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
    #ifdef CONFIG_Brazil 
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

static void bufferUpdate(void){
	
	// obtain_time(&now, &timeInfo);
 	// strftime(strftime_db, sizeof(strftime_db), "%c", &timeInfo);
	time(&now); // Does not take into consideration the time zone

    sprintf(bufferData[bufferCounter],
    		"{ \"T\" : %d, \"H\" : %d, \"S\" : %d, \"D\" : %ld, \"DO\" : %ld, \"CO\" : %ld }",
    		temperature, humidity, motorStatus, now, runningTime, continuousRunningTime);
    bufferCounter++;
}

static void get_last_value(void){


    // ======================================
    // Start GET request
    // ======================================

    // Get last value from firebase
    // obatin time (now, timeInfo)
    // Check if last value was in current day
    // If so update runningTime variable
    // Else, set it to zero

    // Currrently, we are not able to execute
    // the above algorithm, because the structure 
    // of the database.

    // ======================================
    // End of GET request
    // ======================================

}

static void update_daily(char *motorDailyAddress){

	#if CONFIG_debug
		ESP_LOGI(TAG_4, "Updating daily information");
	#endif

	// ======================================
    // Start POST request
    // ======================================

	time(&now); // Does not take into consideration the time zone

	char dailyData[40];

    sprintf(dailyData,"{ \"D\" : %ld, \"DO\" : %ld }", now, runningTime);
    // Create url
	sprintf(url, "https://%s/daily/%s.json",firebaseAddress, motorDailyAddress);

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
    if (esp_http_client_open(client, strlen(dailyData)) == ESP_OK) {
    	#if CONFIG_debug
    		ESP_LOGI(TAG_1, "Connection opened");
    	#endif
        esp_http_client_write(client, dailyData, strlen(dailyData));
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
        // Read the stream of data
        // esp_http_client_read(client, valor, 77);
        // printf("%s\n",valor);
        // CONFIG_debug
        //============

        esp_http_client_close(client);
        esp_http_client_cleanup(client);
    
        runningTime=0;
        continuousRunningTime=0;
    }
    else {
    	#if CONFIG_debug
    	    ESP_LOGE(TAG_1, "Connection failed");
    	#endif
    }
    // ======================================
    // End of POST request
    // ======================================

}

static void get_sp_time(char *motorSPAddress){
	uint32_t lastReading=spTimesec;
	uint32_t currentReading;

	#if CONFIG_debug
		ESP_LOGI(TAG_4, "Check for change in SP time");
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
        // If there is no value on the child, use the default sptime, else, update the value from firebase
        if(startsWith(valor,"null")){
            // Does nothing
        }
        else{
            cJSON *root = cJSON_Parse(valor);
            currentReading = cJSON_GetObjectItem(root,motorSPAddress)->valueint;
            cJSON_Delete(root);
            if(currentReading!=lastReading){
                spTimesec=currentReading;
                continuousRunningTime=0;
            }
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
        esp_http_client_cleanup(client);
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

static void update_frontEndStatus(char *motorStatusAddress){

	if (motorStatus == 2){
		// Create url
		sprintf(url, "https://%s/front_end_reset_status/.json",firebaseAddress);

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
            // If there is no value on the child, use the default sptime, else, update the value from firebase
            if(startsWith(valor,"null")){
                // Does nothing
            }
            else{
                cJSON *root = cJSON_Parse(valor);
	            frontEndReset = cJSON_GetObjectItem(root,motorStatusAddress)->valueint;
	            cJSON_Delete(root);
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
            esp_http_client_cleanup(client);
	    }
	    else {
	    	#if CONFIG_debug
	    	    ESP_LOGE(TAG_1, "Connection failed");
	    	#endif
	    }
	}

}


static void update_sntp_time(void) {
	//wifi_connection_start();
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
    //wifi_connection_end();
}

void time_sync_notification_cb(struct timeval *tv) {
	#if CONFIG_debug
		ESP_LOGI(TAG, "Notification of a time synchronization event");
	#endif
}

static void obtain_time(time_t *local_now, struct tm *local_timeinfo){

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
    // initialize NVS
	ESP_ERROR_CHECK(nvs_flash_init());
	
	// create the event group to handle wifi events
	wifi_event_group = xEventGroupCreate();
		
	// initialize the tcp stack
	tcpip_adapter_init();

	// initialize the wifi event handler
	ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
	
	// initialize the wifi stack in STAtion mode with config in RAM
	wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

	// configure the wifi connection and start the interface
	wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_wifissid,
            .password = CONFIG_wifipass,
            .bssid_set = 0
        },
    };
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

static void wifi_connection_start(void){
    // wait for connection for 30 seconds
	xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, 30000/portTICK_RATE_MS);
	//printf("connected!\n");
	
	// print the local IP address
	tcpip_adapter_ip_info_t ip_info;
	ESP_ERROR_CHECK(tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip_info));
	printf("IP Address:  %s\n", ip4addr_ntoa(&ip_info.ip));
	printf("Subnet mask: %s\n", ip4addr_ntoa(&ip_info.netmask));
	printf("Gateway:     %s\n", ip4addr_ntoa(&ip_info.gw));
}

static void wifi_connection_end(void){
    ESP_ERROR_CHECK(esp_wifi_disconnect());
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

// Wifi event handler
static esp_err_t event_handler(void *ctx, system_event_t *event){
    switch(event->event_id) {
		
    case SYSTEM_EVENT_STA_START:
         ESP_ERROR_CHECK(esp_wifi_connect());
        break;
    
	case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        break;
    
	case SYSTEM_EVENT_STA_DISCONNECTED:
		xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;
    
	default:
        break;
    }
   
	return ESP_OK;
}

int startsWith(const char *a, const char *b){
   if(strncmp(a, b, strlen(b)) == 0) return 1;
   return 0;
}
//==========================
// End of Functions
//==========================

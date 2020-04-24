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
 * The temperature_humidity_task stays gathering the information of the DHT22 sensor
 * every other 2 seconds
 * 
 * An ISR handler takes care of sending events to the queue whenever an interrupt occurs.
 * 
 * Interrupts will occur if the NC switch is pressed or released, calling events
 * responsible for starting/stopping the motor and recording the running time of the motor.
 *
 * When the running time of the motor is greater than a sp time, it turns on the LED.
 *
 * Every second all these information (time of operation, temperature, humidity,
 * motor status and overflow of functioning) will be passed to a database online.
 * 
 * Hopefully, but not mandatory, we will be able to turn on and off the motor online.
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

/**
 * Brief:
 *
 * GPIO status:
 * GPIO18:  output
 * GPIO19:  output
 * GPIO14:  input, pulled up, interrupt from rising edge and falling edge
 * GPIO4:   input
 *
 * Connection:
 * Connect GPIO14 with NC switch
 * Connect GPIO4 with DHT sensor
 * Connect GPIO18 with relay (Motor)
 * Connect GPIO19 with LED
 *
 **/

#define GPIO_OUTPUT_IO_0        18  // Motor pin (will control a relay)
#define GPIO_OUTPUT_IO_1        19  // LED pin 
#define GPIO_OUTPUT_PIN_SEL     ((1ULL<<GPIO_OUTPUT_IO_0) | (1ULL<<GPIO_OUTPUT_IO_1))
#define GPIO_INPUT_IO_0         14  // NC Switch pin
#define GPIO_DATA_0             4   // DHT11 Sensor on the final code, try to use a DHT22 library
#define GPIO_INPUT_PIN_SEL      (1ULL<<GPIO_INPUT_IO_0)
#define ESP_INTR_FLAG_DEFAULT   0


// Create a global variable of type xQueueHandle, which is the type we need to reference a FreeRTOS queue.
static xQueueHandle gpio_evt_queue = NULL;
static const char *TAG = "LwIP SNTP";
static const char *TAG_1 = "HTTP Request";

static const char *motor_address = "motor_3";
static const char *firebase_address = "esp32-66ba5.firebaseio.com";

//==========================
// Function definitions
//==========================
static void update_sntp_time(void);
void time_sync_notification_cb(struct timeval *tv);
static void obtain_time(time_t *local_now, struct tm *local_timeinfo);
static void wifi_connection_start(void);
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
uint8_t motor_status = 1; // It starts on
time_t running_time = 0;
time_t continuous_running_time = 0;

time_t now;
struct tm timeinfo;

//==========================
// End Variable definitions
//==========================


//==========================
// Tasks
//==========================

// Task that controls the motor operation, turning it on, off
void motor_control_task(void* arg) {
    uint32_t io_num;
    while(1) {
        // When receives an event on the gpio queue
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            if(gpio_get_level(io_num)==1) {
            	//printf("Borda de subida\n");
                // Turn on the motor
                gpio_set_level(GPIO_OUTPUT_IO_0, 1);
                motor_status = 1;
            }
            else {
                //printf("Borda de descida\n");
                // Turn off the motor
                gpio_set_level(GPIO_OUTPUT_IO_0, 0);
                motor_status = 0;
            }
        }
    }
}

// Task that gather the information of the temperature and humidity sensor
void temperature_humidity_task(void *arg) {
    DHT11_init(GPIO_DATA_0);

    while(1) {
    	// Update temperature
    	temperature = DHT11_read().temperature;
    	// Update humidity
    	humidity = DHT11_read().humidity;
        // Wait 2 seconds for another update
        vTaskDelay(2000/portTICK_RATE_MS);
    }

}

// Task that maintain and configure the time and date 
void clock_task(void *arg) {
    char strftime_buf_clock[64];

	while(1){
        // Wait an entire day to update the time
        vTaskDelay(86400000/portTICK_RATE_MS);

		ESP_LOGI(TAG, "Correct the drift, daily");
        update_sntp_time();
        obtain_time(&now, &timeinfo);
	}
}

// Task that supervise the motor, time operation and status
// To implement: Create a local buffer that gather information each second and clears it upon uploading to weberver
void motor_supervisor_task(void *arg) {
    //char strftime_buf_motor[26];

	while(1){

		if (motor_status==1){
			//printf("Motor Status            = ON\n");
			// Update the operation time
			running_time += 1;
			continuous_running_time += 1;
		}
		else {
			//printf("Motor Status            = OFF\n");
			continuous_running_time = 0;
		}

		//printf("Running Time            = %ld seconds\n", running_time);
		//printf("Continuous Running Time = %ld seconds\n", continuous_running_time);

        //printf("Temperature             = %d ºC\n", temperature);
        //printf("Humidity                = %d %%\n", humidity);
        //printf("Status code is %d\n", DHT11_read().status);
        //printf("\n");

		// Wait for another update
        vTaskDelay(1000/portTICK_RATE_MS);
	}
}


// for inside a while to update the buffer
void database_task(void *pvParameters){
    char url[62];
    char data[100]; //  { "Temperature" : 28, "Humidity" : "78", "Status" : "ON", "Time" : "14 Apr 2020 08:02:21" }
    int size;
    char strftime_db[26];

    // Wait for for the first update
    vTaskDelay(20000/portTICK_RATE_MS);

    while(1){
        obtain_time(&now, &timeinfo);
        strftime(strftime_db, sizeof(strftime_db), "%c", &timeinfo);

        size = sprintf (data, "{ \"Temperature\" : %d, \"Humidity\" : %d, \"Status\" : %d, \"Time\" : \"%s\" }", temperature, humidity, motor_status, strftime_db);
        sprintf(url, "https://%s/%s.json",firebase_address, motor_address);

        // ======================================
        // Start POST request
        // ======================================

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
            ESP_LOGI(TAG_1, "Connection opened");
            esp_http_client_write(client, data, size);
            // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/esp_http_client.html#_CPPv429esp_http_client_fetch_headers24esp_http_client_handle_t
            esp_http_client_fetch_headers(client);
            //ESP_LOGI(TAG_1, "HTTP POST Status = %d, content_length = %d", esp_http_client_get_status_code(client), esp_http_client_get_content_length(client));
            if (esp_http_client_get_status_code(client)==200){
                ESP_LOGI(TAG_1, "Message successfuly sent!");
            }
            //============
            // Debug, but, in the future, perform a verification that the message was sent
            // char valor[77];
            // // Read the stream of data
            // esp_http_client_read(client, valor, 77);
            // printf("%s\n",valor);
            // Debug
            //============

            esp_http_client_close(client);
        }
        else {
            ESP_LOGE(TAG_1, "Connection failed");
        }
        // ======================================
        // End of POST request
        // ======================================

        esp_http_client_cleanup(client);

        // Wait for another update
        vTaskDelay(600000/portTICK_RATE_MS);

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
    // Enable pull-up mode
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    //=============================
    // End of GPIO configuration
    //=============================

    wifi_connection_start();
    
    //=============================
    // RTC configuration
    //=============================

    // Set timezone to Region
    setenv("TZ", "<-03>3", 1);
    // Apply
    tzset();

    obtain_time(&now, &timeinfo);

    // Is time set? If not, tm_year will be (1970 - 1900).
    if (timeinfo.tm_year < (2020 - 1900)) {
        ESP_LOGI(TAG, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
        update_sntp_time();
        obtain_time(&now, &timeinfo);
    }

    //=============================
    // End of RTC configuration
    //=============================


    /*  So, we will create a queue that can hold a maximum of 10 elements and since it will 
    *   contain integers, we can get the size of an integer in bytes with a call to the 
    *   sizeof function.
    */
    // Create a queue to handle gpio event from ISR
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));

    if(gpio_evt_queue == NULL) {
        printf("Error creating the queue\n");
    }

    // Install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    // Hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler, (void*) GPIO_INPUT_IO_0);

    //=============================
    // Tasks initializations
    //=============================

    // Start motor supervisor task
    xTaskCreate(motor_control_task, "motor_control_task", 2048, NULL, 1, NULL);
    // Start sensor task
    xTaskCreate(&temperature_humidity_task, "temperature_humidity_task", 2048, NULL, 5, NULL);
    // Start clock task
    xTaskCreate(&clock_task, "clock_task", 3072, NULL, 5, NULL);
    // Start motor supervisor task
    xTaskCreate(&motor_supervisor_task, "motor_supervisor_task", 2048, NULL, 5, NULL);
    // Start the database task
    xTaskCreate(&database_task, "database_task", 4096, NULL, 5, NULL);

    //=============================
    // End of Tasks initializations
    //=============================

}

//==========================
// Functions
//==========================
static void update_sntp_time(void) {

    ESP_LOGI(TAG, "Initializing SNTP");
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
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

void time_sync_notification_cb(struct timeval *tv) {
    ESP_LOGI(TAG, "Notification of a time synchronization event");
}

static void obtain_time(time_t *local_now, struct tm *local_timeinfo){
    // For debug, to show in terminal the update time
    char strftime_buf[64];

    // Store in `now` the UNIX timestamp
    time(local_now);
    // Store in `timeinfo` a human readable format
    localtime_r(local_now, local_timeinfo);

    // Print local time (Debug Only)
    strftime(strftime_buf, sizeof(strftime_buf), "%c", local_timeinfo);
    ESP_LOGI(TAG, "The current date/time in Brazil is: %s", strftime_buf);
}

static void wifi_connection_start(void){
    ESP_ERROR_CHECK( nvs_flash_init() );
    tcpip_adapter_init();
    ESP_ERROR_CHECK( esp_event_loop_create_default() );

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
            ESP_LOGD(TAG_1, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG_1, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG_1, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG_1, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG_1, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // Write out data
                // printf("%.*s", evt->data_len, (char*)evt->data);
            }

            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG_1, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG_1, "HTTP_EVENT_DISCONNECTED");
            int mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error(evt->data, &mbedtls_err, NULL);
            if (err != 0) {
                ESP_LOGI(TAG_1, "Last esp error code: 0x%x", err);
                ESP_LOGI(TAG_1, "Last mbedtls failure: 0x%x", mbedtls_err);
            }
            break;
    }
    return ESP_OK;
}
//==========================
// End of Functions
//==========================

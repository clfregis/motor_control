# Motor Control using ESP32 in IDF framework
### Codes to control a motor, get environment information and send it to a webserver (firebase). For this project I'm using ESP32 in IDF framework.

______________

It consists of two outputs (a LED and a relay), one input switch and a sensor reading using serial communication.

The `temperature_humidity_task` stays gathering the information of the DHT22 sensor every other 2 seconds

An ISR handler (`gpio_isr_handler`) takes care of sending events to the queue whenever an interrupt occurs.
 
Interrupts will occur if the NC switch is pressed or released, calling events responsible for starting/stopping the motor and recording the running time of the motor.

The `clock_task` maintain and configure the time and date. This task calls `obtain_time()` daily to correct the drift on time.

The `motor_supervisor_task` is responsible to reunite all information of the motor every other second. In the future it will be responsible to store these values in a local buffer.

In the future a `update_task` will send the buffer (time of operation, temperature, humidity, motor status and overflow of functioning) to a server every 30 seconds. Connecting to WiFi, then connecting to webserver, updating, then disconnecting again. Then, it will clear the buffer **(Yet to be implemented)**.

Currently, the ESP only connects to wifi to update or configure the time, then it disconnects. I think I will try to maintain this, connecting only to send and gather information.

When the running time of the motor is greater than a sp time, it turns on the LED.


## Currently I have implemented the following features

 - GPIO control (reading button and writing to control motor and LED)
 - WiFi connection
 - Updating time from SNTP server
 - Motor and environment gathering information
 - Sending data in a formatted way to firebase database

## Some functions and operations

+ **Updating time locally**

Every time we have to know what time is it, we need to do some calls, as shown below:

1. Call `time()` function.
	+ Description: The time() function is defined in time.h (ctime in C++) header file. This function returns the time since 00:00:00 UTC, January 1, 1970 (Unix timestamp) in seconds. If second is not a null pointer, the returned value is also stored in the object pointed to by second.
	+ Syntax: `time_t time( time_t *second );`
	+ Parameter: This function accepts single parameter second. This parameter is used to set the time_t object which store the time.

	Then, what we do, is defining an universal `time_t` object named `now_clock`, thus, once we need to know what time is it we just call: `time(&now_clock);` Then, now_clock holds time universally.

	However, as stated before, this `now_clock` variable will hold a UNIX timestamp format, which is not human readable. Also, it is the hour in GMT time, thus we need to adjust both things: Transform to human readable format and fix the timezone.


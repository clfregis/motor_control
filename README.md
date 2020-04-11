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

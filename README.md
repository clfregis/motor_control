# Motor Control Project
##### Prototype that controls a motor, get environment information and send it to an online database (firebase).

This project is a turnkey automation of a hopper in a industry. It consists of [hardware](hardware/) development of the board, development of the [firmware](firmware/) that manages the system, and a [web](web/) app that supervise and control the operation.
Basically, the board is able to control the motor, gather temperature and humidity information and send/receive data to/from a firebase database. How the system works is better explained in [firmware](firmware/) section.
______________
## 1. How to get things running

###  1.1. Hardware

First it is necessary to fabricate and mount the board:

1. Send GERBER files provided in [hardware](hardware/GERBER/) section to a PCB factory;
2. Buy components listed in [BOM](hardware/BOM.pdf) files;
3. Solder components in the board;

P.S.: It will be also necessary to buy a 5V power adapter (micro USB) to provide power to the board.

### 1.2. Web

Concurrently with hardware fabrication we can create a firebase project. We chose firebase because of its scalability and the built-in functions such as authentication and hosting.
There are some limitations in free plan, as shown [here](https://firebase.google.com/pricing?authuser=0). For MVP purposes, we will not be even closer to those limitations, but we can upgrade our plan if necessary.  
To create a project we need to have a google account, if you use Gmail, you already have one. Then access this [page](https://console.firebase.google.com/?pli=1), log in and create a `Realtime Database` (ust follows the steps in the tutorials, Google documentation is pretty awesome).
Then, add Firebase to your web app following these [instructions](https://firebase.google.com/docs/web/setup). **Attention!** In step 3, the HTMLs already have the SDKs. However, it is necessary to copy the snippet of our app's Firebase config object in the first line of `/web/js/main.js`.
After deploying the web app, we are ready!

### 1.3. Firmware

After mounting the hardware and deploying the web app, we are ready to build and flash the program in the ESP32. 


#### 1.3.1. Build
1. [Install the Espressif IoT Development Framework](https://github.com/espressif/esp-idf) or [link](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)
2. Make sure that the USB to UART driver of the development board is installed. In our case, the board uses [CP2102](https://www.silabs.com/products/development-tools/software/usb-to-uart-bridge-vcp-drivers)
3. Clone this repo: `git clone ...`
4. Inside the firmware folder, configure: `make menuconfig`
5. Build: `make all`
6. Flash: `make flash`

* Build & flash & monitor: `make flash monitor`

P.S.: If you get an error like: **"make" is not recognized as a internal or external command**. Try to use `idf.py` instead of `make` command.

Note: In step 3 you need to change some config values, for example:

* `Serial flasher config`. 
	+ `Default serial port`: Type the port at which the ESP32 is connected (e.g. COM3). If this option does not appear, just ignore it, the port will be automatically detected. 
	+ `Default baud rate`: Select 921600 for a faster flashing time.
* `Motor Configuration`. 
	+ `Motor Value`: Type the firebase child at which this ESP32 will send its data (e.g. motor_2).
	+ `Firebase address`: Type the firabase address without 'http://' (e.g. name_project.firebaseio.com). 
	+ (Optional) Change the `SP Time in seconds` value (It can be changed later via web app).
	+ `Timezone`: Select the time zone accordingly to your region.
	+ (Optional) `Log prints for Debug`, check it only if you want to see debug logs using the command prompt.
* `Example Connection Configuration`
	+ `Connect using`: Select WiFi.
	+ `WiFi SSID`: Type the SSID of your WiFi station.
	+ `WiFi Password`: Type the password of your WiFi station.

Save the changes and proceed to step 4.

### 1.4. Future Implementations

+ Update the daily and continuous operating times upon booting. Useful if the systems crashes during the day, because when it restarts it resets these values.
+ Every night at midnight, send the daily operating times to the server. Create a page on web app to display these values in bar graph format.
+ Implement a supervisor to reset the system if have problems with WiFi connection.




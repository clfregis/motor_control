# Motor Control using ESP32 in IDF framework
### Codes to control a motor, get environment information and send it to a webserver (firebase). For this project I'm using ESP32 in IDF framework.

______________

This project is a turnkey automation of a hopper in a industry. It consists of [hardware](hardware/) development of the board, development of the [firmware](firmware/) that manages the system, and a [web](web/) app that supervise and control the operation.
Basically, the board is able to control motor, gather temperature and humidity information and send/receive data to/from a firebase database. How the system works is better explained in [firmware](firmware/) section.

## How to get things running

### Hardware

First it is necessary to fabricate and mount the board:

1. Send GERBER files provided in [hardware](hardware/GERBER/) section to a PCB factory;
2. Buy components listed in [BOM](hardware/README.md) files;
3. Solder components in the board;

P.S.: It will be also necessary to buy a 5V font (micro USB) to provide power to the board.

### Web

Concurrently with hardware fabrication we can create a firebase project. We chose firebase because of its scalability, built in functions such as authentication and hosting.
In free plan, we have some limitations, shown [here](https://firebase.google.com/pricing?authuser=0). For MVP purposes, we will not be even closer to these limitations. If necessary, we can upgrade our plan. 
To create a project we need to have a google account, if you use Gmail, you already have one. Then access this [page](https://console.firebase.google.com/?pli=1) and login. Create a `Realtime Database`. The Google documentation is pretty awesome, so just follows the steps in the tutorials.
Then, add Firebase to your web app. Follow these [instructions](https://firebase.google.com/docs/web/setup). In step 3, the HTMLs already have the SDKs. However, it is necessary to copy the snippet of our app's Firebase config object inn the first line of `/web/js/main.js`.
After deploying the web app, we are ready!

### Firmware

After mounting the hardware and deploying the web app, we are ready to build and flash the program in the ESP32. 


#### Build
1. [Install the Espressif IoT Development Framework](https://github.com/espressif/esp-idf) or [link](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)
2. Clone this repo: `git clone ...`
3. Inside the firmware folder, configure: `make menuconfig`
4. Build: `make all`
5. Flash: `make flash`

* Build & flash FAT image: `make flashfatfs`

Note: In step 3 You need to change some config values, for example:

* `Serial flasher config`. 
	+ `Default serial port` type the port at wich the ESP32 is connected (e.g. COM3). 
	+ `Default baud rate` select 921600 for a faster flashing time.
* `Motor Configuration`. 
	+ `Motor Value` type the firebase child at which this ESP32 will send its data (e.g. motor_2).
	+ `Firebase address` type the firabase address without 'http://' (e.g. name_project.firebaseio.com). 
	+ (Optional) Change the `SP Time in seconds` value (It can be changed later via web app).
	+ `Timezone` select the time zone accordingly to your region.
	+ (Optional) `Log prints for Debug`, check it only if you want to check the operation using the command prompt.
* `Example Connection Configuration`
	+ `Connect using`: Select WiFi.
	+ `WiFi SSID`: Type the SSID of your WiFi station.
	+ `WiFi Password`: Type the password of your WiFi station.

Save the changes and proceed to step 4.



Talk about current version features and future implementations
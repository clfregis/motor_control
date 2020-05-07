# Motor Control using ESP32 in IDF framework
### Codes to control a motor, get environment information and send it to a webserver (firebase). For this project I'm using ESP32 in IDF framework.

______________

Well, depending on the project, the firmware development can be the most time consuming and complex task to do.
For this project, we used free RTOS mostly because it was necessary to deliver a robust application. Dealing with real motors, safety is a big concern.
For the microcontroller, we used ESP32, a very very powerful device (Arduinos are ancient technology compared to it). I will not get into too much detail regarding the specifications of the device. In general it has WiFi and Bluetooth connections, it is dual core, have some flash memory (used to store the SSID and Password of WiFi Networks) and can be easily programmed to work in Low Power.
Our application consists of a motor control of a hopper, based on the state of a NC switch. Basically, when the switch is not pressed (digital Low) the motor is running and when the switch is pressed (digital High) the motor stops. The system is also capable of retrieve the temperature and humidity of the environment using a DHT22 sensor. On top of that, we count the operation time of the motor. The operation time, temperature and humidity are stored internally and every minute it is sent to a database (we used firebase from Google).
We use the information of operation time for two things:

1. Stop the motor if it stays running for a long period of time (named SP time). Basically, when the motor runs for a long period of time, it means that the line breaks and material is being spilled outside the hopper.
2. Record the daily operation of the motor to cross check the material used daily.

### Global Variables

The global variables as in an ordinary code can be manipulated by all functions and all tasks. It is not good practice to use global variables in free RTOS, unless you really know what you are doing. The risk of a task change the value of a variable in the middle of execution of other task that also manipulates this same varibale is high. We can use semaphores, queues and other things to maintain our code safe, however, in our case, there is only one task that ever writes to the variable (although many can read from it), then a global variable should not cause a problem.  
The variables, its type and purpose are organized in the table below:

| Variable name         | Type              | For what it is used                                                               |
|-----------------------|-------------------|-----------------------------------------------------------------------------------|
| temperature           | uint8_t           | Store the reading of the temperature                                              |
| humidity              | uint8_t           | Store the reading of the humidity                                                 | 
| motorStatus           | uint8_t           | Store the status of the motor. 1-Running, 2-Halted, 0-Stopped                     |
| runningTime           | time_t            | Store the operation time of the motor since midnight                              |
| continuousRunningTime | time_t            | Store the continuous operating time of the motor since the last time it was off   |
| now                   | time_t            | Store the current time of the system every time it is called by time() function   |
| timeInfo              | struct tm         | Actually it is a struct not a variable, it stores the time in a very detailed<br> way by localtime() function, taking into consideration timezone |
| url[65]               | char              | Store the firebase url to do the HTTP requests                                    |
| bufferData[60][120]   | char              | Store the Data that will be sent to the database, up to 60 different POST request |
| bufferCounter         | uint8_t           | Counter used to do a loop when sending data to database                           |
| size                  | int               | Stores the buffer size to correctly open the connection with the server           | 
| strftime_db[26]       | char              | For debug only, store a string with time in a human readable format               |
| *motorAddress         | static const char | Stores the value of the motor to create the url on connection with the<br>database. It is defined when programming the board on `menuconfig`, thus we pass<br>`CONFIG_motorValue` to it |
| *firebaseAddress      | static const char | Stores the address of the firebase to create the url on connection with the<br>database. It is defined when programming on `menuconfig`, thus we pass<br>`CONFIG_firebaseAddress` to it |
| spTimesec             | uint32_t          | Stores the SP time of the motor. It can be changed by front end or when <br>programming, thus we pass `CONFIG_SPTimesec` to it when programmin the board. |
| frontEndReset         | uint8_t           | Used to reset the motor by front end                                              |
 

### Tasks

In free RTOS, we have tasks, which we can think of as pieces of codes that can run concurrently. The definition of a task is much more complex and I recommend to read the Kolban's book on ESP32 where he explains in details the architecture of a task. One thing to keep in mind is that tasks have priorities (the higher the number, higher is its priority), thus, let's say we have two tasks: A (priority 2) and B (priority 3). If A is running and B is ready to run, the task A will stop and then the task B will run. When B finishes, the task A will resume from where it stopped.
All tasks are shown in the table below.

| Task name             | Priority |Function   | Writes to  | Reads from |
|-----------------------|----------|-----------|------------|------------|
| motor_control_task | 4 | Controls the motor operation, based on the state<br>of NC switch, motor status and reset button|motorStatus, continuousRunningTime and frontEndReset|motorStatus|
| clock_task | 3 | Correct the drift time daily around midnight and<br>reset the operation times|runningTime, continuousRunningTime,<br>now and timeInfo|            |
| motor_supervisor_task | 2 | Every other second reads temperature and humidity, updates the operation times and update the buffer|runningTime, continuousRunningTime,<br>motorStatus, frontEndReset, bufferData<br>size and bufferCounter |motorStatus and frontEndReset |
| database_task | 1 | Every other minute sends the buffer data to database,<br>read SP time modifications in front end and update front end status|url|size, bufferData, spTime and frontEndReset|


### Functions

|Function Name|Discussion|Parameters|Result|
|-------------|----------|----------|------|
|bufferUpdate|Take the values of tempertature,<br>humidity, now, runningTime, motorStatus and<br>continuousRunningTime and fetch to buffer|void|bufferData with a new value|
|update_sntp_time|Connect to SNTP server and update system time|void|System time synchronized with UTC time|
|get_last_value|It takes the last operation times for the current day|void|continuousRunningTime and runningTime updated|
|update_frontEndStatus|Connect to a child on firebase<br>accordingly to the motorAddress and<br>retrieve its status if it changed<br>since last reading|motorStatusAddress pointer to motorAddress|frontEndStatus with a new value|
|get_sp_time|Take the values of tempertature,<br>humidity, now, runningTime, motorStatus and<br>continuousRunningTime and fetch to buffer|void|bufferData with a new value|
|time_sync_notification_cb|Take the values of tempertature,<br>humidity, now, runningTime, motorStatus and<br>continuousRunningTime and fetch to buffer|void|bufferData with a new value|
|obtain_time|Take the values of tempertature,<br>humidity, now, runningTime, motorStatus and<br>continuousRunningTime and fetch to buffer|void|bufferData with a new value|
|wifi_connection_start|Take the values of tempertature,<br>humidity, now, runningTime, motorStatus and<br>continuousRunningTime and fetch to buffer|void|bufferData with a new value|
|wifi_connection_begin|Take the values of tempertature,<br>humidity, now, runningTime, motorStatus and<br>continuousRunningTime and fetch to buffer|void|bufferData with a new value|
|wifi_connection_end|Take the values of tempertature,<br>humidity, now, runningTime, motorStatus and<br>continuousRunningTime and fetch to buffer|void|bufferData with a new value|
|IRAM_ATTR gpio_isr_handler|Take the values of tempertature,<br>humidity, now, runningTime, motorStatus and<br>continuousRunningTime and fetch to buffer|void|bufferData with a new value|



static void get_sp_time(char *motorSPAddress);
void time_sync_notification_cb(struct timeval *tv);
static void obtain_time(time_t *local_now, struct tm *local_timeinfo); // only need this function to check if it is midnight
static void wifi_connection_start(void);
static void wifi_connection_begin(void);
static void wifi_connection_end(void);
esp_err_t _http_event_handler(esp_http_client_event_t *evt);
static void IRAM_ATTR gpio_isr_handler(void* arg);






# Motor Control using ESP32 in IDF framework
### Codes to control a motor, get environment information and send it to a webserver (firebase). For this project I'm using ESP32 in IDF framework.

______________

## 3D Model of the board when mounted

![3D Model](https://github.com/clfregis/motor_control/blob/master/hardware/3DModel.png)

______________

The `hardware` folder has all necessary files to produce and mount the hardware part of the project as can be seen on the table below.

|File/Folder                |Function                                                                         |
|---------------------------|---------------------------------------------------------------------------------|
|GERBER                     |Output files to produce the PCB in the factory.                                  |
|3DModel.png                |3D Model picture of the mounted board.                                           |
|BOM.csv                    |Bill of Materials, all components necessary to build the project.                |
|BOM.pdf                    |Bill of Materials, all components necessary to build the project.                |
|Board 3D.pdf               |3D Model picture of the mounted board. Open in Acrobat Reader.                   |
|Drill.Cam                  |Mechanical positioning and sizes of holes in the board in CAM format.            |
|ESP32-pinout-mapping.png   |Pinout of the development board used in the project.                             |
|GERBER.zip                 |Output files to produce the PCB in the factory. The industries accept .zip files.|
|General_Schematic.pdf      |File that shows how the components are connected to each other in the board.     |
|Layers.Cam                 |Mechanical positioning and sizes of all layers in the board in CAM format.       |
|Mechanical Bot Mirrored.pdf|Mechanical designed with sizes and holes distances.                              |
|Mechanical Bot.pdf         |Mechanical designed with sizes and holes distances.                              |
|Mechanical Top.pdf         |Mechanical designed with sizes and holes distances.                              |	

For this specific project we do not have any special part, no SMD nor difficult components to find and use. We have two options when mounting this board, one is to solder the ESP32 Development Board directly to PCB, other is use female headers. The latter is better because it will be possible to remove the board easily if you want to.
It is important to be aware of what model of ESP 32 Development board to use. This project was designed to be compatible with [DoIT ESP32 DevKit V1](https://docs.platformio.org/en/latest/boards/espressif32/esp32doit-devkit-v1.html) with 30 pins (some development boards have more pins). The pinout is shown below:
![DoIT ESP32 DevKit V1](https://github.com/clfregis/motor_control/blob/master/hardware/ESP32-pinout-mapping.png)


## BOM

|Designator|Comment                    |Description                        |Quantity|Value |Reference Link    |
|----------|---------------------------|-----------------------------------|--------|------|------------------|
|D1        |Diode                      |Rectifier Diode 1N4007             |1       |1N4007|[1N4007]()        |
|L1        |Red LED                    |5mm Red LED                        |1       |      |[5mm Red LED]()   |
|Q1        |Transistor                 |NPN General Purpose Amplifier      |1       |PN2222|[PN2222]()        |
|R1        |Resistor                   |1/4 W Resistor 1K 5%               |1       |1K    |[1K]()            |
|R2, R3, R5|Resistor                   |1/4 W Resistor 10K 5%              |3       |10K   |[10K]()           |
|R4        |Resistor                   |1/4 W Resistor 300R 5%             |1       |300R  |[300R]()          |
|RLY1      |Relay                      |5V Relay                           |1       |      |[5V]()            |
|S1        |Switch                     |Switch Pushbutton                  |1       |      |[Switch]()        |
|T1        |Terminal                   |3-Way Screw Terminal               |1       |      |[Screw Terminal]()|
|T2        |JST                        |JST Male Board Connector           |1       |      |[JST Connector]() |
|T3,T4     |Not in Schematic (Optional)|Female Pin Header 15x2.54mm (Pitch)|2       |      |[Pin Header]()    |
|U1        |ESP32 Dev Kit V1           |DoIT Development board             |1       |      |[ESP32]()         |
|U2        |DHT22                      |Temperature and Humidity Sensor    |1       |      |[DHT22]()         |

## Future Implementations

+ Add [licesing](https://forum.mysensors.org/topic/3096/open-hardware-licensing).
+ Change PTH to SMD components to reduce size.
+ Add LEDs to show operation processes (blink fast when looking for internet connection, blink slow when is working properly, etc).
+ Instead of using DoIT ESP32 DevKit V1 board, put the ESP32 microcontroller directly into PCB.
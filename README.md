# Coffee Percolator Monitor


My final-year project was to add a few sensors to an ordinary coffee percolator. The heating element was controlled using an opto-coupled TRIAC as the actuator. This percolator is then remotely monitored via the Trinity SMART Sight service that transmits and receives instructions via a Sierra Wireless FXT009 GSM modem which has additional custom AWTDA commands to communicate with the Trinity SMART platform.



## Arduino Mega 2560

The microprocessor chosen for this project, because it had 16 ADC ports, which are handy when interfacing various sensors.

## Temperature sensors
For this project it was required to measure the base, steam and coffee pot's temperature.
The Microchip MCP9700 thermistors were used for measuring the base and steam temperatures, while the Melexis MLX90614 infrared themometer was used to measure the coffee pot's temperature.
### Microchip MCP9700 Active Thermistor

This thermistor does not need extra conditioning circuitry. It is literary a ``plug-and-play'' thermistor. 
It is inexpensive (costs ZAR 3.50) and very durable for a TO-92 package (it can operate in temperatures ranging from -40 degrees celsius to 125 degrees celsius). Its accuracy is unfortunately +/- 2 degrees celsius, but for the price one cannot complain.

Mantech.co.za - [Click here](http://www.mantech.co.za/ProductInfo.aspx?Item=14M1688)


Datasheet:[Click here](http://ww1.microchip.com/downloads/en/DeviceDoc/21942e.pdf)


### MLX90614 infrared thermometer

Datasheet:
[Click here](https://www.sparkfun.com/datasheets/Sensors/Temperature/SEN-09570-datasheet-3901090614M005.pdf)


Application notes:
[Click here](http://www.melexis.com/Prodmain.aspx?nID=615)


Where to buy?
- Sparkfun - [Click here](https://www.sparkfun.com/products/9570)
- Netram.co.za - [Click here](http://netram.co.za/978-infrared-thermometer.html)
- Riektron - [Click here](https://www.riecktron.co.za/en/product/1006)

## Actuator

Very inexpensive and easy [circuit](https://github.com/jeanbritz/RemoteCoffee/blob/master/actuator_control_test/actuator-circuit.png) to build.

Components used:

- Fairchild MOC3061 --- Opto-coupled TRIAC driver with built-in zero-crossing circuit ([Buy](http://www.mantech.co.za/ProductInfo.aspx?Item=35M3489)) ([Datasheet](http://www.fairchildsemi.com/ds/MO/MOC3061M.pdf)) ([App note](http://www.fairchildsemi.com/an/AN/AN-3004.pdf))
- NXP BT136-600E --- Main TRIAC ([Buy](http://www.mantech.co.za/ProductInfo.aspx?Item=35M3275)) ([More info](http://www.nxp.com/products/thyristors/4_quadrant_triacs/BT136-600E.html))
- 2N2222 --- NPN Switching Transistor ([Buy](http://www.mantech.co.za/ProductInfo.aspx?Item=72M3626))


This Fairchild MOC3061 opto-coupled TRIAC is used, in conjunction with the BT136-600E TRIAC, to control the percolator using the cycle control method [1].





References:
---
- [1] ON Semiconductor. (2006, Nov) Thyristor Theory and Design Considerations. [Online].
     Available: http://www.onsemi.com/pub_link/Collateral/HBD855-D.pdf
- [2] Philips Semiconductors, "Chapter 6 - Power Control with Thyristors and Triacs."
[Online]. Available: http://www.nxp.com/documents/application_note/APPCHP6.pdf


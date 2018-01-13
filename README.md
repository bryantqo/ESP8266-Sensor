# ESP8266-Sensor
Use ESP8266 and a DS18B20 as a sensor for HASS
Arduino code for the ESP8266 is in the ESP8266 folder.
Sample config for HASS in the Homeassistant folder


## Usage
1. Wire a DS18B20 to the ESP8266. Update ESP8266.ino to set the correct pin on line 37.

2. When the ESP8266 is first powered on it should launch a configuration portal. Select/enter your wifi info. Enter the server name and port for MQTT for the homeassistant server. Set a unique device name. Make note of the name as you will need it below. 

3. Update your configuration.yaml for homeassistant with the sample in the Homeassistant folder. Make sure you set the <Device Name Here> segments to the name of the device from above. Restart your homeassistant service and the device should appear in the sensors.


## Features
* By default we are only reporting on a single sensor. When a temperature change of more than 1 degree celsius or 30 seconds passed since the last report. Additionally the retention flag is set.

* Temperatures are posted to /Home/Temp/<Device Name> in json format.
	* Example: /Home/Temp/TestSensor {"temp":14.06}

* Upon connect the device will post a status of "online" to /Home/Sensors/<Device Name> in json format.
	* Example: /Home/Sensors/Basement1 {"status":"online"}

* The device will also set a last will on the same topic with a status of "offline". Additionally the retention flag is set.
	* Example: /Home/Sensors/Basement1 {"status":"offline"}

#include "Globals.h"

#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library (you most likely already have this in your sketch)
#include "WIFI_Core.h"
#include "EEPROM_Core.h"




WiFiManager wifiManager;
bool shouldSaveConfig = false;


char* mqtt_server_t;
char* mqtt_port_t;
char* device_name_t;



WiFiManagerParameter custom_mqtt_server("server", "controller", mqtt_server_t, 40);
WiFiManagerParameter custom_mqtt_port("port", "8123", mqtt_port_t, 40);

WiFiManagerParameter custom_device_name("device_name", "Sensor 1", device_name_t, 40);



long lastWifi = -999;
long wifiRate = 500;







//void configModeCallback (WiFiManager *myWiFiManager) {
  //Serial.println("Entered config mode");
  //Serial.println(WiFi.softAPIP());
  //Serial.println(myWiFiManager->getConfigPortalSSID());
//}


void saveConfigCallback () {
  Serial.println("Config has changed ... writing");
  shouldSaveConfig = true;
  mqtt_server = String(custom_mqtt_server.getValue());
  mqtt_port = String(custom_mqtt_port.getValue()).toInt();
  device_name = String(custom_device_name.getValue());


  Serial.printf("Saving\n");
  Serial.println(mqtt_server);
  Serial.println(mqtt_port);
  Serial.println(device_name);


  writeValues();
}



void setupWifi()
{
  // put your setup code here, to run once:
  //TODO: Base the name off the device name ... we will need to pull config before this
  //wifiManager.setAPCallback(configModeCallback);
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.setConfigPortalTimeout(300);

  // id/name, placeholder/prompt, default, length
  
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_device_name);

  String wifi = "HASS-8266-" + device_name;
  char buf[50];
  wifi.toCharArray(buf,50);

  if(!force_launch_config)
    wifiManager.autoConnect(buf);
  else
    wifiManager.startConfigPortal(buf);
}



void manuallyStartWifiPortal()
{
  String wifi = "HASS-8266-" + device_name;
  char buf[50];
  wifi.toCharArray(buf,50);

  wifiManager.startConfigPortal(buf);
}


void wifiStep()
{
  long now = millis();
  if(now - lastWifi > wifiRate)
  {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WIFI DISCONNECTED ... reconnecting");
      setupWifi();
    }
  }
}



#include "Globals.h"

#include <OneWire.h>
#include <DallasTemperature.h>

#include "MQTT_Core.h"
#include "WIFI_Core.h"
#include "EEPROM_Core.h"



#define FAKE_A_DISCONNECT false
#define FAKE_DISCONNECT_PROB 0.25





String TopicBase = "/Sensor/";

bool con = false;



#define WIFI_ENABLED true
#define MQTT_ENABLED true
#define SENSORS_ENABLED true
#define SENSORS_TEMPERATURE_ENABLED true







OneWire oneWire(D4);  
DallasTemperature sensors(&oneWire);




























void setupSensors()
{
  if(SENSORS_TEMPERATURE_ENABLED)
  {
    setupTempSensors();
  }
}

void setupTempSensors()
{
  sensors.begin();
  delay(1500);
}

void init_vars()
{
  mqtt_server = "controller";
  mqtt_port = 1883;
  device_name = "UNKNOWN";
  force_launch_config = false;
}

void setup() {
  init_vars();
  Serial.begin(115200);
  readSaved();

  if(WIFI_ENABLED)
    setupWifi();
  
  if(MQTT_ENABLED && WIFI_ENABLED)
    setupMQTT();

  if(SENSORS_ENABLED)
  {
    setupSensors();
  }

  }


void loop() {
  // put your main code here, to run repeatedly:
  if(WIFI_ENABLED)
    wifiStep();
  if(MQTT_ENABLED)
    mqttStep();
  if(SENSORS_ENABLED)
    sensorsStep();
  delay(100);
}





float minReport = 1.0;
float maxReportInterval = 30000;

float lastReportedTemp = -999;
long lastReportedTime = -999;

bool pendingReport = false; 

void reportTemp(float temp)
{
  Serial.print("Reporting temp: ");
  Serial.println(temp);

  
  if(MQTT_ENABLED)
  {
    if(mqttReportTemp(temp))
    {
      pendingReport = false;
    }
    else
    {
      pendingReport = true;
    }
  }
  lastReportedTemp = temp;
  lastReportedTime = millis();
}

long lastSensor = 0;
long sensorReport = 1000;


void sensorsStep()
{
  long now = millis();
  if(SENSORS_TEMPERATURE_ENABLED)
  {
    if(pendingReport)
    {
      Serial.println("We have a pending report!");
      reportTemp(lastReportedTemp);
    }
    
    if(now-lastSensor > sensorReport)
    {
      int dc = sensors.getDeviceCount();
      //Serial.print("Reading from devices: ");
      //Serial.println(dc);
      sensors.requestTemperatures();
      //for(int i = 0; i < dc; i++)
      //{
        float temp = sensors.getTempCByIndex(0);
        //Serial.println(temp);
        if(abs(lastReportedTemp-temp)>minReport)
        {
          reportTemp(temp);
        }
        
        if(now - lastReportedTime > maxReportInterval)
        {
          Serial.println ("Its been a while since we last reported ...");
          reportTemp(temp);
        }
        
  
      lastSensor = now;
    }
  }
  
  
  
}




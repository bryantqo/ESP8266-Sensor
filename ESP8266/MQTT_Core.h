#include <PubSubClient.h>
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <Arduino.h>








void mqtt_callback(char* topic, byte* payload, unsigned int length);
void setupMQTT();
void mqttStep();
bool mqttReportTemp(float temp);

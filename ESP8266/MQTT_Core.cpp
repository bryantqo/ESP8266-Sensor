#include "Globals.h"
#include <Arduino.h>
#include "MQTT_Core.h"
#include "WIFI_Core.h"


WiFiClient espClient;
PubSubClient client(espClient);



long lastMQTT = 0;
long mqttInterval = 5000;




void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is acive low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}



void setupMQTT()
{
  Serial.println("Connecting to " + String(mqtt_server.c_str()) + ":" + mqtt_port);
  client.setServer(mqtt_server.c_str(), mqtt_port);
  client.setCallback(mqtt_callback);

  int connect_cnt = 0, connect_cnt_max = 5;
  while (!client.connected() && connect_cnt < connect_cnt_max) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String randS = String(random(0xffff), HEX);
    Serial.println(randS);
    String clientId = "ESP-" + randS + "-" + device_name;
    Serial.println("Connecting as " + clientId);
    if (client.connect(clientId.c_str(), (String("/Home/Sensors/") + device_name).c_str(), 
    2, true, (String("{\"status\":\"offline\"}").c_str()))) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      connect_cnt++;
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
  

  if(!client.connected())
  {
    manuallyStartWifiPortal();
  }
  else
  {
    client.publish((String("/Home/Sensors/") + device_name).c_str(), 
    (String("{\"status\":\"online\"}").c_str()), true);
  }
  
}


void mqttStep()
{
  client.loop();
  long now = millis();
  if(now - lastMQTT > mqttInterval)
  {
    if(client.connected())
    {
      lastMQTT = now;
    }
    else
    {
      setupMQTT();
    }
    
  }
  
}


bool mqttReportTemp(float temp)
{
  if(client.connected())
    {
      client.publish((String("/Home/Temp/") + device_name).c_str(), 
      (String("{\"temp\":") + temp + "}").c_str(), true);
      return true;
    }
    else
    {
      Serial.println("MQTT Disconnected ... marking our report as pending");
      return false;
    }
}


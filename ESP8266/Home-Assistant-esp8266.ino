#include <EEPROM.h>
#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library (you most likely already have this in your sketch)
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic

#include <OneWire.h>
#include <DallasTemperature.h>
#include <PubSubClient.h>



WiFiManager wifiManager;
bool shouldSaveConfig = false;


//Max sizes in eeprom
#define SETTING_OFFSET 100
#define APP_VERSION 12
#define APP_HEADER "HAE"
#define MQTT_SERVER_SIZE_MAX 40
#define DEVICE_NAME_SIZE_MAX 40
#define FAKE_A_DISCONNECT false
#define FAKE_DISCONNECT_PROB 0.25

//Also serve as default values
String mqtt_server = "controller";
int mqtt_port = 1883;
String device_name = "UNKNOWN";


char* mqtt_server_t;
char* mqtt_port_t;
char* device_name_t;

String TopicBase = "/Sensor/";

bool con = false;

WiFiManagerParameter custom_mqtt_server("server", "controller", mqtt_server_t, 40);
WiFiManagerParameter custom_mqtt_port("port", "8123", mqtt_port_t, 40);

WiFiManagerParameter custom_device_name("device_name", "Sensor 1", device_name_t, 40);


//MQTTClient mqtt;


#define WIFI_ENABLED true
#define MQTT_ENABLED true
#define SENSORS_ENABLED true
#define SENSORS_TEMPERATURE_ENABLED true


bool force_launch_config = false;



WiFiClient espClient;
PubSubClient client(espClient);


OneWire oneWire(D4);  
DallasTemperature sensors(&oneWire);








void readSaved()
{
  int offset = SETTING_OFFSET;
  String App_Header = "";
  App_Header += char(EEPROM.read(offset+0));
  App_Header += char(EEPROM.read(offset+1));
  App_Header += char(EEPROM.read(offset+2));
  
  int App_Version = EEPROM.read(offset+3);
  offset += 4;

  if(App_Header == APP_HEADER)
  {
    Serial.println("App header matched!");
  }
  else
  {
    Serial.println("Read header: " + App_Header);
    //Write defaults
    WriteDefaults();
  }

  if(App_Version == APP_VERSION)
  {
    Serial.println("App version matched!");
  }
  else
  {
    Serial.println(App_Version);
    WriteDefaults();
  }

  String saved_mqtt_server = "";
  
    for (int i = 0; i < MQTT_SERVER_SIZE_MAX; ++i)
    {
      saved_mqtt_server += char(EEPROM.read(offset+i));
    }
    offset += MQTT_SERVER_SIZE_MAX;
    Serial.println("Read server as: " + saved_mqtt_server);
  
  int saved_mqtt_port = EEPROM.read(offset);
  saved_mqtt_port = EEPROM.read(offset+1)<<8|saved_mqtt_port;
  offset+=2;

    Serial.printf("Read port as %d", saved_mqtt_port);
    
  String saved_device_name = "";
  
    for (int i = 0; i < DEVICE_NAME_SIZE_MAX; ++i)
    {
      saved_device_name += char(EEPROM.read(offset+i));
    }
    offset += DEVICE_NAME_SIZE_MAX;
    Serial.println("Read name as: " + saved_device_name);

  //TODO: CRC

  mqtt_server = saved_mqtt_server;
  mqtt_port = saved_mqtt_port;
  device_name = saved_device_name;
}

void writeHeader()
{
  int offset = SETTING_OFFSET;

  for(int i = 0; i < 3; i++)
  {
    Serial.printf("Writing %c to %d\n", APP_HEADER[i], offset+i);
    EEPROM.write(offset+i,APP_HEADER[i]);

    Serial.printf("Read %d from %d\n", char(EEPROM.read(offset+i)));
  }
  offset+=3;
  EEPROM.write(offset,APP_VERSION);
  offset++;

}

void writeServer()
{
  int offset = SETTING_OFFSET + 4;
  char serverBuf[40];
  mqtt_server.toCharArray(serverBuf,40);
  for(int i = 0; i < 40; i++)
    EEPROM.write(offset+i,serverBuf[i]);
}

void writePort()
{
  int offset = SETTING_OFFSET + 4 + MQTT_SERVER_SIZE_MAX;
  
  EEPROM.write(offset,mqtt_port&0xFF);
  offset++;
  EEPROM.write(offset,mqtt_port>>8&0xFF);
  offset++;
  
}

void writeName()
{
    int offset = SETTING_OFFSET + 4 + MQTT_SERVER_SIZE_MAX + 2;

  char deviceBuf[40];
  device_name.toCharArray(deviceBuf,40);
  for(int i = 0; i < 40; i++)
    EEPROM.write(offset+i,deviceBuf[i]);

}

void WriteDefaults()
{
  Serial.println("Writing defaults!");
  writeHeader();
  
  writeServer();
  writeName();
  
  Serial.println("Committing defaults!");
  EEPROM.commit();
  Serial.println("Done!");

  force_launch_config = true;
  
}










void setupWifi()
{
  // put your setup code here, to run once:
  //TODO: Base the name off the device name ... we will need to pull config before this
  wifiManager.setAPCallback(configModeCallback);
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


void setup() {
  Serial.begin(115200);
  EEPROM.begin(512);
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

void configModeCallback (WiFiManager *myWiFiManager) {
  //Serial.println("Entered config mode");
  //Serial.println(WiFi.softAPIP());
  //Serial.println(myWiFiManager->getConfigPortalSSID());
}


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

  
  writeServer();
  writePort();
  writeName();
  EEPROM.commit();
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


long lastWifi = -999;
long wifiRate = 500;
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


long last = 0;
long trip = 5000;

void mqttStep()
{
  client.loop();
  long now = millis();
  if(now - last > trip)
  {
    Serial.println("mqtt_tick");

    if(client.connected())
    {
      last = now;
    }
    else
    {
      setupMQTT();
    }
    
  }
  
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
  bool fakeDisconnect = false;
  if(FAKE_A_DISCONNECT)
  {
    Serial.println("Rolling to see if we fake a disconnect");
    float ran = random(0,100) / 100.0;
    Serial.println(ran);
    if(ran < FAKE_DISCONNECT_PROB)
    {
      Serial.println("Were gonna disconnect!");
      fakeDisconnect = true;
    }
  }


  
  if(MQTT_ENABLED)
  {
    if(client.connected() && !fakeDisconnect)
    {
      client.publish((String("/Home/Temp/") + device_name).c_str(), 
      (String("{\"temp\":") + temp + "}").c_str(), true);
      pendingReport = false;
    }
    else
    {
      Serial.println("MQTT Disconnected ... marking our report as pending");
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




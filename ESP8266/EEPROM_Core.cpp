#include "Globals.h"
#include <EEPROM.h>




#define SETTING_OFFSET 100
#define APP_VERSION 13
#define APP_HEADER "HAE"

//Max sizes in eeprom
#define MQTT_SERVER_SIZE_MAX 40
#define DEVICE_NAME_SIZE_MAX 40

bool eeprom_init_eeprom = false;








String mqtt_server = "controller";
int mqtt_port = 1883;
String device_name = "UNKNOWN";
bool force_launch_config = false;




void init_eeprom()
{
  if(!eeprom_init_eeprom)
  {
    EEPROM.begin(512);

    
    eeprom_init_eeprom = true;
  }
}



void writeHeader()
{
  init_eeprom();
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
  init_eeprom();
  int offset = SETTING_OFFSET + 4;
  char serverBuf[40];
  mqtt_server.toCharArray(serverBuf,40);
  for(int i = 0; i < 40; i++)
    EEPROM.write(offset+i,serverBuf[i]);
}

void writePort()
{
  init_eeprom();
  int offset = SETTING_OFFSET + 4 + MQTT_SERVER_SIZE_MAX;
  
  EEPROM.write(offset,mqtt_port&0xFF);
  offset++;
  EEPROM.write(offset,mqtt_port>>8&0xFF);
  offset++;
  
}

void writeName()
{
  init_eeprom();
  int offset = SETTING_OFFSET + 4 + MQTT_SERVER_SIZE_MAX + 2;

  char deviceBuf[40];
  device_name.toCharArray(deviceBuf,40);
  for(int i = 0; i < 40; i++)
    EEPROM.write(offset+i,deviceBuf[i]);

}

void WriteDefaults()
{
  init_eeprom();
  Serial.println("Writing defaults!");
  writeHeader();
  
  writeServer();
  writeName();
  
  Serial.println("Committing defaults!");
  EEPROM.commit();
  Serial.println("Done!");

  force_launch_config = true;
  
}


void writeValues()
{
  writeServer();
  writePort();
  writeName();
  EEPROM.commit();
}






void readSaved()
{
  init_eeprom();
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


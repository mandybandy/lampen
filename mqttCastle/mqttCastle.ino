/*
    Name:     Mqtt.ino
    Created:  27.02.2019 17:30:03
    Changed:  31.01.2021 18:00:00
    Author:   Mandl-PC\Philipp
*/

#include <PubSubClient.h>
#include <Ethernet.h>
#include <SPI.h>
#include <MCP23S17.h>

//anzahl der hardware definieren
const int anzahleffekte = 6;
const int anzahllampen = 118;
const int anzahlmcps = 8;

//Ethernet daten definieren
byte mac[] = {0x90, 0xA2, 0xDA, 0x10, 0x8D, 0x14};
IPAddress ip(192, 168, 1, 246);
IPAddress server(192, 168, 1, 245);

//bool array für state speicherung
bool lampstate[anzahllampen] = {0};

//var für message input
int length = 0;
int payload;
String topic;
byte *payloadraw;

//Ethernet daten definieren
EthernetClient ethClient;
PubSubClient mqttClient(ethClient);

MCP mcpSelect[anzahlmcps] = {
  MCP(0, 9), //type mcpx(SPI Adresse, SlaveSelect)
  MCP(1, 9),
  MCP(2, 9),
  MCP(3, 9),
  MCP(4, 9),
  MCP(5, 9),
  MCP(6, 9),
  MCP(7, 9)
};



//Funktion wird aufgerufen wenn msg ankommt

void getMqtt(char *topicchar, byte *payloadbyte, unsigned int length)
{
  convert(topicchar, payloadbyte, length);
  //Serial.print("Nachricht angekommen: ");
  //Serial.println(topic);
  //Serial.println(payload);
  payload = constrain(payload, 0, anzahllampen - 1); //für website nur zahlen zwischen 0-119 möglich
  if (topic == "/lc/output/on")
  {
    lampstate[payload] = HIGH;
    render();
  }
  else if (topic == "/lc/output/off")
  {
    lampstate[payload] = LOW;
    render();
  }
  else if (topic == "/lc/output/")
  {
    mqttClient.publish("/status/arduino", "1");
  }
  else if (topic == "/lc/output/effekte")
  {
    payload = constrain(payload, 0, anzahleffekte); //für website nur effekte zwischen 0-6 möglich
    effekte(payload);
  }
  //Serial.println(payload + " ist ");
  //Serial.print(mcpout(detMCP(payload).no).digitalRead(detMCP(payload).pin));
}


//mit mqtt broker als webclient verbinden und subscriben
void reconnectMQTT()
{
  while (!mqttClient.connected())
  {
    if (mqttClient.connect("ArduinoClient", "webclient", "passwort"))
    {
      //Serial.println("connected...");
      mqttClient.subscribe("/lc/#");
      mqttClient.subscribe("/status/#");
      mqttClient.publish("/status/arduino", "1");
    }
    else
    {
      delay(1000);
    }
  }
}

//Funktion die auf mcp schreibt und bestätigung sendet
void render()
{
  for (int payload = 0; payload < anzahllampen; payload++)
  {
    mcpSelect[payload / 16].digitalWrite(payload % 16, lampstate[payload]); //auf MCP schreiben/Relais ansteuern

    if (lampstate[payload])
    {
      mqttClient.publish("/lc/output/ack/on", payloadraw, length);//FIXME: payload statt payloadraw
    } else {
      mqttClient.publish("/lc/output/ack/off", payloadraw, length);
    }
  }
}

//werte konvertieren auf richtigen dateitypen
void convert(char *topicchar, byte * payloadbyte, unsigned int length)
{
  String topic(topicchar);             //topic in string verwandeln
  payloadraw = (byte *)malloc(length); //payload erstellen
  memcpy(payloadraw, payloadbyte, length);
  String value = ""; //payload in int verwandeln für weiterverarbeitung
  for (unsigned int i = 0; i < length; i++)
  {
    value += (char)payloadbyte[i];
  }
  payload = value.toInt();
}

void setup()
{
  Ethernet.begin(mac, ip);
  //Serial.begin(9600);
  //TODO:sicher machen wenns scheat
  mqttClient.setServer(server, 1883);
  mqttClient.setCallback(getMqtt);
  //kommunikation und pinMode festlegen bzw. starten
  for (int i = 0; i < anzahlmcps; i++)
  {
    mcpout(i).begin();
    mcpout(i).pinMode(0B1111111111111111);
  }
}
//mqtt client
void loop()
{
  reconnectMQTT();
  mqttClient.loop();
}

void (*effektSelect[anzahleffekte]) = {
  effekt0,
  effekt1,
  effekt2,
  effekt3,
  effekt4,
  effekt5
};
void effekte(unsigned int payload)
{
  effekt0();
  (*effektSelect[payload])();
  mqttClient.publish("/lc/output/effekte", payloadraw, length); //payloadraw=byte* BESTÄTIGUNG
}
//Effekte
void effekt0() //effekt 0 schaltet alles aus
{
  lampstate[anzahllampen] = {0};
  render();
}

void effekt1()
{
  for (int i = 0; i < anzahllampen; i++)
  {
    i = constrain(i++, 0, anzahllampen - 1);
    lampstate[i++] = HIGH;
    lampstate[i] = LOW;
    effektdelay(1000);
    render();
  }
}

void effekt2()
{
  for (int i = 0; i < anzahllampen; i + 2)
  {
    i = constrain(i, 0, anzahllampen - 1);
    lampstate[i] = HIGH;
  }
  render();
  effektdelay(1000);
  for (int i = 0; i < anzahllampen; i + 2)
  {
    i = constrain(i, 0, anzahllampen - 1);
    lampstate[i] = LOW;
  }
  render();
}
void effekt3()
{

  render();
}

void effekt4()
{

  render();
}

void effekt5()
{

  render();
}

void effektdelay(unsigned long delaytime)
{
  long startmillis = millis();
  while ((millis() - startmillis) < delaytime)
  {
    mqttClient.loop();
  }
}

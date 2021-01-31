/*
    Name:       Mqtt.ino
    Created:  27.02.2019 17:30:03
    Author:     Mandl-PC\Philipp
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

MCP mcp0(0, 9); //type mcpx(SPI Adresse, SlaveSelect)
MCP mcp1(1, 9);
MCP mcp2(2, 9);
MCP mcp3(3, 9);
MCP mcp4(4, 9);
MCP mcp5(5, 9);
MCP mcp6(6, 9);
MCP mcp7(7, 9);

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

struct mcp
{
  int no;
  int pin;
};

//Funktion wird aufgerufen wenn msg ankommt

void getMqtt(char *topicchar, byte *payloadbyte, unsigned int length)
{
  convert(topicchar, payloadbyte, length);
  Serial.print("Nachricht angekommen: ");
  Serial.println(topic);
  Serial.println(payload);
  payload = constrain(payload, 0, anzahllampen - 1); //für website nur zahlen zwischen 0-119 möglich
  if (topic == "/lc/par36/on")
  {
    lampstate[payload] = HIGH;
    render();
  }
  else if (topic == "/lc/par36/off")
  {
    lampstate[payload] = LOW;
    render();
  }
  else if (topic == "/lc/par36/")
  {
    mqttClient.publish("/status/arduino", "1");
  }
  else if (topic == "/lc/par36/effekte")
  {
    payload = constrain(payload, 0, anzahleffekte); //für website nur effekte zwischen 0-6 möglich
    effekte(payload);
  }
  Serial.println(payload + " ist ");
  Serial.print(mcpout(detMCP(payload).no).digitalRead(detMCP(payload).pin));
}

//umrechnung von payload zu mcpx und pin y
mcp detMCP(int lamp)
{
  mcp ret;
  ret.no = lamp / 16;        //mcp
  ret.pin = (lamp % 16) + 1; //pin
  return ret;
}

//mit mqtt broker als webclient verbinden und subscriben
void reconnectMQTT()
{
  while (!mqttClient.connected())
  {
    if (mqttClient.connect("ArduinoClient", "webclient", "passwort"))
    {
      Serial.println("connected...");
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
    if (lampstate[payload])
    {
      mcpout(detMCP(payload).no).digitalWrite(detMCP(payload).pin, HIGH); //auf MCP schreiben/Relais ansteuern
      mqttClient.publish("/lc/par36/ack/on", payloadraw, length);         //payloadraw=byte* BESTÄTIGUNG
    }
    else if (!lampstate[payload])
    {
      mcpout(detMCP(payload).no).digitalWrite(detMCP(payload).pin, LOW);
      mqttClient.publish("/lc/par36/ack/on", payloadraw, length);
    }
  }
}

//werte konvertieren auf richtigen dateitypen
void convert(char *topicchar, byte *payloadbyte, unsigned int length)
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
  Serial.begin(9600);
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
//auswahl Funktionen
//int zu type verwandeln
MCP mcpout(int cur)
{
  switch (cur)
  {
  case 0:
    return mcp0;
  case 1:
    return mcp1;
  case 2:
    return mcp2;
  case 3:
    return mcp3;
  case 4:
    return mcp4;
  case 5:
    return mcp5;
  case 6:
    return mcp6;
  case 7:
    return mcp7;
  default:
    effekt0();
    return mcp0; //egal weil, boolarray lampstate =NULL
  }
}
void effekte(unsigned int payload)
{
  mqttClient.publish("/lc/par36/effekte", payloadraw, length); //payloadraw=byte* BESTÄTIGUNG
  switch (payload)
  {
  case 0:
    effekt0();
    break;
  case 1:
    effekt0();
    effekt1();
    break;
  case 2:
    effekt0();
    effekt2();
    break;
  case 3:
    effekt0();
    effekt3();
    break;
  case 4:
    effekt0();
    effekt4();
    break;
  case 5:
    effekt0();
    effekt5();
    break;
  }
}

/* ignorierts es afoch
  FFFFFFFFFF      A        IIIII   L         EEEEEEEE  DDDDDDD
  F              A A         I     L         E         D      D
  F             A   A        I     L         E         D       D
  FFFFFFFFF    AAAAAAA       I     L         EEEEEEEE  D       D
  F           A       A      I     L         E         D       D
  F          A         A     I     L         E         D      D
  F         A           A  IIIII   LLLLLLLL  EEEEEEEE  DDDDDDD
*/
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
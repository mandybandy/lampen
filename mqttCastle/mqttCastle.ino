/*
    Name:     Mqtt.ino
    Created:  27.02.2019 17:30:03
    Changed:  31.01.2021 18:00:00
    Author:   Mandl-PC\Philipp
*/
/*
https://arduino-projekte.info/https-ssl-seiten-mit-arduino-eps8266-aufrufen/
<WiFiClientSecure.h>
2. Der WiFiClient client; Befehl durch den WiFiClientSecure client; Befehlt ersetzt werden.
3. Der Port muss von 80 auf 443 geändert werden.
4. Man muss dem Arduino / EPS8266 das Sicherheitszertifikat mitteilen. Das macht man mit dem Befehlt client.setFingerprint(fingerprint);*/


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
  Serial.print("Nachricht angekommen: ");
  Serial.println(topic);
  Serial.println(payload);
  payload = constrain(payload, 0, anzahllampen - 1); //für website nur zahlen zwischen 0-119 möglich
  if (topic == "/lc/output/on")
  {
    lampstate[payload] = HIGH;
    render(payload);
  }
  else if (topic == "/lc/output/off")
  {
    lampstate[payload] = LOW;
    render(payload);
  }
  else if (topic == "/lc/output/effekte")
  {
    Serial.println("effekt");
    payload = constrain(payload, 0, anzahleffekte); //für website nur effekte zwischen 0-6 möglich
    char* toSend = malloc(2);
    itoa(payload, toSend, 10);
    mqttClient.publish("/lc/output/ack/effekte", toSend);
    free(toSend);
    effekte();
  }
  else if (topic == "/lc/output/")
  {
    mqttClient.publish("/status/arduino", "1");
  }
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
      Serial.println("connecting...");
      delay(1000);
    }
  }
}

//Funktion die auf mcp schreibt und bestätigung sendet
//spricht nur einen pin an
void render(uint8_t toRender)
{
  char* toSend = malloc(3);
  itoa(toRender, toSend, 10);
  mcpSelect[toRender%16].digitalWrite(toRender%16, lampstate[toRender]);

  if(lampstate[toRender])
    mqttClient.publish("/lc/output/ack/on", toSend);
  else
    mqttClient.publish("/lc/output/ack/off", toSend);
  
  Serial.print("/lc/output/ack/" + lampstate[toRender] ? "on" : "off");
  Serial.println(toSend);
  free(toSend);
}

//spricht alle pins an
void render()
{
  for (uint8_t i = 0; i < anzahllampen; i++)
  {
    mcpSelect[i/16].digitalWrite(i%16, lampstate[i]);
    mqttClient.publish("/lc/output/ack/" + lampstate[i]?"on":"off", i);
  }
}

//werte konvertieren auf richtigen dateitypen
void convert(char *topicchar, byte * payloadbyte, unsigned int length)
{
  payloadraw = (byte *)malloc(length); //payload erstellen
  memcpy(payloadraw, payloadbyte, length);
  String value = ""; //payload in int verwandeln für weiterverarbeitung
  for (unsigned int i = 0; i < length; i++)
  {
    value += (char)payloadbyte[i];
  }
  payload = value.toInt();
  topic = topicchar;
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
    mcpSelect[i].begin();
    mcpSelect[i].pinMode(0B1111111111111111);
  }
}
//mqtt client
void loop()
{
  reconnectMQTT();
  mqttClient.loop();
}

void (*effektSelect[anzahleffekte]) ()= {
  effekt0,
  effekt1,
  effekt2,
  effekt3,
  effekt4,
  effekt5
};

void effekte()
{
  effekt0();
  (*effektSelect[payload])();
  mqttClient.publish("/lc/output/effekte", payload); //payloadraw=byte* BESTÄTIGUNG
}
//Effekte
void effekt0() //effekt 0 schaltet alles aus
{
  lampstate[anzahllampen] = {0};
  render();
}

void effekt1()
{
  Serial.println("effekt1");
  for (int i = 0; i < anzahllampen; i++)
  {
    i = constrain(i+1, 0, anzahllampen - 1);
    lampstate[i+1] = HIGH;
    lampstate[i] = LOW;
    effektdelay(1000);
    render(i);
  }
}

void effekt2()
{
  for (int i = 0; i < anzahllampen; i + 2)
  {
    i = constrain(i, 0, anzahllampen - 1);
    lampstate[i] = HIGH;
    render(i);
  }
  effektdelay(1000);
  for (int i = 0; i < anzahllampen; i + 2)
  {
    i = constrain(i, 0, anzahllampen - 1);
    lampstate[i] = LOW;
    render(i);
  }
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

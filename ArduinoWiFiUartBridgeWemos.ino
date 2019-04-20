// ESP8266 WiFi <-> UART Bridge

#include <ESP8266WiFi.h>


// config: ////////////////////////////////////////////////////////////

#define UART_BAUD 9600
#define packTimeout 5 // ms (UART - Timeout)
#define bufferSize 8192

//#define MODE_AP // ESP Verbindung direkt ohne Router
#define MODE_STA // ESP Verbindung zu WiFi Router

#define PROTOCOL_TCP
//#define PROTOCOL_UDP


#ifdef MODE_AP
// For AP mode:
const char *ssid = ""; 
const char *pw = "";
IPAddress ip(0, 0, 0, 0);
IPAddress netmask(255, 255, 255, 0);
const int port = 4001; //Port für Verbindung
#endif


#ifdef MODE_STA
// For STATION mode:
const char *ssid = "";
const char *pw = "";
const char deviceName[] = "Gardena_Roboter";
IPAddress staticIP(10,0,0,89);
IPAddress gateway(10,0,0,138);
IPAddress netmask(255,255,255,0);
const int port = 4001; //Port für Verbindung
#endif

//////////////////////////////////////////////////////////////////////////




#ifdef PROTOCOL_TCP
#include <WiFiClient.h>
WiFiServer server(port);
WiFiClient client;
#endif

#ifdef PROTOCOL_UDP
#include <WiFiUdp.h>
WiFiUDP udp;
IPAddress remoteIp;
#endif


uint8_t buf1[bufferSize];
uint8_t i1=0;

uint8_t buf2[bufferSize];
uint8_t i2=0;



void setup() {

  delay(500);
  
  Serial.begin(UART_BAUD);
  Serial.println();

  #ifdef MODE_AP 
  //AP mode (ESP Verbindung direkt ohne Router)
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(ip, ip, netmask); 
  WiFi.softAP(ssid, pw);
  #endif

 

  
  #ifdef MODE_STA
  // STATION mode (ESP Verbindet zum Router mit statischer Ip-Adresse)
  WiFi.hostname(deviceName);
  Serial.println();
  WiFi.setAutoConnect(false);
  Serial.printf("Connecting to %s\n", ssid);
  Serial.println("Adresse: " + staticIP.toString());
  Serial.print("Port: ");
  Serial.print(port);
  Serial.println();
  WiFi.begin(ssid, pw);
  WiFi.config(staticIP, gateway, netmask);
  WiFi.mode(WIFI_STA);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
  }
  #endif

  #ifdef PROTOCOL_TCP
  Serial.println("Starting TCP Server");
  server.begin(); // start TCP server 
  #endif

  #ifdef PROTOCOL_UDP
  Serial.println("Starting UDP Server");
  udp.begin(port); // start UDP server 
  #endif
}


void loop() {
  #ifdef PROTOCOL_TCP
  if(!client.connected()) { // wenn keine Verbindung
    client = server.available(); // warte auf Verbindung
    return;
  }

  // here we have a connected client

  if(client.available()) {
    while(client.available()) {
      buf1[i1] = (uint8_t)client.read(); // lese Zeichen von Client (Ipsymcon)
      if(i1<bufferSize-1) i1++;
    }
    // jetzt senden zu UART:
    Serial.write(buf1, i1);
    i1 = 0;
  }

  if(Serial.available()) {

    // lesen Daten bis pause:
    
    while(1) {
      if(Serial.available()) {
        buf2[i2] = (char)Serial.read(); // lese Zeichen von UART
        if(i2<bufferSize-1) i2++;
      } else {
        //delayMicroseconds(packTimeoutMicros);
        delay(packTimeout);
        if(!Serial.available()) {
          break;
        }
      }
    }
    
    // jetzt senden zu WiFi:
    client.write((char*)buf2, i2);
    i2 = 0;
  }
  #endif



  #ifdef PROTOCOL_UDP
  // wenn daten vorhanden
  int packetSize = udp.parsePacket();
  if(packetSize>0) {
    remoteIp = udp.remoteIP(); // speichern der Ip vom entfernten Gerät
    udp.read(buf1, bufferSize);
    // now send to UART:
    Serial.write(buf1, packetSize);
  }

  if(Serial.available()) {

    
    //Serial.println("sa");
    
    while(1) {
      if(Serial.available()) {
        buf2[i2] = (char)Serial.read(); // lese Zeichen von UART
        if(i2<bufferSize-1) {
          i2++;
        }
      } else {
        //delayMicroseconds(packTimeoutMicros);
        //Serial.println("dl");
        delay(packTimeout);
        if(!Serial.available()) {
          //Serial.println("bk");
          break;
        }
      }
    }

    // jetzt senden zu WiFi:  
    udp.beginPacket(remoteIp, port); // entfernter IP and port (IpSymcon)
    udp.write(buf2, i2);
    udp.endPacket();
    i2 = 0;
  }
    
  #endif
  
  
}

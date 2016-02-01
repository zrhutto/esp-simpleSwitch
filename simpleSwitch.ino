#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <RCSwitch.h>
#include "ssid_config.h"

#define DEBUG 0

const char* ssid = mySSID;
const char* password = myPasswd;


ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;
RCSwitch mySwitch = RCSwitch();


void enableRX() {
  if (DEBUG) {
    Serial.print(server.method());
  }
  mySwitch.enableReceive(0);
  server.send(200, "application/json", "{\"status\":\"rxEnabled\"}");
}

void disableRX() {
  mySwitch.disableReceive();
  server.send(200, "application/json", "{\"status\":\"rxDisabled\"}");

}

void handleRX() {
   String codeStr = "{\"code\":";
   if (mySwitch.available()) {
    
    int value = mySwitch.getReceivedValue();
    
    if (value == 0) {
      Serial.print("Unknown encoding");
    } else if (DEBUG) {
      Serial.print("Received ");
      Serial.print( mySwitch.getReceivedValue() );
      Serial.print(" / ");
      Serial.print( mySwitch.getReceivedBitlength() );
      Serial.print("bit ");
      Serial.print("Protocol: ");
      Serial.println( mySwitch.getReceivedProtocol() );
    }
    codeStr += String(value);
    codeStr += "}";
    server.send(200, "application/json", codeStr);
    mySwitch.resetAvailable();
  } else {
    codeStr += "null}";
    server.send(200, "application/json", codeStr);
  }
}

void handleTX() {
  if (server.hasArg("code")) {
    long unsigned int myCode = server.arg("code").toInt();
    mySwitch.send(myCode, 24);
    server.send(200, "application/json", "{\"status\":\"sent\"}");
  } else {
    server.send(500, "application/json", "{\"status\":\"error\"}");
  }
}


void handleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void setup(void){
  if (DEBUG) {
    Serial.begin(115200);
    Serial.println("");
  }
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

    // prepare GPIO2
  pinMode(2, OUTPUT);
  pinMode(0, INPUT);
  // Transmitter is connected to ESP8266 PIN #2
  mySwitch.enableTransmit(2);
  //EtekCity ZAP Remote Outlets use pulse of approximately 189
  mySwitch.setPulseLength(189);
  
  // Wait for connection
  if (DEBUG) {
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    
    if (MDNS.begin("espRC")) {
      Serial.println("MDNS responder started");
    }
    Serial.println("HTTP server started");
  }
  
  httpUpdater.setup(&server);
  server.on("/rc/rx/on", HTTP_POST, enableRX);
  server.on("/rc/rx/off", HTTP_POST, disableRX);
  server.on("/rc/rx", handleRX);
  server.on("/rc/tx", HTTP_POST, handleTX);
  server.on("/", [](){
    server.send(200, "text/plain", "espRC ready");
  });

  server.onNotFound(handleNotFound);

  server.begin();
}

void loop(void){
  server.handleClient();
}

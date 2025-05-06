//Include the webpage that we will be sending to the client as UI
#include "index.h"
//Include the necessary libraries for controlling our servo, running our local server, and setting up our ESP8266 as a local Access Point.
#include <Servo.h>
#include <Arduino.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

//Declare our servo object. Used later to control our servo via PWM
Servo clamp;

//I promise I didn't actually use such an insecure password
const char *ssid = "SpikeControl";
const char *password = "TSATSATSA";

//Configuration information
IPAddress local_IP(200,0,0,1);
IPAddress gateway(192,168,4,9);
IPAddress subnet(255,255,255,0);

//Create the necessary objects to run our server on port 80 and use websockets
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

//When using the Servo object, we pass a value from 0 to 180. Centerspeed determines what the default value will be.
const int centerspeed = 102;
int speed = centerspeed;

//Recommended function from documentation to handle incoming web socket messages
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    //Convert our data from a character array to a String
    String Data;
    for (int i = 0; i < len; i += 1) {
      Data += char(data[i]);
    }
    //The only data that our UI will ever send is a number
    speed = Data.toInt();
    Serial.println(speed);
    //Update our servo with the correct speed
    clamp.write(speed);
    
  }
}

//Recommended function for handling various websocket events. Points to the above function handleWebSocketMessage
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
    switch (type) {
      case WS_EVT_CONNECT:
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
        break;
      case WS_EVT_DISCONNECT:
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
        break;
      case WS_EVT_DATA:
        handleWebSocketMessage(arg, data, len);
        break;
      case WS_EVT_PONG:
      case WS_EVT_ERROR:
        break;
  }
}

//Initialize our websocket
void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

//Necessary processor function for our various libraries
String processor(const String& var){
  Serial.println(var);
  return String();
}

void setup() {
  //Open a serial port for debugging purposes
  Serial.begin(9600);
  Serial.println("testing");
  //Asign a pin to our servo and write the default value to it
  clamp.attach(D4);
  clamp.write(centerspeed);
  //Create our Access Point and web socket server
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(ssid, password);
  initWebSocket();
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", WEBpage, processor);
  });
  server.begin();
}

void loop() {
  //Run our server
  ws.cleanupClients();
}

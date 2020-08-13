#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>

#define GRN D5 
#define BLU D4  
#define BUTTON D7 

String ssid = "";
String password = "";
String mqtt_server = "";
String ssidAP = "";
String passwordAP = "";

ESP8266WebServer server(80);
WiFiClient espClient;
PubSubClient client(espClient);
MDNSResponder MDNS;
int value = 0;
String mess;
String jsonConfig;

String getContentType(String filename){
    if(filename.endsWith(".html")) return "text/html";
    else if(filename.endsWith(".css")) return "text/css";
    else if(filename.endsWith(".js")) return "application/javascript";
    else if(filename.endsWith(".json")) return "application/json";
    return "text/plain";
}

bool handleFileRead(String path){
    Serial.println("handleFileRead: " + path);
    String contentType = getContentType(path);
    if(SPIFFS.exists(path)){
      File file = SPIFFS.open(path, "r");
      size_t sent = server.streamFile(file, contentType);
      file.close();
      return true;
    }
    Serial.println("File not found");
    return false;
}

bool loadConfig() {
    File configFile = SPIFFS.open("/config.json", "r");
    if (!configFile) {
      Serial.println("Failed to open config file");
      saveConfig();
      return false;
    }
    jsonConfig = configFile.readString();
    Serial.println();
    Serial.println(jsonConfig);
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(jsonConfig); 
    ssid = root["ssidName"].as<String>();
    password = root["ssidPassword"].as<String>();
    mqtt_server = root["mqttServerName"].as<String>();
    ssidAP = root["ssidAPName"].as<String>();
    passwordAP = root["ssidAPPassword"].as<String>();
    configFile.close();
    return true;
}

bool saveConfig() {
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.parseObject(jsonConfig);
    json["ssidName"] = ssid;
    json["ssidPassword"] = password;
    json["mqttServerName"] = mqtt_server;
    json["ssidAPName"] = ssidAP;
    json["ssidAPPassword"] = passwordAP;
    json.printTo(jsonConfig);
    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("Failed to open config file for writing");
      return false;
    }
    json.printTo(configFile);
    return true;
}


void handleRoot() {
    handleFileRead("/page.html");
    int temp = server.arg("state").toInt();
    switch(temp) {
    case 1:
         Serial.print("Publish message: ");
         Serial.println("MESSAGE: TRUE");
         value++;
         Serial.println(value);
         client.publish("mqtt/paho/enter", "True");
         break;
    case 2:
        Serial.println("PUBLISH MESSAGE: False");
        value++;
        Serial.println(value);
        client.publish("mqtt/paho/enter", "False");
        break; 
    case 0:
        break;
    }
    server.send(200, "text/plain", "OK");
}


void handleSettingsSTA() {
    handleFileRead("/pageWiFi.html");
    String data = server.arg("plain");
    DynamicJsonBuffer jBuffer;
    JsonObject& jObject = jBuffer.parseObject(data);

    ssid = jObject["ssid"].as<String>();
    password = jObject["password"].as<String>();
    
    saveConfig(); 
    if(ssid == "" || password == "") {
      server.send(200, "text/plain", "No arguments");
    }
    else {
      server.send(200, "text/plain", "OK");
      setup_wifi();
    }
}



void handleSettingsAP() {
    handleFileRead("/pageAP.html");
    String data = server.arg("plain");
    DynamicJsonBuffer jBuffer;
    JsonObject& jObject = jBuffer.parseObject(data);

    ssidAP = jObject["ssid"].as<String>();
    passwordAP = jObject["password"].as<String>();
    
    saveConfig(); 
    if(ssidAP == "" || passwordAP == "") {
      server.send(200, "text / plain", "No arguments");
    }
    else {
      server.send(200, "text / plain", "OK");
     
    }
}

void handleApiData(){
    String message = mess;
    server.send(200, "text/plain", message);
}

void handleNotFound() {
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    for(uint8_t i = 0; i < server.args(); i++) {
      message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    server.send(404, "text/plain", message);
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  Serial.println(password);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());

  while (WiFi.status() != WL_CONNECTED) {
    delay(2000);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}


void callback(char* topic, byte* payload, unsigned int length) {
    digitalWrite(GRN, LOW);
    digitalWrite(BLU, HIGH);
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    mess.clear();
    for (int i = 0; i < length; i++) {
      Serial.print((char)payload[i]);
      mess+=(char)payload[i];
    }
    Serial.println();
    delay(1000);
}

void reconnect() {
    while (!client.connected()) {
      Serial.print("Attempting MQTT connection...");
      String clientId = "ESP8266Client-CodeLock";
      clientId += String(random(0xffff), HEX);
      if (client.connect(clientId.c_str())) {
        Serial.println("connected");
        client.subscribe("mqtt/paho/enter");
      } else {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 10 seconds");
        delay(10000);
      }
    }
}

void setup() {
  pinMode(GRN, OUTPUT);
  pinMode(BLU, OUTPUT);
  pinMode(BUTTON, INPUT);
  Serial.begin(115200);
  SPIFFS.begin();
  {
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
    }
  }
  
  loadConfig();
  delay(1000);
  if(MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }
  
  setup_wifi();

  server.on("/", handleRoot);
  server.on("/settings_sta", handleSettingsSTA);
  server.on("/settings_ap", handleSettingsAP);
  server.on("/getApiData", handleApiData);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");

  client.setServer(mqtt_server.c_str(), 1883);
  client.setCallback(callback);
}

void loop() {
  server.handleClient();
  MDNS.update();
  digitalWrite(BLU, LOW);
  digitalWrite(GRN, HIGH);
  if(!client.connected()) {
    reconnect();
  }
  client.loop();
  
  int buttonState = digitalRead(BUTTON);
  if(buttonState == HIGH) {
    digitalWrite(GRN, LOW);
    digitalWrite(BLU, HIGH);
    ++value;
    Serial.println("PUBLISH MESSAGE: TRUE");
    Serial.println(value);
    client.publish("mqtt/paho/enter", "True");
    delay(2000);
  }
}

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <PubSubClient.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <FS.h>
#include "config.h"

// Configure static IP address
IPAddress staticIP(192, 168, 1, 195);   // Static IP address you want to assign to NodeMCU
IPAddress gateway(192, 168, 1, 1);     // Gateway address (IP address of the modem router)
IPAddress subnet(255, 255, 255, 0);    // Address subnet mask

// MQTT Broker
const int mqttPort = 1883;
const char* mqttTopic = "profile";

// Initialize the web server object
ESP8266WebServer server(80);

// Web server HTML content
String profileHTML;

// Initialize the MQTT client
WiFiClient wifiClient;
PubSubClient client(wifiClient);

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message received on topic: ");
  Serial.println(topic);

  // Convert payload from byte format to string
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.print("Payload: ");
  Serial.println(message);

  // Process message based on topic and payload
  if (strcmp(topic, mqttTopic) == 0) {
    // Process message on the "profile" topic
    if (message.equals("get")) {
      // Send profile information back to the client
      server.send(200, "text/html", profileHTML);
    } else if (message.equals("update")) {
      // Update profile information from the client
      if (server.hasArg("name") && server.hasArg("age") && server.hasArg("location")) {
        String newName = server.arg("name");
        String newAge = server.arg("age");
        String newLocation = server.arg("location");

        // Perform profile update processing here
        // ...

        // Construct the updated profile HTML
        String updatedProfileHTML = profileHTML;
        updatedProfileHTML.replace("{name}", newName);
        updatedProfileHTML.replace("{age}", newAge);
        updatedProfileHTML.replace("{location}", newLocation);

        server.send(200, "text/html", updatedProfileHTML);
      } else {
        server.send(400, "text/html", "Invalid request parameters");
      }
    }
  }
}

void setup() {
  Serial.begin(115200);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected. IP address: ");
  Serial.println(WiFi.localIP());

  // Assign static IP address to NodeMCU
  WiFi.config(staticIP, gateway, subnet);

  // Initialize SPIFFS
  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system");
    return;
  }

  // Read profile HTML file
  File file = SPIFFS.open("/profile.html", "r");
  if (file) {
    profileHTML = file.readString();
    file.close();
  } else {
    Serial.println("Failed to open profile.html");
  }

  // Connect to MQTT broker
  client.setServer(mqtt_server, mqttPort);
  client.setCallback(callback);

  if (client.connect("ESP8266Client")) {
    Serial.println("Connected to MQTT broker");
    client.subscribe(mqttTopic);
  } else {
    Serial.println("Failed to connect to MQTT broker");
  }

  // Handle root URL request
  server.on("/", []() {
    server.send(200, "text/html", profileHTML);
  });

  // Start web server
  server.begin();
  Serial.println("Web server started");
}

void loop() {
  // Handle MQTT messages
  if (!client.connected()) {
    if (client.connect("ESP8266Client")) {
      Serial.println("Connected to MQTT broker");
      client.subscribe(mqttTopic);
    } else {
      Serial.print("Failed to connect to MQTT broker, retrying...");
      delay(2000);
    }
  }
  client.loop();

  // Handle web server requests
  server.handleClient();
}

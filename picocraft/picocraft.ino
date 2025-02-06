#include <WiFi.h>  // Use WiFiNINA if you installed that library

// Replace with your network credentials
#ifndef STASSID
#define STASSID "your-ssid"
#define STAPSK "your-password"
#endif

const char *ssid = STASSID;
const char *password = STAPSK;

WiFiServer server(25565); // Using port 80 for TCP server

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ; // Wait for serial port to open (optional for native USB)
  }

  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("\nConnected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.begin();
}

void loop() {
  WiFiClient client = server.accept();

  if (client) {
    Serial.println("New Client Connected");
    client.println("Hello, client!");

    while (client.connected()) {
      if (client.available()) {
        char thisChar = client.read();
        Serial.write(thisChar); // Echo to Serial Monitor
        client.write(thisChar); // Echo back to client
      }
    }

    client.stop();
    Serial.println("Client Disconnected");
  }
}

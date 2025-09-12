#include "WebServerESP32.h"

const char* ssid     = "A56 de João Vitor";
const char* password = "leme1234";

WebServer server(80);

String htmlPage() {
  String html = "<!DOCTYPE html><html>";
  html += "<head><meta charset='UTF-8'><title>ESP32 Web Server</title></head>";
  html += "<body style='text-align:center; font-family:Arial;'>";
  html += "<h2>Servidor Web ESP32</h2>";
  html += "<p>LED está: ";
  html += (ledState ? "LIGADO" : "DESLIGADO");
  html += "</p>";
  html += "<a href='/ligar'><button>Ligar</button></a>";
  html += "<a href='/desligar'><button>Desligar</button></a>";
  html += "</body></html>";
  return html;
}

void handleRoot() {
  server.send(200, "text/html", htmlPage());
}

void handleLigar() {
  digitalWrite(ledPin, HIGH);
  ledState = true;
  server.send(200, "text/html", htmlPage());
}

void handleDesligar() {
  digitalWrite(ledPin, LOW);
  ledState = false;
  server.send(200, "text/html", htmlPage());
}

void setupWebServer() {
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  server.on("/", handleRoot);
  server.on("/ligar", handleLigar);
  server.on("/desligar", handleDesligar);
  server.begin();
}



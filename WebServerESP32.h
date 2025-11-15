#ifndef WEBSERVERESP32_H
#define WEBSERVERESP32_H

#include <WiFi.h>
#include <WebServer.h>

extern const char* ssid;
extern const char* password;

extern WebServer server;
extern bool ledState;
extern const int ledPin;

void setupWebServer();

#endif

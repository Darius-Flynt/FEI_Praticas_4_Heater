#ifndef WEBSERVERESP32_H
#define WEBSERVERESP32_H

#include <WiFi.h>
#include <WebServer.h>

// Defina aqui suas credenciais WiFi
extern const char* ssid;
extern const char* password;

// Variáveis globais do servidor
extern WebServer server;
extern bool ledState;
extern const int ledPin;

// Funções do servidor
void setupWebServer();

#endif

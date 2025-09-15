#include <WiFi.h>
#include <WebServer.h>
#include "WebPage.h"

// =====================
// Classes
// =====================
class Sensor {
  public:
    float value;
    int pino;

    Sensor() {
      value = 0;
      pino = A0; // valor default
    }

    void LinkIO(int pin){
      pino = pin;
    }

    float Leitura(){
      int raw = analogRead(pino);  // leitura ADC
      value = (float)raw;          // salva em value
      return value;
    }
};

class Rele {
  public:
    bool estado;
    int pino;

    Rele(){
      estado = false;
      pino = 0;
    }

    void LinkIO(int pin){
      pino = pin;
      pinMode(pino, OUTPUT);
      digitalWrite(pino, LOW);
    }

    void Comando(bool cmd){
      estado = cmd;
      digitalWrite(pino, estado ? HIGH : LOW);
    }
};

class RFID {
  public:
    int codigo;
    int pinComm1;
    int pinComm2;

    RFID(){
      codigo = 0;
      pinComm1 = 0;
      pinComm2 = 0;
    }

    void linkComm(int p1, int p2){
      pinComm1 = p1;
      pinComm2 = p2;
    }

    int ReadCode(){
      codigo = 0; // simulação
      return codigo;
    }
};

// =====================
// Objetos globais
// =====================
Sensor sensor;
Rele ventilador;
Rele lampada;

float extTemp = 0;
float Setpoint = 500; // valor inicial de referência

// =====================
// Wi-Fi + WebServer
// =====================
const char* ssid = "A56 de João Vitor";
const char* password = "leme1234";

WebServer server(80);

// rota principal
void handleRoot() {
  server.send(200, "text/html", htmlPage());
}

// rota para setpoint
void handleSetpoint() {
  if (server.hasArg("value")) {
    Setpoint = server.arg("value").toFloat();
    Serial.print("Novo Setpoint: ");
    Serial.println(Setpoint);
  }
  server.send(200, "text/html", htmlPage());
}

// =====================
// Setup
// =====================
void setup() {
  Serial.begin(115200);

  // Inicializa sensores e atuadores
  sensor.LinkIO(A0);
  ventilador.LinkIO(2);
  lampada.LinkIO(4);

  // Conecta no Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");
  Serial.print("IP do ESP32: ");
  Serial.println(WiFi.localIP());

  // Configura rotas
  server.on("/", handleRoot);
  server.on("/setpoint", handleSetpoint);

  // Comandos WEB
  server.on("/ligaFan", []() {
    ventilador.Comando(true);
    server.send(200, "text/html", htmlPage());
    Serial.println("Ligou Fan");
  });
  server.on("/desligaFan", []() {
    ventilador.Comando(false);
    server.send(200, "text/html", htmlPage());
    Serial.println("Desligou Fan");
  });
  server.on("/ligaHeat", []() {
    lampada.Comando(true);
    server.send(200, "text/html", htmlPage());
    Serial.println("Ligou Lampada");
  });
  server.on("/desligaHeat", []() {
    lampada.Comando(false);
    server.send(200, "text/html", htmlPage());
    Serial.println("Desligou Lampada");
  });

  server.begin();
}

// =====================
// Loop principal
// =====================
void loop() {
  server.handleClient();

  extTemp = sensor.Leitura();
  /*
  if (Setpoint < extTemp) {
    ventilador.Comando(true);
    lampada.Comando(false);
  }
  else if (Setpoint > extTemp) {
    ventilador.Comando(false);
    lampada.Comando(true);
  }
  else {
    ventilador.Comando(false);
    lampada.Comando(false);
  }
  */

}

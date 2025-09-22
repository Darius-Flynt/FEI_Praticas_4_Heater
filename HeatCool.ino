#include <WiFi.h>
#include <WebServer.h>

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
float Setpoint = 20; // valor inicial de referência
float histerese;
int Modo; //0 = Manual, 1 = Automatico, 2 = democratico

struct User {
  int ID;
  float Temperatura;
  String Nome;
  int RFID;
};

//Estrutura de dados comunicação Supervisório WEB SERVER
struct Supervisorio {
  int presentUsers; //Numero de Usúarios presentes na sala
  int operMode; //modo de operação (0 - manual, 1 - automatico, 2 - controle de acesso)
  float setpoint; //setpoint definido para controle automatico
  float temp;     //temperatura atual
  bool lampStatus; 
  bool fanStatus; 
  User users[11];
  int acessControl[11];
  bool manualFanCMD;
  bool manualCoolerCMD;
  int validUsers = 3;
};

Supervisorio supervisorio;

#include "WebPage.h"

// =====================
// Wi-Fi + WebServer
// =====================
const char* ssid = "A56 de João Vitor";
const char* password = "leme1234";

WebServer server(80);

// rota principal
void handleRoot() {
  server.send(200, "text/html", htmlPage(supervisorio));
}

// rota para setpoint
void handleSetpoint() {
  if (server.hasArg("value")) {
    Setpoint = server.arg("value").toFloat();
    supervisorio.setpoint = Setpoint;   // atualiza struct
    Serial.print("Novo Setpoint: ");
    Serial.println(Setpoint);
  }
  server.send(200, "text/html", htmlPage(supervisorio));
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

  //SETA HISTERESE PADRÃO:
  histerese = 2;

  // Inicializa supervisorio
  supervisorio.presentUsers = 0;
  supervisorio.operMode = 0;
  supervisorio.setpoint = Setpoint;
  supervisorio.temp = 0;
  supervisorio.lampStatus = false;
  supervisorio.fanStatus = false;
  supervisorio.manualFanCMD = false;
  supervisorio.manualCoolerCMD = false;

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
    supervisorio.fanStatus = true;
    supervisorio.manualFanCMD = true;
    server.send(200, "text/html", htmlPage(supervisorio));
    Serial.println("Ligou Fan");
  });
  server.on("/desligaFan", []() {
    ventilador.Comando(false);
    supervisorio.fanStatus = false;
    supervisorio.manualFanCMD = false;
    server.send(200, "text/html", htmlPage(supervisorio));
    Serial.println("Desligou Fan");
  });
  server.on("/ligaHeat", []() {
    lampada.Comando(true);
    supervisorio.lampStatus = true;
    supervisorio.manualCoolerCMD = true;
    server.send(200, "text/html", htmlPage(supervisorio));
    Serial.println("Ligou Lampada");
  });
  server.on("/desligaHeat", []() {
    lampada.Comando(false);
    supervisorio.lampStatus = false;
    supervisorio.manualCoolerCMD = false;
    server.send(200, "text/html", htmlPage(supervisorio));
    Serial.println("Desligou Lampada");
  });
  server.on("/manual", []() {
    Modo = 0;
    server.send(200, "text/html", htmlPage(supervisorio));
    Serial.println("Modo Manual");
  });
  server.on("/automatico", []() {
    Modo = 1;
    server.send(200, "text/html", htmlPage(supervisorio));
    Serial.println("Modo Automatico");
  });
  server.on("/acesso", []() {
    Modo = 2;
    server.send(200, "text/html", htmlPage(supervisorio));
    Serial.println("Modo Acesso");
  });

  server.begin();
}

// =====================
// Loop principal
// =====================
void loop() {
  server.handleClient();

  // Atualiza leitura
  //extTemp = sensor.Leitura(); DESATIVADO POR TESTE
  extTemp = 22;
  supervisorio.temp = extTemp;
  supervisorio.setpoint = Setpoint;
  supervisorio.lampStatus = lampada.estado;
  supervisorio.fanStatus = ventilador.estado;
  supervisorio.operMode = Modo;

  // Lógica de controle
  if (Modo == 1) { // Automático
    if (extTemp < (Setpoint - histerese)) {
      lampada.Comando(true);
      supervisorio.lampStatus = true;
      ventilador.Comando(false);
      supervisorio.fanStatus = false;
    }
    if (extTemp == Setpoint) {
      lampada.Comando(false);
      supervisorio.lampStatus = false;
      ventilador.Comando(false);
      supervisorio.fanStatus = false;
    }
    if (extTemp > (Setpoint + histerese)) {
      lampada.Comando(false);
      supervisorio.lampStatus = false;
      ventilador.Comando(true);
      supervisorio.fanStatus = true;
    }
  }
}

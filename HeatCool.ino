//===============================================================
// RFID (MFRC522) + Sensor de Temperatura (MAX6675) - ESP32
//===============================================================
#include <SPI.h>
#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
#include <MFRC522DriverPinSimple.h>
#include <max6675.h>
#include <WiFi.h>
#include <WebServer.h>

//---------------------------------------------------------------
// Estruturas de dados
//---------------------------------------------------------------
struct User {
  int ID;
  float Temperatura;
  String Nome;
  int RFID;
  String UID;
  bool presente;
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

struct Supervisorio {
  int presentUsers;
  int operMode;
  float setpoint;
  float temp;
  bool lampStatus;
  bool fanStatus;
  User users[11];
  int acessControl[11];
  bool manualFanCMD;
  bool manualCoolerCMD;
  int validUsers = 0;
};

//---------------------------------------------------------------
const char* ssid = "A56 de João Vitor";
const char* password = "leme1234";

float extTemp = 0;
float Setpoint = 20;
float histerese;
int Modo;

Rele ventilador;
Rele lampada;

WebServer server(80);
Supervisorio supervisorio;
bool precisaRecarregar = false; // <-- flag para reload automático

#include "WebPage.h"

//---------------------------------------------------------------
// HANDLERS
//---------------------------------------------------------------
void handleRoot() {
  server.send(200, "text/html", htmlPage(supervisorio));
}

void handleSetpoint() {
  if (server.hasArg("value")) {
    Setpoint = server.arg("value").toFloat();
    supervisorio.setpoint = Setpoint;
    Serial.print("Novo Setpoint: ");
    Serial.println(Setpoint);
  }
  server.send(200, "text/html", htmlPage(supervisorio));
}

void handleCheckUpdate() {
  if (precisaRecarregar) {
    precisaRecarregar = false;
    server.send(200, "text/plain", "reload");
  } else {
    server.send(200, "text/plain", "ok");
  }
}

//---------------------------------------------------------------
// Pinos RFID (SPI principal)
//---------------------------------------------------------------
#define SS_PIN   5
#define RST_PIN  2
#define SCK_RFID 17
#define MISO_RFID 4
#define MOSI_RFID 16

//---------------------------------------------------------------
// Pinos MAX6675 (SPI secundário)
//---------------------------------------------------------------
#define SCK_MAX 18
#define CS_MAX  19
#define SO_MAX  23

//---------------------------------------------------------------
// Objetos
//---------------------------------------------------------------
MFRC522DriverPinSimple ss_pin(SS_PIN);
SPIClass &spiRFID = SPI;
const SPISettings spiSettings = SPISettings(SPI_CLOCK_DIV4, MSBFIRST, SPI_MODE0);
MFRC522DriverSPI driver{ss_pin, spiRFID, spiSettings};
MFRC522 mfrc522{driver};
MAX6675 thermocouple(SCK_MAX, CS_MAX, SO_MAX);

int estado_sistema = LOW;
unsigned long timer_rfid = 0;
unsigned long timer_temp = 0;
String read_rfid;

//---------------------------------------------------------------
// Funções RFID
//---------------------------------------------------------------
void dump_byte_array(byte *buffer, byte bufferSize) {
  read_rfid = "";
  for (byte i = 0; i < bufferSize; i++) {
    read_rfid += String(buffer[i], HEX);
  }
}

void leitura_rfid() {
  if ((millis() - timer_rfid) > 1000) {
    if (!mfrc522.PICC_IsNewCardPresent()) return;
    if (!mfrc522.PICC_ReadCardSerial()) return;

    dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
    Serial.print("\nID do cartao (HEX): ");
    Serial.println(read_rfid);

    int idx = buscarUsuario(read_rfid);

    if (idx == -1) {
      cadastrarUsuario(read_rfid);
    } else {
      atualizarPresenca(idx);
    }

    timer_rfid = millis();
  }
}

int buscarUsuario(String uid) {
  for (int i = 0; i < supervisorio.validUsers; i++) {
    if (supervisorio.users[i].UID == uid) return i;
  }
  return -1;
}

void cadastrarUsuario(String uid) {
  if (supervisorio.validUsers >= 11) {
    Serial.println("Limite maximo de usuarios atingido!");
    return;
  }
  int idx = supervisorio.validUsers;
  supervisorio.users[idx].ID = idx + 1;
  supervisorio.users[idx].UID = uid;
  supervisorio.users[idx].Nome = "Usuario " + String(idx + 1);
  supervisorio.users[idx].Temperatura = 0;
  supervisorio.users[idx].presente = true;
  supervisorio.validUsers++;
  supervisorio.presentUsers++;
  precisaRecarregar = true;  // <- avisa o navegador
  Serial.println("Novo usuario cadastrado e marcado como presente!");
}

void atualizarPresenca(int indice) {
  supervisorio.users[indice].presente = !supervisorio.users[indice].presente;
  if (supervisorio.users[indice].presente)
    supervisorio.presentUsers++;
  else
    supervisorio.presentUsers--;

  precisaRecarregar = true; // <- avisa o navegador que deve recarregar

  Serial.print("Usuario ");
  Serial.print(supervisorio.users[indice].Nome);
  Serial.print(" agora esta ");
  Serial.println(supervisorio.users[indice].presente ? "PRESENTE" : "AUSENTE");
}

//---------------------------------------------------------------
// Setup
//---------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(500);

  ventilador.LinkIO(13);
  lampada.LinkIO(32);
  pinMode(SS_PIN, OUTPUT);
  digitalWrite(SS_PIN, HIGH);
  pinMode(RST_PIN, OUTPUT);
  digitalWrite(RST_PIN, HIGH);

  SPI.begin(SCK_RFID, MISO_RFID, MOSI_RFID, SS_PIN);
  mfrc522.PCD_Init();
  Serial.println("Sistema bloqueado. Aguardando cartao...\n");

  pinMode(SCK_MAX, OUTPUT);
  pinMode(CS_MAX, OUTPUT);
  pinMode(SO_MAX, INPUT);

  histerese = 2;

  supervisorio.presentUsers = 0;
  supervisorio.operMode = 0;
  supervisorio.setpoint = Setpoint;
  supervisorio.temp = 0;
  supervisorio.lampStatus = false;
  supervisorio.fanStatus = false;
  supervisorio.manualFanCMD = false;
  supervisorio.manualCoolerCMD = false;

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");
  Serial.print("IP do ESP32: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/setpoint", handleSetpoint);
  server.on("/checkUpdate", handleCheckUpdate);

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

//---------------------------------------------------------------
// Loop
//---------------------------------------------------------------
void loop() {
  leitura_rfid();

  if (millis() - timer_temp > 10000) {
    double tempC = thermocouple.readCelsius();
    Serial.print("Temperatura: ");
    Serial.print(tempC);
    Serial.println(" °C");
    timer_temp = millis();
    supervisorio.temp = tempC;
    extTemp = tempC;
  }
  
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

  server.handleClient();
}

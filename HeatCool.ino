#include <WiFi.h>
#include <WebServer.h>
#include <SPI.h>
#include <MFRC522.h>
#include "max6675.h"

// =====================
// Classes
// =====================
class Sensor {
  private:
    int pinSO;   // MISO
    int pinCS;   // Chip Select
    int pinSCK;  // Clock
    MAX6675 thermocouple;

  public:
    float value;

    Sensor() : thermocouple(0, 0, 0) {
      value = 0.0;
      pinSO = pinCS = pinSCK = 0;
    }

    void LinkIO(int sck, int cs, int so) {
      pinSCK = sck;
      pinCS = cs;
      pinSO = so;
      thermocouple = MAX6675(pinSCK, pinCS, pinSO);
    }

    float Leitura() {
      value = thermocouple.readCelsius();
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
  private:
    MFRC522 mfrc522;          // objeto da biblioteca
    MFRC522::MIFARE_Key key;  // chave de comunicação (não usada aqui, mas pode ser útil)
    byte readCard[4];         // UID do cartão
    bool cardDetected;        // flag de leitura
    int pinSS;                // pino de comunicação SS
    int pinRST;               // pino de reset

  public:
    // Construtor
    RFID(int ssPin, int rstPin) : mfrc522(ssPin, rstPin) {
      pinSS = ssPin;
      pinRST = rstPin;
      cardDetected = false;
    }

    // Inicializa o módulo
    void begin() {
      SPI.begin(18, 19, 23, pinSS);
      mfrc522.PCD_Init();
      Serial.println("RFID iniciado com sucesso.");
      Serial.println("Aproxime o cartão...");
    }

    // Verifica e lê o cartão
    bool readCardUID() {
      // verifica se há novo cartão
      if (!mfrc522.PICC_IsNewCardPresent()) {
        return false;
      }

      // tenta ler o cartão
      if (!mfrc522.PICC_ReadCardSerial()) {
        return false;
      }

      Serial.print("Cartão detectado! UID: ");
      for (byte i = 0; i < mfrc522.uid.size; i++) {
        readCard[i] = mfrc522.uid.uidByte[i];
        Serial.print(readCard[i], HEX);
      }
      Serial.println();
      mfrc522.PICC_HaltA(); // encerra comunicação com o cartão
      return true;
    }

    // Retorna o UID em formato inteiro (ou qualquer outro formato desejado)
    unsigned long getCardCode() {
      unsigned long code = 0;
      for (int i = 0; i < 4; i++) {
        code = (code << 8) | readCard[i];
      }
      return code;
    }
};

RFID leitor(21, 22);
Sensor termopar;

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

  leitor.begin();
  termopar.LinkIO(18, 32, 19);
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

  if (leitor.readCardUID()) {
    unsigned long codigo = leitor.getCardCode();
    Serial.print("Código numérico: ");
    Serial.println(codigo);
    delay(1000); // pequena pausa
  }

  // --- Leitura do MAX6675 ---
  float temp = termopar.Leitura();
  Serial.print("Temperatura: ");
  Serial.print(temp);
  Serial.println(" °C");
}

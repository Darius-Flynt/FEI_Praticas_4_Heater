//===============================================================
// RFID (MFRC522)
//===============================================================
#include <SPI.h>
#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
#include <MFRC522DriverPinSimple.h>


//===============================================================
// Sensor de Temperatura (MAX6675)
//===============================================================
#include <max6675.h>

//===============================================================
// WIFI
//===============================================================
#include <WiFi.h>
#include <WebServer.h>

//===============================================================
// Display OLED - ESP32
//===============================================================
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C  // endereço detectado
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//===============================================================
// Variáveis Display e botão
//===============================================================
int pinoBotao = 14; // Botão no GPIO 14
int telaAtual = 0;
bool botaoAnterior = HIGH;
unsigned long ultimoUpdateLCD = 0;

void MOSTRA_LCD(String valor, String frase) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(frase);
  display.setCursor(0, 20);
  display.print("Valor: ");
  display.println(valor);

  display.display();
  delay(10);
}

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

//Conexão WIFI---------------------------------------------------------------
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
bool precisaRecarregar = false; // <-- flag para reload automático no Client WEB

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

unsigned long tempoleitura = millis();

const int pino_sct = 33;  // Pino Analogico ADC
double CorrenteRms = 0;
unsigned long tempoatual;
float corrente = 0;
float correnteRMS=0;
float correnteMax = 0;
float correnteMin = 0;

int rede = 127; // Tensao da rede eletrica
double UltimoS = 0;
float energia=0;
unsigned long tempoanterior;
unsigned long deltat;

float CorrenteRmsMedia = 0;
float UltimoCorrenteRmsMedia = 0;
int nMed_Irms = 0;
double Ativa = 0;
unsigned long timer_Irms = millis();

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

  ventilador.LinkIO(32);
  lampada.LinkIO(13);
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

  histerese = 1;

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
    //teste
    if (Modo == 2) { // Acesso
    float somaTemperaturas = 0.0;
    int totalPresentes = 0;

    for (int i = 0; i < 10; i++) {
      if (supervisorio.users[i].presente) {
        somaTemperaturas += supervisorio.users[i].Temperatura;
        totalPresentes++;
      }
    }

    float mediaTemperatura = 0.0;
    if (totalPresentes > 0) {
      mediaTemperatura = somaTemperaturas / totalPresentes;
    }

    // Exemplo de uso:
    Setpoint = mediaTemperatura;
    supervisorio.setpoint = mediaTemperatura;
    }
    server.send(200, "text/html", htmlPage(supervisorio));
    Serial.println("Modo Acesso");
  });

  server.on("/updateUser", []() {
  for (int i = 0; i < supervisorio.validUsers; i++) {
    String nomeCampo = "name" + String(i);
    String spCampo = "spUser" + String(i);

    if (server.hasArg(nomeCampo)) {
      supervisorio.users[i].Nome = server.arg(nomeCampo);
    }
    if (server.hasArg(spCampo)) {
      supervisorio.users[i].Temperatura = server.arg(spCampo).toFloat();
    }
  }

  precisaRecarregar = true;  // força reload no navegador
  server.send(200, "text/html", htmlPage(supervisorio));

  Serial.println("=== Usuários atualizados ===");
  for (int i = 0; i < supervisorio.validUsers; i++) {
    Serial.print("User ");
    Serial.print(i);
    Serial.print(" -> Nome: ");
    Serial.print(supervisorio.users[i].Nome);
    Serial.print(" | SP: ");
    Serial.println(supervisorio.users[i].Temperatura, 1);
  }
});


  server.begin();

  //===============================================================
  // Inicialização do OLED e botão
  //===============================================================
  Wire.begin(21, 22);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("Falha ao inicializar o display OLED!");
    while (1);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 10);
  display.println("Iniciando...");
  display.display();

  pinMode(pinoBotao, INPUT_PULLUP);
  MOSTRA_LCD(String(Setpoint, 1), "Sistema Pronto");
  delay(1000);

}

float Irms() {
  // Leitura da Corrente
  float soma = 0;
  float nMedio = 0;
  float amostras[1000]; 
  float produto[1000]; 

  // Leitura de 1 periodo
  int i = 1;
  tempoatual = micros();
  while(micros() - tempoatual < 16667) {
    //corrente = ((analogRead(pino_sct) - 2048.0) / 4096.0 * 3.3 / 10.0 / 20.0 * 2000.0); // SCT013 100A:50mA, R = 20 Ohm e 10 voltas de fio
    //corrente = ((analogRead(pino_sct) - 2048.0) / 4096.0 * 3.3 / 20.0 * 2000.0);        // SCT013 100A:50mA, R = 20 Ohm e fio sem voltas
      corrente = (((analogRead(pino_sct) - 2048.0) / 4096.0) * 3.3 * 20.0);                 // SCT013 20A/1V, sem resistor e fio sem voltas
    //corrente = ((analogRead(pino_sct) - 2048.0) / 4096.0 * 3.3 * 30.0);                 // SCT013 30A/1V, sem resistor e fio sem voltas
    //corrente = ((analogRead(pino_sct) - 2048.0) / 4096.0 * 3.3 * 05.0);                 // SCT013 5A/1V, sem resistor e fio sem voltas
    amostras[i] = corrente;
    i++;
  }

  // Calcula o nivel medio
  correnteMax = -100.0;
  correnteMin = 100.0;
  for(int n = 1; n < i; n++) {
    if(amostras[n] > correnteMax) {
      correnteMax = amostras[n];
    }
    if(amostras[n] < correnteMin) {
      correnteMin = amostras[n];
    }
  }
  nMedio = (correnteMin + correnteMax) / 2.0;

  // Remove o nivel medio e faz o somatorio das amostras ao quadrado
  for(int n = 1; n < i; n++) {
    //amostras[n] = amostras[n] - nMedio; // Remove nivel medio
    produto[n] = amostras[n] * amostras[n];
    soma += produto[n];
  }

  // Calcula a media dos valores
  correnteRMS = soma / (i-1);
  // Calcula a raiz quadrada
  correnteRMS = sqrt(correnteRMS);

  // Calculo da potAparente em kVA
  UltimoS = correnteRMS * rede / 1000.0; 

  // Calcula a potencia em kW considerando FP = 0.8
  Ativa = UltimoS * 0.8;

  // Calcula o consumo de energia em kWh
  tempoatual = millis();
  deltat = tempoatual - tempoanterior;
  tempoanterior = tempoatual;
  energia += (Ativa * deltat) / 3600.0 / 1000.0;
  
  Serial.print("Corrente RMS (A): ");
  Serial.print(correnteRMS,3);
  Serial.print("\tPotencia Aparente (kVA): ");
  Serial.print(UltimoS,3);
  Serial.print("\tPotencia Ativa (FP=0.8 - kW): ");
  Serial.print(Ativa,3);
  Serial.print("\tConsumo de Energia (FP=0.8 - kWh): ");
  Serial.print(energia,6);
  Serial.println("");
  return correnteRMS;
  
}

//---------------------------------------------------------------
// Loop
//---------------------------------------------------------------
void loop() {
  leitura_rfid();

  if (millis() - timer_temp > 3000) {
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
    if (lampada.estado == true and extTemp > Setpoint ) {
      lampada.Comando(false);
      supervisorio.lampStatus = false;
    }
    if (ventilador.estado == true and extTemp < Setpoint ) {
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
  if (Modo == 2) { // Acesso
    float somaTemperaturas = 0.0;
    int totalPresentes = 0;

    for (int i = 0; i < 10; i++) {
      if (supervisorio.users[i].presente) {
        somaTemperaturas += supervisorio.users[i].Temperatura;
        totalPresentes++;
      }
    }

    float mediaTemperatura = 0.0;
    if (totalPresentes > 0) {
      mediaTemperatura = somaTemperaturas / totalPresentes;
    }

    // calcula media da temperatura dos usuarios presentes:
    Setpoint = mediaTemperatura;
    supervisorio.setpoint = mediaTemperatura;

    if(totalPresentes != 0){
      if (extTemp < (Setpoint - histerese)) {
        lampada.Comando(true);
        supervisorio.lampStatus = true;
        ventilador.Comando(false);
        supervisorio.fanStatus = false;
      }
      if (lampada.estado == true and extTemp > Setpoint ) {
        lampada.Comando(false);
        supervisorio.lampStatus = false;
      }
      if (ventilador.estado == true and extTemp < Setpoint ) {
        ventilador.Comando(false);
        supervisorio.fanStatus = false;
      }
      if (extTemp > (Setpoint + histerese)) {
        lampada.Comando(false);
        supervisorio.lampStatus = false;
        ventilador.Comando(true);
        supervisorio.fanStatus = true;
      }
    } else {
        lampada.Comando(false);
        supervisorio.lampStatus = false;
        ventilador.Comando(false);
        supervisorio.fanStatus = false;
    }
    
  }
  

  //LEITURA SENSOR CORRENTE
  if((millis() - tempoleitura) > 500) {
    Irms();
    tempoleitura = millis();
  }

  server.handleClient();

  //inclusao oled
  // Alternância de tela via botão
  bool botaoEstado = digitalRead(pinoBotao);
  if (botaoAnterior == HIGH && botaoEstado == LOW) {
    telaAtual = !telaAtual;
  }
  botaoAnterior = botaoEstado;

  // Atualização do OLED
  if (millis() - ultimoUpdateLCD > 1000) {
    ultimoUpdateLCD = millis();
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    if (telaAtual == 0) {
      display.setCursor(0, 0);
      display.println("Corrente RMS:");
      display.setCursor(0, 20);
      display.print(correnteRMS, 2);
      display.println(" A");
    } else {
      display.setCursor(0, 0);
      display.print("Temp: ");
      display.print(extTemp, 1);
      display.println(" C");
      display.setCursor(0, 20);
      display.print("SP: ");
      display.print(Setpoint, 1);
      display.println(" C");
    }

    // === Mostra o IP do ESP32 na última linha ===
    display.setTextSize(1);
    display.setCursor(0, 56);  
    display.print("IP: ");
    display.print(WiFi.localIP());

    display.display();
  }
  
}

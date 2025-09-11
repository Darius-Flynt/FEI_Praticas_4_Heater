class Sensor {
  public:
    float value;
    int pino;

    // Construtor
    Sensor() {
      value = 0;
      pino = A0; // valor default
    }

    void LinkIO(int pin){
      pino = pin;
    }

    float Leitura(){
      int raw = analogRead(pino);  // leitura do ADC
      value = (float)raw;          // salva em value
      return value;
    }
};

class Rele {
  public:
    bool estado;
    int pino;

    // Construtor
    Rele(){
      estado = false;
      pino = 0;
    }

    void LinkIO(int pin){
      pino = pin;
      pinMode(pino, OUTPUT);  // define como saída
      digitalWrite(pino, LOW); // inicializa desligado
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

    //construtor
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
      codigo = 0; //Substituir depois *********************************
      return codigo;
    }

};

// Objetos globais
Sensor sensor;
Rele ventilador;
Rele lampada;

float extTemp = 0;
float Setpoint = 500; // exemplo de referência

void setup() {
  sensor.LinkIO(A0);   // pino analógico
  ventilador.LinkIO(3);
  lampada.LinkIO(4);
}

void loop() {
  extTemp = sensor.Leitura();

  if (Setpoint < extTemp) {
    ventilador.Comando(true);
    lampada.Comando(false);
  }
  else if (Setpoint > extTemp) {
    ventilador.Comando(false);
    lampada.Comando(true);
  }
  else if (Setpoint == extTemp) {
    ventilador.Comando(false);
    lampada.Comando(false);
  }
}

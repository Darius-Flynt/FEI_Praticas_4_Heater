class Sensor {
  public:
    float value;
    float raw;

    // Construtor
    Sensor() {
      value = 0;
      raw = 0;
    }

    void LinkIO(int pin){
      analogWrite(pin, raw);
    }

    float Leitura(){ 
      value = raw;
      return value;
    }
};

Sensor sensor;
float Valor;

void setup() {
  // put your setup code here, to run once:

}

void loop() {
  sensor.LinkIO(2);
  Valor = sensor.Leitura();
}

/* Estação Meteorológica
 * 
 */
#include <SimpleTimer.h>        // Incluir biblioteca do Timer?? 
#include <ESP8266WiFi.h>          // Incluir biblioteca do ESP para WiFi
#include <BlynkSimpleEsp8266.h>   // Incluir biblioteca para o Blynk
#include "DHT.h"                  // Incluir biblioteca do sensor de Temperatura e Umidade
#include <Wire.h>                 // Incluir biblioteca para sensor de Pressão
#include <Adafruit_BMP085.h>      // Incluir biblioteca do sensor de Pressão 


#define DHTTYPE DHT22             // Sensor de Temperatura DHT11
#define dht_dpin D4               // Pino do Sensor de Temperatura e Umidade
#define CO2PIN A0                 // Pino Analógico do Sensor de CO2
#define coefficient_A 19.32       // Coeficiente para calcular PPM do Sensor de CO2
#define coefficient_B -0.64       // Coeficiente para calcular PPM do Sensor de CO2
#define R_Load 10.0               // Resistência de Carga do Sensor de CO2
#define raio 0.05                 // Raio do Anemometro


DHT dht(dht_dpin, DHTTYPE);       // Inicia Sensor de Temperatura e Umidade com nome dht
SimpleTimer timer;                // Define nome do timer de timer
SimpleTimer timerCO2;                // Define nome do timer de timer
Adafruit_BMP085 bmp;              // Inicia sensor de Pressão com nome bmp (D1 – SCL e D2 – SDA)



char auth[] = "#############";    // Código do Blynk
char ssid[] = "########";         // Nome da WiFi
char pass[] = "########";         // Senha da WiFi
float t;                          // Variável para Temperatura
float h;                          // Variável para Umidade
float p;                          // Variável para Pressão
float co2 = 0.00;                 // Variável para CO2
float hic;                        // Variável para Índice de Calor
const byte interruptPin = 13;     // Pino do Sensor do Anemômetro
volatile byte interruptCounter = 0; // Variável do Contador
float velocidade;                 // Variável para Velocidade 
const byte aquecPin = 16;         // Pino do Sensor do Anemômetro
int contador = 0;

void ICACHE_RAM_ATTR handleInterrupt();

void setup() {
  //Serial.begin(115200);                   // Inicia Serial com baud 115200
  Blynk.begin(auth, ssid, pass);          // Inicia servidor do Blynk
  dht.begin();                            // Inicia sensor de Temperatura e Umidade
  bmp.begin();                            // Inicia sensor de Pressão
  pinMode(interruptPin, INPUT_PULLUP);    // Pino da interrupção (Sensor de Velocidade) como entrada e com Pull-up
  attachInterrupt(digitalPinToInterrupt(interruptPin), handleInterrupt, FALLING); // Inicia interrupção externa com borda de descida
  timer.setInterval(30000, sendUptime);    // Inicia interrupção de Timer com 2 segundos
  pinMode(aquecPin, OUTPUT);              // Pino da
  timerCO2.setInterval(30000, calcCO2);    // Inicia interrupção de Timer com 30 segundos
  digitalWrite(aquecPin,HIGH);
  delay(15);
}

void handleInterrupt() {                    // Rotina de Interrupção Externa
  interruptCounter++;
}

void loop() {
  timer.run();                            // Ativa o Timer
  timerCO2.run();                            // Ativa o Timer
  Blynk.run();                            // Ativa o Blynk
}

void calcCO2() {        // 60s em 5V e 90 s em 1,5 V
  contador++;
  Serial.println("Entrou");
  if (contador == 2) {
    co2 = getPPM();                           // Lê CO2 e salva em co2
    digitalWrite(aquecPin,LOW);
    //Serial.println("60s");
  }
  if (contador == 5) {
    digitalWrite(aquecPin,HIGH);
    contador = 0;
    //Serial.println("90s");
  }
}

void sendUptime() {                               // Rotina de Estouro do Timer
  h = dht.readHumidity();                   // Lê umidade e salva em h
  t = dht.readTemperature();                // Lê temperatura e salva em t 
  p = (bmp.readPressure() + 225.373)/100.00;          // Lê pressão e salva em p
  velocidade = calc_velocidade();           // Lê velocidade e salva em velocidade
  hic = dht.computeHeatIndex(t, h, false);  // Calcula Índice de calor
  /*Serial.print("Umidade = ");                     // Imprime dados na serial
  Serial.print(h);                                // Retirar após testes 
  Serial.print("%, Temperatura = ");
  Serial.print(t);
  Serial.print(" ºC, Índice de Calor = ");
  Serial.print(hic);
  Serial.print(" ºC, Pressão = ");
  Serial.println(p);
  Serial.print(" Pa, CO2 = ");
  Serial.print(co2);
  Serial.println(" PPM");, Velocidade do Vento = ");
  Serial.print(velocidade);
  Serial.println(" km/h");                        // Retirar atá aqui */
  Blynk.virtualWrite(V0, t);                      // Variável virtual da temperatura V0
  Blynk.virtualWrite(V1, h);                      // Variável virtual da umidade V1
  Blynk.virtualWrite(V2, p);                      // Variável virtual da pressão V2
  Blynk.virtualWrite(V3, co2);                    // Variável virtual da CO2 V3
  Blynk.virtualWrite(V4, velocidade);             // Variável virtual da Velocidade V4
  Blynk.virtualWrite(V5, hic);                    // Variável virtual da Índice de Calor V5 
}



float calc_velocidade(){                    // Rotina de Calculo de velocidade do vento
  float circ = 2 * PI * raio;
  float vel = (circ * interruptCounter)/2;
  interruptCounter = 0;
  return vel*3.6;
}

float getPPM(){                             // Rotina de Leitura do Sensor de CO2 em PPM
  return (float)(coefficient_A * pow(getRatio(), coefficient_B));
}

float getRatio(){                          // Rotina de Leitura do Sensor de CO2
  int value = analogRead(CO2PIN);
  float v_out = voltageConversion(value);
  return (3.3 - v_out) / v_out;
}

float voltageConversion(int value){        // Rotina de Leitura do Sensor de CO2
  return (float) value * (3.3 / 1023.0);
}

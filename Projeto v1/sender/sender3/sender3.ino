//Sender da Estação-1 E1  04/12/2023
// Sistema para recebimento e leituras de Estações de Coleta de Dados e envio para plataforma Tago.Io
//Autor: Davi Gaede Fiusa
//davi.fiusa@ifro.edu.br
//(69) 98472-7951

//bibliotecas utilizadas
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <LoRa.h>

//definção dos pinos utilizados pelo LORA
#define SCK 5    // GPIO5  -- SX127x's SCK
#define MISO 19  // GPIO19 -- SX127x's MISO
#define MOSI 27  // GPIO27 -- SX127x's MOSI
#define SS 18    // GPIO18 -- SX127x's CS
#define RST 14   // GPIO14 -- SX127x's RESET
#define DI00 26  // GPIO26 -- SX127x's IRQ(Interrupt Request)

// Espaçamentos para fonte tamanho 1 para exibição no display = display.setTextSize(1);
#define linha1 0
#define linha2 8
#define linha3 16
#define linha4 24
#define linha5 32
#define linha6 40
#define linha7 48
#define linha8 56

int soloumido = 1040; //variáveis para determinação dos valores de solo totalmente úmido encontrados no processo de calibração
int soloseco = 2740; //variáveis para determinação dos valores de solo totalmente seco encontrados no processo de calibração

int contenvio = 0;   //contador de envio de pacotes ao receiver
int ContRecebe = 0;  //contador de recebimento de pacotes do receiver
int Estacao = 3;     //número da Estação

int umi1 = 0;            //recebe a leitura da serial em 12bits de 0 a 4095
int umidade1 = 0;        //converte em um valor decimal de 0 a 100
int umidade1_max = 0;    //armazena a leitura máxima
int umidade1_min = 100;  //armazena a leitura mínima

int umi2 = 0;
int umidade2 = 0;
int umidade2_max = 0;
int umidade2_min = 100;

int umi3 = 0;
int umidade3 = 0;
int umidade3_max = 0;
int umidade3_min = 100;

float temperatura;
float temperatura_max;
float temperatura_min = 100.00;

float umidade;
float umidade_min = 100.00;
float umidade_max;

int soma = 0;   //soma das leituras de umidade de solo
int media = 0;  //media das leituras de umidade de solo

String mensagem = "";  //string de envio do Json

int rssi = 0; //variável da leitura do RSSI

// Variáveis para a Função UPTIME
unsigned long previousMillis1 = 0;
const unsigned long interval1 = 60000;  // Intervalo de 1 minuto
int minutos = 0;
int horas = 0;

// Variáveis para a função de envio por LoRa
unsigned long previousMillis2 = 0;
unsigned long Interval2 = 240000;  //intervalo inicial de envio das leituras dos sensores

// Variáveis para a Função de leitura dos sensores a cada 14s
unsigned long previousMillis3 = 0;
const unsigned long interval3 = 236000;  // Intervalo de 236 segundos

Adafruit_SSD1306 display(128, 64, &Wire, 16);  //objeto do display, resolução 128x64

DHT dht(17, DHT22);  //define o modelo do DHT e o pino ligado

///////////////////////////////////////////////////////////////////
//capta as leituras de todos os sensores e atualiza Min e Max de todos
void atualizaleituras() {
  temperatura = dht.readTemperature();
  umidade = dht.readHumidity();
  umi1 = (analogRead(36));
  umi2 = (analogRead(39));
  umi3 = (analogRead(38));
  umidade1 = map(umi1, soloumido, soloseco, 100, 0);  //mapeamento de valores
  umidade2 = map(umi2, soloumido, soloseco, 100, 0);
  umidade3 = map(umi3, soloumido, soloseco, 100, 0);
  soma = umidade1 + umidade2 + umidade3;
  media = soma / 3;

  if (umidade1 > umidade1_max) {
    umidade1_max = umidade1;
  }
  if (umidade1 < umidade1_min) {
    umidade1_min = umidade1;
  }

  if (umidade2 > umidade2_max) {
    umidade2_max = umidade2;
  }
  if (umidade2 < umidade2_min) {
    umidade2_min = umidade2;
  }
  if (umidade3 > umidade3_max) {
    umidade3_max = umidade3;
  }
  if (umidade3 < umidade3_min) {
    umidade3_min = umidade3;
  }

  if (temperatura > temperatura_max) {
    temperatura_max = temperatura;
  }
  if (temperatura < temperatura_min) {
    temperatura_min = temperatura;
  }

  if (umidade > umidade_max) {
    umidade_max = umidade;
  }
  if (umidade < umidade_min) {
    umidade_min = umidade;
  }
}

///////////////////////////////////////////////////////////////////
//atualiza leituras, envia leituras e atualiza contador
void envialeituraslora() {
  unsigned long currentMillis2 = millis();  // Verifica se é hora de enviar o pacote de leituras dos sensores
  if (currentMillis2 - previousMillis2 >= Interval2) {
    previousMillis2 = currentMillis2;

    atualizaleituras();
    Interval2 = random(240000, 300000);  // Intervalo aleatório entre 4 e 5 minutos para envio das leituras
    contenvio++;
    // Monta a mensagem JSON com as leituras de umidade
    String mensagem = String(R"({
  "Est": )") + String(Estacao)
                      + String(R"(,
  "um1": )") + String(umidade1)
                      + String(R"(,
  "um2": )") + String(umidade2)
                      + String(R"(,
  "um3": )") + String(umidade3)
                      + String(R"(,
  "md": )") + String(media)
                      + String(R"(,
  "te": )")
                      + String(temperatura)
                      + String(R"(,
  "um": )")
                      + String(umidade)
                      + String(R"(,
  "mi": )") + String(minutos)
                      + String(R"(,
  "hs": )") + String(horas)
                      + String(R"(,
  "co": )") + String(contenvio)
                      + String(R"(  
 })");

    LoRa.beginPacket();    //inicia o processo de envio de um pacote
    LoRa.print(mensagem);  //dado do pacote
    LoRa.endPacket();      //fechamento do pacote e envio
    Serial.println(mensagem);
  }
}
//Recebe pacotes LoRa para verificar RSSI no Sender
void recebepacoterssi() {
  if (LoRa.parsePacket()) {  // Recebe a mensagem via LoRa
    String receivedMessage = "";
    while (LoRa.available()) {
      receivedMessage += (char)LoRa.read();
    }
    if (receivedMessage == "666") {  //mensagem que o receiver ira enviar para não confundir com outras mensagens enviadas das outras estações
      rssi = LoRa.packetRssi();
      ContRecebe++;  // Armazena o RSSI do pacote recebido
      Serial.print("RSSI do pacote recebido do Receiver: ");
      Serial.println(rssi);  // Imprime o RSSI do pacote recebido
    }
  }
}

//Faz um relógio contando minutos e horas de UPTIME do sender
void relogio() {
  unsigned long currentMillis1 = millis();
  if (currentMillis1 - previousMillis1 >= interval1) {
    previousMillis1 = currentMillis1;
    minutos++;
    if (minutos == 60) {
      minutos = 0;
      horas++;
    }
  }
}

void touchdisplay() {
  //--==DISPLAY OLED==--
  // --Legendas do Display:--
  //E: Estação
  //UT: UpTime
  //dB: decibés do RSSI nos pacotes de input
  //In: Pacotes de Entrada
  //Ou: Pacotes de saída
  //Me: Media da leitura dos 3 sensores de umidade de solo
  //Se1 2 3: Leitura dos 3 sensores de umidade de solo
  //Ma1 2 3: Leitura máxima de cada um dos 3 sensores de umidade de solo
  //Mi1 2 3: Leitura mínima de cada um dos 3 sensores de umidade de solo
  //Te: Temperatura Ambiente
  //Um: Umidade Ambiente
  //MaT: Máxima da Temperatura Ambiente
  //MiT: Mínima da Temperatura Ambiente
  //MaU: Máxima da Umidade Ambiente
  //Miu: Mínima da Umidade Ambiente
  
  //laço para exibir a leitura no display local apenas quando feito "touch" no pino 32 economizando bateria 
  //assim o display fica desligado e só exibe quando há um touch
  if (touchRead(32) < 20) {
    atualizaleituras();

    display.clearDisplay();  //Limpa o buffer do display mas sem limpar a tela Display

    //Linha-1
    display.setCursor(0, linha1);
    display.print("E:");  //Estação
    display.print(Estacao);
    display.print(" UT:");  //UpTime
    display.printf("%03d:%02d", horas, minutos);
    display.print(" dB:");  //decibés
    display.print(rssi);

    //Linha-2
    display.setCursor(0, linha2);
    display.print("In:");  //Input
    display.print(ContRecebe);
    display.print(" Ou:");  //Output
    display.print(contenvio);
    display.print(" Me:");  //Media dos sensores de solo
    display.print(media);
    display.print("%");

    //Linha-3
    display.setCursor(0, linha3);
    display.print("Se1 2 3: ");  //leitura dos sensores de umidade de solo
    display.print(umidade1);     
    display.print("% ");
    display.print(umidade2);  
    display.print("% ");
    display.print(umidade3);  
    display.print("%");

    //Linha-4
    display.setCursor(0, linha4);
    display.print("Ma1 2 3: ");  //Maxima dos sensores de umidade de solo
    display.print(umidade1_max);
    display.print("% ");
    display.print(umidade2_max);
    display.print("% ");
    display.print(umidade3_max);
    display.print("%");

    //Linha-5
    display.setCursor(0, linha5);
    display.print("Mi1 2 3: ");  //Minima dos sensores de umidade de solo
    display.print(umidade1_min);
    display.print("% ");
    display.print(umidade2_min);
    display.print("% ");
    display.print(umidade3_min);
    display.print("%");

    //Linha-6
    display.setCursor(0, linha6);
    display.print("Te:");  //Temperatura ambiente
    display.print(temperatura);
    display.print("C");
    display.print(" Um:");  //Umidade ambiente
    display.print(umidade);
    display.print("%");

    //Linha-7
    display.setCursor(0, linha7);
    display.print("MaT:");  //Maxima Temperatura ambiente
    display.print(temperatura_max);
    display.print("C");
    display.print(" MiT:");  //Minima Temperatura ambiente
    display.print(temperatura_min);
    display.print("C");

    //Linha-8
    display.setCursor(0, linha8);
    display.print("MaU:");  //Maxima Umidade ambiente
    display.print(umidade_max);
    display.print("%");
    display.print(" MiU:");  //Minima Umidade ambiente
    display.print(umidade_min);
    display.print("%");

    display.display();
    delay(2000);
  } else {
    display.clearDisplay();  //Limpa o buffer do display mas sem limpar a tela
    display.display();
  }
}

void setup() {
  randomSeed(analogRead(0));  // Inicialização da função random com uma semente aleatória
  Serial.begin(9600);         //inicio da leitura serial
  dht.begin();                //inicia o dht

  //Parametros LoRa
  LoRa.setTxPower(20);             // Configura o ganho do receptor LoRa para 20dBm, o maior ganho possível (visando maior alcance possível)
  SPI.begin(SCK, MISO, MOSI, SS);  // Inicio do módulo LoRa
  LoRa.setPins(SS, RST, DI00);     // Inicio do módulo LoRa
  LoRa.begin(915E6);               // Inicio do módulo LoRa setando a frequência 915MHz (SX1276 é utilizada no Brasil)
  LoRa.enableCrc();                // inicializa o monitoramento do RSSI
  LoRa.setSpreadingFactor(12);     //Maior possível vai de 7 a 12
  LoRa.setSignalBandwidth(125E3);  // 125 kHz Largura de banda

  //Parametros Display
  Wire.begin(4, 15);                          //inicia display OLED
  display.begin(SSD1306_SWITCHCAPVCC, 0x3c);  //Endereço i2c do display
  display.setTextSize(1);                     //tamanho da fonte no display
  display.setTextColor(WHITE);                //Configura cor da fonte
  display.clearDisplay();                     //Limpa o buffer do display mas sem limpar a tela
  display.display();
}

void loop() {
  display.clearDisplay();  //Limpa o buffer do display mas sem limpar a tela
  display.display();
  relogio();
  envialeituraslora();
  touchdisplay();
  recebepacoterssi();
}

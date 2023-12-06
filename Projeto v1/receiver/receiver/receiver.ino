//Receiver 04/12/2023
// Sistema para recebimento e leituras de Estações de Coleta de Dados e envio para plataforma Tago.Io
//Autor: Davi Gaede Fiusa
//davi.fiusa@ifro.edu.br
//(69) 98472-7951

//bibliotecas utilizadas
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <LoRa.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <PubSubClient.h>

// Definição dos pinos utilizados pelo LORA
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

int rssi;  //recebe o RSSI dos pacotes recebidos dos senders
int datatosend = 666;  //variável para enviar pacote para as estações testar RSSI

unsigned long previousMillis = 0;     //variaveis millis para keep alive client.loop(); do MQTT
const unsigned long interval = 3000;  //variaveis do millis para keep alive client.loop(); do MQTT Intervalo de 3 segundos

unsigned long previousSendMillis = 0;  //variaveis millis para monitorar RSSI nos senders envia pacote a cada 5 minutos
const unsigned long sendInterval = 300000;

int umidade1;
int umidade2;
int umidade3;
int media;
float temperatura;
float umidade;
int minutos;
int horas;
int contenvio;

int E1conterroumidade1;  // Contadores de erro de leituras recebidas de cada sensor quando o sender envia os dados corretos mas o receiver recebe 0 no lugar
int E1conterroumidade2;
int E1conterroumidade3;
int E1conterrotemperatura;
int E1conterroumidade;

int E2conterroumidade1;  // Contadores de erro de leituras recebidas de cada sensor quando o sender envia os dados corretos mas o receiver recebe 0 no lugar
int E2conterroumidade2;
int E2conterroumidade3;
int E2conterrotemperatura;
int E2conterroumidade;

int E3conterroumidade1;  // Contadores de erro de leituras recebidas de cada sensor quando o sender envia os dados corretos mas o receiver recebe 0 no lugar
int E3conterroumidade2;
int E3conterroumidade3;
int E3conterrotemperatura;
int E3conterroumidade;

//variáveis para o display
int e1umidade1;
int e1umidade2;
int e1umidade3;
int e1media;
float e1temperatura;
float e1umidade;
int e1minutos;
int e1horas;
int e1contenvio;
int e1rssi;
int e1somaerro;

int e2umidade1;
int e2umidade2;
int e2umidade3;
int e2media;
float e2temperatura;
float e2umidade;
int e2minutos;
int e2horas;
int e2contenvio;
int e2rssi;
int e2somaerro;


int e3umidade1;
int e3umidade2;
int e3umidade3;
int e3media;
float e3temperatura;
float e3umidade;
int e3minutos;
int e3horas;
int e3contenvio;
int e3rssi;
int e3somaerro;

//variáveis para criação dos objetos JSON
char charE1umidade1[100];
char charE1umidade2[100];
char charE1umidade3[100];
char charE1media[100];
char charE1temperatura[100];
char charE1umidade[100];
char charE1minutos[100];
char charE1horas[100];
char charE1contenvio[100];
char charE1conterroumidade1[100];
char charE1conterroumidade2[100];
char charE1conterroumidade3[100];
char charE1conterrotemperatura[100];
char charE1conterroumidade[100];
char charE1rssi[100];

char charE2umidade1[100];
char charE2umidade2[100];
char charE2umidade3[100];
char charE2media[100];
char charE2temperatura[100];
char charE2umidade[100];
char charE2minutos[100];
char charE2horas[100];
char charE2contenvio[100];
char charE2conterroumidade1[100];
char charE2conterroumidade2[100];
char charE2conterroumidade3[100];
char charE2conterrotemperatura[100];
char charE2conterroumidade[100];
char charE2rssi[100];

char charE3umidade1[100];
char charE3umidade2[100];
char charE3umidade3[100];
char charE3media[100];
char charE3temperatura[100];
char charE3umidade[100];
char charE3minutos[100];
char charE3horas[100];
char charE3contenvio[100];
char charE3conterroumidade1[100];
char charE3conterroumidade2[100];
char charE3conterroumidade3[100];
char charE3conterrotemperatura[100];
char charE3conterroumidade[100];
char charE3rssi[100];

// Configurações da conexão Wi-Fi
const char* ssid = "NOME DO SSID DA REDE WIFI";
const char* password = "SENHA DE ACESSO DA REDE WIFI";

// Configurações do MQTT Broker
const char* mqttServer = "mqtt.tago.io";
const int mqttPort = 1883;
const char* mqttUsername = "Default";
const char* mqttPassword = "22f8f34a-d40c-4f4e-894a-647541a5faf9";
const char* mqttClientName = "TestClient";

// Instância do cliente WiFi
WiFiClient espClient;

// Instância do cliente MQTT
PubSubClient client(espClient);

Adafruit_SSD1306 display(128, 64, &Wire, 16);  //objeto do display, resolução 128x64

void setup() {
  Serial.begin(9600);  // Inicializa a comunicação serial

  // Conecta-se à rede Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConexão Wi-Fi estabelecida!");

  //Parametros MQTT
  client.setServer(mqttServer, mqttPort);  // Configuração do cliente MQTT
  reconnectMQTT();                         // faz reconnect da conexão MQTT

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

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.println("Tentando conectar ao broker MQTT...");
    if (client.connect(mqttClientName, mqttUsername, mqttPassword)) {
      Serial.println("Conexão MQTT estabelecida!");
    } else {
      Serial.print("Falha na conexão MQTT, rc=");
      Serial.print(client.state());
      Serial.println(" Tentando novamente em 5 segundos...");
      delay(5000);
    }
  }
}

void loop() {

  // Laço para verifica se é hora de enviar o pacote de teste RSSI para as estações
    unsigned long currentMillis2 = millis();
    if (currentMillis2 - previousSendMillis >= sendInterval) {
      previousSendMillis = currentMillis2;
  //Envia o pacote LoRa
      LoRa.beginPacket();
      LoRa.print(datatosend);
      LoRa.endPacket();
    }

  // laço para chamar o client.loop() a cada 3 segundos forçando a conexão ficar ativa por keep alive
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    client.loop();
  }

  if (!client.connected()) {
    reconnectMQTT();
  }

  if (LoRa.parsePacket()) {

    umidade1 = 0;  // Limpa as leituras anteriores
    umidade2 = 0;
    umidade3 = 0;
    media = 0;
    temperatura = 0;
    umidade = 0;
    minutos = 0;
    horas = 0;
    contenvio = 0;

    // Recebe a mensagem via LoRa
    String mensagem = "";

    while (LoRa.available()) {
      mensagem += (char)LoRa.read();
    }
    Serial.println(mensagem);
    rssi = LoRa.packetRssi();  // armazena o RSSI do pacote recebido

    StaticJsonDocument<500> doc;
    deserializeJson(doc, mensagem);  // Realiza a análise da mensagem recebida (assumindo formato JSON)

    if (doc["Est"] == 1) {  //Laço para pacotes recebidos do Sender-1

      // Obtém as leituras individuais e armazena em cada variável
      umidade1 = doc["um1"];
      umidade2 = doc["um2"];
      umidade3 = doc["um3"];
      media = doc["md"];
      temperatura = doc["te"];
      umidade = doc["um"];
      minutos = doc["mi"];
      horas = doc["hs"];
      contenvio = doc["co"];

      //variáveis para o display OLED
      e1umidade1 = umidade1;
      e1umidade2 = umidade2;
      e1umidade3 = umidade3;
      e1media = media;
      e1temperatura = temperatura;
      e1umidade = umidade;
      e1minutos = minutos;
      e1horas = horas;
      e1contenvio = contenvio;
      e1rssi = rssi;

      // Criando objetos Json para envio para MQTT
      StaticJsonDocument<300> jsE1umidade1;
      jsE1umidade1["variable"] = "E1umidade1";
      jsE1umidade1["value"] = umidade1;
      serializeJson(jsE1umidade1, charE1umidade1);

      StaticJsonDocument<300> jsE1umidade2;
      jsE1umidade2["variable"] = "E1umidade2";
      jsE1umidade2["value"] = umidade2;
      serializeJson(jsE1umidade2, charE1umidade2);

      StaticJsonDocument<300> jsE1umidade3;
      jsE1umidade3["variable"] = "E1umidade3";
      jsE1umidade3["value"] = umidade3;
      serializeJson(jsE1umidade3, charE1umidade3);

      StaticJsonDocument<300> jsE1media;
      jsE1media["variable"] = "E1media";
      jsE1media["value"] = media;
      serializeJson(jsE1media, charE1media);

      StaticJsonDocument<300> jsE1temperatura;
      jsE1temperatura["variable"] = "E1temperatura";
      jsE1temperatura["value"] = temperatura;
      serializeJson(jsE1temperatura, charE1temperatura);

      StaticJsonDocument<300> jsE1umidade;
      jsE1umidade["variable"] = "E1umidade";
      jsE1umidade["value"] = umidade;
      serializeJson(jsE1umidade, charE1umidade);

      StaticJsonDocument<300> jsE1minutos;
      jsE1minutos["variable"] = "E1minutos";
      jsE1minutos["value"] = minutos;
      serializeJson(jsE1minutos, charE1minutos);

      StaticJsonDocument<300> jsE1horas;
      jsE1horas["variable"] = "E1horas";
      jsE1horas["value"] = horas;
      serializeJson(jsE1horas, charE1horas);

      StaticJsonDocument<300> jsE1contenvio;
      jsE1contenvio["variable"] = "E1contenvio";
      jsE1contenvio["value"] = contenvio;
      serializeJson(jsE1contenvio, charE1contenvio);

      StaticJsonDocument<300> jsE1rssi;
      jsE1rssi["variable"] = "E1rssi";
      jsE1rssi["value"] = rssi;
      serializeJson(jsE1rssi, charE1rssi);

      if (umidade1 >= 1) {                               //Condição necessária para evitar leituras errôneas ou falhas na transmissão pelo LoRa transmitindo 0 como leitura
        publishMQTT("info/E1umidade1", charE1umidade1);  // Envia para MQTT
      } else {
        E1conterroumidade1++;
        StaticJsonDocument<300> jsE1conterroumidade1;
        jsE1conterroumidade1["variable"] = "E1conterroumidade1";
        jsE1conterroumidade1["value"] = E1conterroumidade1;
        serializeJson(jsE1conterroumidade1, charE1conterroumidade1);
        publishMQTT("info/E1conterroumidade1", charE1conterroumidade1);  // Envia para MQTT
      }

      if (umidade2 >= 1) {
        publishMQTT("info/E1umidade2", charE1umidade2);  // Envia para MQTT
      } else {
        E1conterroumidade2++;
        StaticJsonDocument<300> jsE1conterroumidade2;
        jsE1conterroumidade2["variable"] = "E1conterroumidade2";
        jsE1conterroumidade2["value"] = E1conterroumidade2;
        serializeJson(jsE1conterroumidade2, charE1conterroumidade2);
        publishMQTT("info/E1conterroumidade2", charE1conterroumidade2);  // Envia para MQTT
      }

      if (umidade3 >= 1) {
        publishMQTT("info/E1umidade3", charE1umidade3);  // Envia para MQTT
      } else {
        E1conterroumidade3++;
        StaticJsonDocument<300> jsE1conterroumidade3;
        jsE1conterroumidade3["variable"] = "E1conterroumidade3";
        jsE1conterroumidade3["value"] = E1conterroumidade3;
        serializeJson(jsE1conterroumidade3, charE1conterroumidade3);
        publishMQTT("info/E1conterroumidade3", charE1conterroumidade3);  // Envia para MQTT
      }

      if (temperatura >= 1) {
        publishMQTT("info/E1temperatura", charE1temperatura);  // Envia para MQTT
      } else {
        E1conterrotemperatura++;
        StaticJsonDocument<300> jsE1conterrotemperatura;
        jsE1conterrotemperatura["variable"] = "E1conterrotemperatura";
        jsE1conterrotemperatura["value"] = E1conterrotemperatura;
        serializeJson(jsE1conterrotemperatura, charE1conterrotemperatura);
        publishMQTT("info/E1conterrotemperatura", charE1conterrotemperatura);  // Envia para MQTT
      }

      if (umidade >= 1) {
        publishMQTT("info/E1umidade", charE1umidade);  // Envia para MQTT
      } else {
        E1conterroumidade++;
        StaticJsonDocument<300> jsE1conterroumidade;
        jsE1conterroumidade["variable"] = "E1conterroumidade";
        jsE1conterroumidade["value"] = E1conterroumidade;
        serializeJson(jsE1conterroumidade, charE1conterroumidade);
        publishMQTT("info/E1conterroumidade", charE1conterroumidade);  // Envia para MQTT
      }

      publishMQTT("info/E1contenvio", charE1contenvio);  // Envia para MQTT
      publishMQTT("info/E1media", charE1media);          // Envia para MQTT
      publishMQTT("info/E1minutos", charE1minutos);      // Envia para MQTT
      publishMQTT("info/E1horas", charE1horas);          // Envia para MQTT
      publishMQTT("info/E1rssi", charE1rssi);            // Envia para MQTT

      Serial.print("RSSI do pacote recebido da E1: ");
      Serial.println(rssi);
      Serial.print("Cont. Erro Sensor E1-1: ");
      Serial.println(E1conterroumidade1);
      Serial.print("Cont. Erro Sensor E1-2: ");
      Serial.println(E1conterroumidade2);
      Serial.print("Cont. Erro Sensor E1-3: ");
      Serial.println(E1conterroumidade3);
      Serial.print("Cont. Erro Sensor Temperatura: ");
      Serial.println(E1conterrotemperatura);
      Serial.print("Cont. Erro Sensor Umidade: ");
      Serial.println(E1conterroumidade);

      e1somaerro = (E1conterroumidade1 + E1conterroumidade2 + E1conterroumidade3 + E1conterrotemperatura + E1conterroumidade);

    } else if (doc["Est"] == 2) {  //Laço para pacotes recebidos do Sender-2

      // Obtém as leituras individuais e armazena em cada variável
      umidade1 = doc["um1"];
      umidade2 = doc["um2"];
      umidade3 = doc["um3"];
      media = doc["md"];
      temperatura = doc["te"];
      umidade = doc["um"];
      minutos = doc["mi"];
      horas = doc["hs"];
      contenvio = doc["co"];

      //variáveis para o display OLED
      e2umidade1 = umidade1;
      e2umidade2 = umidade2;
      e2umidade3 = umidade3;
      e2media = media;
      e2temperatura = temperatura;
      e2umidade = umidade;
      e2minutos = minutos;
      e2horas = horas;
      e2contenvio = contenvio;
      e2rssi = rssi;


      // Criando objetos Json para envio para MQTT
      StaticJsonDocument<300> jsE2umidade1;
      jsE2umidade1["variable"] = "E2umidade1";
      jsE2umidade1["value"] = umidade1;
      serializeJson(jsE2umidade1, charE2umidade1);

      StaticJsonDocument<300> jsE2umidade2;
      jsE2umidade2["variable"] = "E2umidade2";
      jsE2umidade2["value"] = umidade2;
      serializeJson(jsE2umidade2, charE2umidade2);

      StaticJsonDocument<300> jsE2umidade3;
      jsE2umidade3["variable"] = "E2umidade3";
      jsE2umidade3["value"] = umidade3;
      serializeJson(jsE2umidade3, charE2umidade3);

      StaticJsonDocument<300> jsE2media;
      jsE2media["variable"] = "E2media";
      jsE2media["value"] = media;
      serializeJson(jsE2media, charE2media);

      StaticJsonDocument<300> jsE2temperatura;
      jsE2temperatura["variable"] = "E2temperatura";
      jsE2temperatura["value"] = temperatura;
      serializeJson(jsE2temperatura, charE2temperatura);

      StaticJsonDocument<300> jsE2umidade;
      jsE2umidade["variable"] = "E2umidade";
      jsE2umidade["value"] = umidade;
      serializeJson(jsE2umidade, charE2umidade);

      StaticJsonDocument<300> jsE2minutos;
      jsE2minutos["variable"] = "E2minutos";
      jsE2minutos["value"] = minutos;
      serializeJson(jsE2minutos, charE2minutos);

      StaticJsonDocument<300> jsE2horas;
      jsE2horas["variable"] = "E2horas";
      jsE2horas["value"] = horas;
      serializeJson(jsE2horas, charE2horas);

      StaticJsonDocument<300> jsE2contenvio;
      jsE2contenvio["variable"] = "E2contenvio";
      jsE2contenvio["value"] = contenvio;
      serializeJson(jsE2contenvio, charE2contenvio);

      StaticJsonDocument<300> jsE2rssi;
      jsE2rssi["variable"] = "E2rssi";
      jsE2rssi["value"] = rssi;
      serializeJson(jsE2rssi, charE2rssi);

      if (umidade1 >= 1) {                               //Condição necessária para evitar leituras errôneas ou falhas na transmissão pelo LoRa transmitindo 0 como leitura
        publishMQTT("info/E2umidade1", charE2umidade1);  // Envia para MQTT
      } else {
        E2conterroumidade1++;
        StaticJsonDocument<300> jsE2conterroumidade1;
        jsE2conterroumidade1["variable"] = "E2conterroumidade1";
        jsE2conterroumidade1["value"] = E2conterroumidade1;
        serializeJson(jsE2conterroumidade1, charE2conterroumidade1);
        publishMQTT("info/E2conterroumidade1", charE2conterroumidade1);  // Envia para MQTT
      }

      if (umidade2 >= 1) {
        publishMQTT("info/E2umidade2", charE2umidade2);  // Envia para MQTT
      } else {
        E2conterroumidade2++;
        StaticJsonDocument<300> jsE2conterroumidade2;
        jsE2conterroumidade2["variable"] = "E2conterroumidade2";
        jsE2conterroumidade2["value"] = E2conterroumidade2;
        serializeJson(jsE2conterroumidade2, charE2conterroumidade2);
        publishMQTT("info/E2conterroumidade2", charE2conterroumidade2);  // Envia para MQTT
      }

      if (umidade3 >= 1) {
        publishMQTT("info/E2umidade3", charE2umidade3);  // Envia para MQTT
      } else {
        E2conterroumidade3++;
        StaticJsonDocument<300> jsE2conterroumidade3;
        jsE2conterroumidade3["variable"] = "E2conterroumidade3";
        jsE2conterroumidade3["value"] = E2conterroumidade3;
        serializeJson(jsE2conterroumidade3, charE2conterroumidade3);
        publishMQTT("info/E2conterroumidade3", charE2conterroumidade3);  // Envia para MQTT
      }

      if (temperatura >= 1) {
        publishMQTT("info/E2temperatura", charE2temperatura);  // Envia para MQTT
      } else {
        E2conterrotemperatura++;
        StaticJsonDocument<300> jsE2conterrotemperatura;
        jsE2conterrotemperatura["variable"] = "E2conterrotemperatura";
        jsE2conterrotemperatura["value"] = E2conterrotemperatura;
        serializeJson(jsE2conterrotemperatura, charE2conterrotemperatura);
        publishMQTT("info/E2conterrotemperatura", charE2conterrotemperatura);  // Envia para MQTT
      }

      if (umidade >= 1) {
        publishMQTT("info/E2umidade", charE2umidade);  // Envia para MQTT
      } else {
        E2conterroumidade++;
        StaticJsonDocument<300> jsE2conterroumidade;
        jsE2conterroumidade["variable"] = "E2conterroumidade";
        jsE2conterroumidade["value"] = E2conterroumidade;
        serializeJson(jsE2conterroumidade, charE2conterroumidade);
        publishMQTT("info/E2conterroumidade", charE2conterroumidade);  // Envia para MQTT
      }

      publishMQTT("info/E2contenvio", charE2contenvio);  // Envia para MQTT
      publishMQTT("info/E2media", charE2media);          // Envia para MQTT
      publishMQTT("info/E2minutos", charE2minutos);      // Envia para MQTT
      publishMQTT("info/E2horas", charE2horas);          // Envia para MQTT
      publishMQTT("info/E2rssi", charE2rssi);            // Envia para MQTT


      Serial.print("RSSI do pacote recebido da E2: ");
      Serial.println(rssi);
      Serial.print("Cont. Erro Sensor E2-1: ");
      Serial.println(E2conterroumidade1);
      Serial.print("Cont. Erro Sensor E2-2: ");
      Serial.println(E2conterroumidade2);
      Serial.print("Cont. Erro Sensor E2-3: ");
      Serial.println(E2conterroumidade3);
      Serial.print("Cont. Erro Sensor Temperatura: ");
      Serial.println(E2conterrotemperatura);
      Serial.print("Cont. Erro Sensor Umidade: ");
      Serial.println(E2conterroumidade);

      e2somaerro = (E2conterroumidade1 + E2conterroumidade2 + E2conterroumidade3 + E2conterrotemperatura + E2conterroumidade);

    } else if (doc["Est"] == 3) {  //Laço para pacotes recebidos do Sender-3

      // Obtém as leituras individuais e armazena em cada variável
      umidade1 = doc["um1"];
      umidade2 = doc["um2"];
      umidade3 = doc["um3"];
      media = doc["md"];
      temperatura = doc["te"];
      umidade = doc["um"];
      minutos = doc["mi"];
      horas = doc["hs"];
      contenvio = doc["co"];

      //variáveis para o display OLED
      e3umidade1 = umidade1;
      e3umidade2 = umidade2;
      e3umidade3 = umidade3;
      e3media = media;
      e3temperatura = temperatura;
      e3umidade = umidade;
      e3minutos = minutos;
      e3horas = horas;
      e3contenvio = contenvio;
      e3rssi = rssi;

      // Criando objetos Json para envio para MQTT
      StaticJsonDocument<300> jsE3umidade1;
      jsE3umidade1["variable"] = "E3umidade1";
      jsE3umidade1["value"] = umidade1;
      serializeJson(jsE3umidade1, charE3umidade1);

      StaticJsonDocument<300> jsE3umidade2;
      jsE3umidade2["variable"] = "E3umidade2";
      jsE3umidade2["value"] = umidade2;
      serializeJson(jsE3umidade2, charE3umidade2);

      StaticJsonDocument<300> jsE3umidade3;
      jsE3umidade3["variable"] = "E3umidade3";
      jsE3umidade3["value"] = umidade3;
      serializeJson(jsE3umidade3, charE3umidade3);

      StaticJsonDocument<300> jsE3media;
      jsE3media["variable"] = "E3media";
      jsE3media["value"] = media;
      serializeJson(jsE3media, charE3media);

      StaticJsonDocument<300> jsE3temperatura;
      jsE3temperatura["variable"] = "E3temperatura";
      jsE3temperatura["value"] = temperatura;
      serializeJson(jsE3temperatura, charE3temperatura);

      StaticJsonDocument<300> jsE3umidade;
      jsE3umidade["variable"] = "E3umidade";
      jsE3umidade["value"] = umidade;
      serializeJson(jsE3umidade, charE3umidade);

      StaticJsonDocument<300> jsE3minutos;
      jsE3minutos["variable"] = "E3minutos";
      jsE3minutos["value"] = minutos;
      serializeJson(jsE3minutos, charE3minutos);

      StaticJsonDocument<300> jsE3horas;
      jsE3horas["variable"] = "E3horas";
      jsE3horas["value"] = horas;
      serializeJson(jsE3horas, charE3horas);

      StaticJsonDocument<300> jsE3contenvio;
      jsE3contenvio["variable"] = "E3contenvio";
      jsE3contenvio["value"] = contenvio;
      serializeJson(jsE3contenvio, charE3contenvio);

      StaticJsonDocument<300> jsE3rssi;
      jsE3rssi["variable"] = "E3rssi";
      jsE3rssi["value"] = rssi;
      serializeJson(jsE3rssi, charE3rssi);

      if (umidade1 >= 1) {                               //Condição necessária para evitar leituras errôneas ou falhas na transmissão pelo LoRa transmitindo 0 como leitura
        publishMQTT("info/E3umidade1", charE3umidade1);  // Envia para MQTT
      } else {
        E3conterroumidade1++;
        StaticJsonDocument<300> jsE3conterroumidade1;
        jsE3conterroumidade1["variable"] = "E3conterroumidade1";
        jsE3conterroumidade1["value"] = E3conterroumidade1;
        serializeJson(jsE3conterroumidade1, charE3conterroumidade1);
        publishMQTT("info/E3conterroumidade1", charE3conterroumidade1);  // Envia para MQTT
      }

      if (umidade2 >= 1) {
        publishMQTT("info/E3umidade2", charE3umidade2);  // Envia para MQTT
      } else {
        E3conterroumidade2++;
        StaticJsonDocument<300> jsE3conterroumidade2;
        jsE3conterroumidade2["variable"] = "E3conterroumidade2";
        jsE3conterroumidade2["value"] = E3conterroumidade2;
        serializeJson(jsE3conterroumidade2, charE3conterroumidade2);
        publishMQTT("info/E3conterroumidade2", charE3conterroumidade2);  // Envia para MQTT
      }

      if (umidade3 >= 1) {
        publishMQTT("info/E3umidade3", charE3umidade3);  // Envia para MQTT
      } else {
        E3conterroumidade3++;
        StaticJsonDocument<300> jsE3conterroumidade3;
        jsE3conterroumidade3["variable"] = "E3conterroumidade3";
        jsE3conterroumidade3["value"] = E3conterroumidade3;
        serializeJson(jsE3conterroumidade3, charE3conterroumidade3);
        publishMQTT("info/E3conterroumidade3", charE3conterroumidade3);  // Envia para MQTT
      }

      if (temperatura >= 1) {
        publishMQTT("info/E3temperatura", charE3temperatura);  // Envia para MQTT
      } else {
        E3conterrotemperatura++;
        StaticJsonDocument<300> jsE3conterrotemperatura;
        jsE3conterrotemperatura["variable"] = "E3conterrotemperatura";
        jsE3conterrotemperatura["value"] = E3conterrotemperatura;
        serializeJson(jsE3conterrotemperatura, charE3conterrotemperatura);
        publishMQTT("info/E3conterrotemperatura", charE3conterrotemperatura);  // Envia para MQTT
      }

      if (umidade >= 1) {
        publishMQTT("info/E3umidade", charE3umidade);  // Envia para MQTT
      } else {
        E3conterroumidade++;
        StaticJsonDocument<300> jsE3conterroumidade;
        jsE3conterroumidade["variable"] = "E3conterroumidade";
        jsE3conterroumidade["value"] = E3conterroumidade;
        serializeJson(jsE3conterroumidade, charE3conterroumidade);
        publishMQTT("info/E3conterroumidade", charE3conterroumidade);  // Envia para MQTT
      }

      publishMQTT("info/E3contenvio", charE3contenvio);  // Envia para MQTT
      publishMQTT("info/E3media", charE3media);          // Envia para MQTT
      publishMQTT("info/E3minutos", charE3minutos);      // Envia para MQTT
      publishMQTT("info/E3horas", charE3horas);          // Envia para MQTT
      publishMQTT("info/E3rssi", charE3rssi);            // Envia para MQTT

      Serial.print("RSSI do pacote recebido da E3: ");
      Serial.println(rssi);
      Serial.print("Cont. Erro Sensor E3-1: ");
      Serial.println(E3conterroumidade1);
      Serial.print("Cont. Erro Sensor E3-2: ");
      Serial.println(E3conterroumidade2);
      Serial.print("Cont. Erro Sensor E3-3: ");
      Serial.println(E3conterroumidade3);
      Serial.print("Cont. Erro Sensor Temperatura: ");
      Serial.println(E3conterrotemperatura);
      Serial.print("Cont. Erro Sensor Umidade: ");
      Serial.println(E3conterroumidade);

      e3somaerro = (E3conterroumidade1 + E3conterroumidade2 + E3conterroumidade3 + E3conterrotemperatura + E3conterroumidade);
    }
    client.loop();  // Mantém a conexão MQTT ativa
  }

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

  display.clearDisplay();  //Limpa o buffer do display mas sem limpar a tela Display

  //Linha-1
  display.setCursor(0, linha1);
  display.print("RSSI:");  //Estação
  display.print(e1rssi);
  display.print(" ");
  display.print(e2rssi);
  display.print(" ");
  display.print(e3rssi);

  //Linha-2
  display.setCursor(0, linha2);
  display.print("E1:");  //leitura dos sensores de umidade de solo
  display.print(e1umidade1);
  display.print("% ");
  display.print(e1umidade2);
  display.print("% ");
  display.print(e1umidade3);
  display.print("% ");
  display.print(e1media);
  display.print("%");

  //Linha-3
  display.setCursor(0, linha3);
  display.print("E2:");  //leitura dos sensores de umidade de solo
  display.print(e2umidade1);
  display.print("% ");
  display.print(e2umidade2);
  display.print("% ");
  display.print(e2umidade3);
  display.print("% ");
  display.print(e2media);
  display.print("%");

  //Linha-4
  display.setCursor(0, linha4);
  display.print("E3:");  //leitura dos sensores de umidade de solo
  display.print(e3umidade1);
  display.print("% ");
  display.print(e3umidade2);
  display.print("% ");
  display.print(e3umidade3);
  display.print("% ");
  display.print(e3media);
  display.print("%");

  //Linha-5
  display.setCursor(0, linha5);
  display.print("CE:");
  display.print(e1contenvio);
  display.print(" ");
  display.print(e2contenvio);
  display.print(" ");
  display.print(e3contenvio);

  //Linha-6
  display.setCursor(0, linha6);
  display.print("T:");  //leitura dos sensores de umidade de solo
  display.print(e1temperatura);
  display.print(" ");
  display.print(e2temperatura);
  display.print(" ");
  display.print(e3temperatura);

  //Linha-7
  display.setCursor(0, linha7);
  display.print("U:");  //leitura dos sensores de umidade de solo
  display.print(e1umidade);
  display.print(" ");
  display.print(e2umidade);
  display.print(" ");
  display.print(e3umidade);

  //Linha-8
  display.setCursor(0, linha8);
  display.print("Erros:");  //leitura dos sensores de umidade de solo
  display.print(e1somaerro);
  display.print(" ");
  display.print(e2somaerro);
  display.print(" ");
  display.print(e3somaerro);

  display.display();
}


// Função para publicar mensagens no MQTT Broker
void publishMQTT(const char* topic, char* payload) {
  if (client.connected()) {
    client.publish(topic, payload);
  }
}

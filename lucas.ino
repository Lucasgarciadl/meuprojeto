
#include <ArduinoJson.h>
#include "BluetoothSerial.h"
#include "FS.h"
#include "SPIFFS.h"
BluetoothSerial SerialBT;
//faz o controle do temporizador (interrupção por tempo)
hw_timer_t *timer = NULL;

//função que o temporizador irá chamar, para reiniciar o ESP32
void IRAM_ATTR resetModule() {
  ets_printf("(watchdog) reiniciar\n"); //imprime no log
  esp_restart(); //reinicia o chip
}

//função que o configura o temporizador
void configureWatchdog()
{
  timer = timerBegin(0, 80, true); //timerID 0, div 80
  //timer, callback, interrupção de borda
  timerAttachInterrupt(timer, &resetModule, true);
  //timer, tempo (us), repetição
  timerAlarmWrite(timer, 5000000, true);
  timerAlarmEnable(timer); //habilita a interrupção //enable interrupt
}

uint64_t chipid;
#define ledPin 2
int ledState = LOW;
unsigned long previousMillis = 0;
const long interval = 1000;
String conteudo;
String tratamento;
char caractere;
//ID DO CHIP
void chip_id() {
  chipid = ESP.getEfuseMac();
  Serial.printf("ESP32 Chip ID = %04X", (uint16_t)(chipid >> 32));
  Serial.printf("%08X\n", (uint32_t)chipid);
  delay(3000);
}
//BLINK
void pisca() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;

    // if the LED is off turn it on and vice-versa:
    if (ledState == LOW) {
      ledState = HIGH;
    } else {
      ledState = LOW;
    }

    // set the LED with the ledState of the variable:
    digitalWrite(ledPin, ledState);
  }

}

//STRUTURAÇÂO
struct dados1 {

  String Type;
  String last_update;
  String id;
  String municipio;
  int barramento;
  String situacao;
  String logradouro;
  String lon;
  String lat;

};
//END

dados1 variavel;

void writeFile(String state, String path) {
  //Abre o arquivo para escrita ("w" write)
  //Sobreescreve o conteúdo do arquivo
  File rFile = SPIFFS.open("/state.txt", FILE_WRITE);
  if (!rFile) {
    Serial.println("Erro ao abrir arquivo!");
  } else {
    rFile.println(state);
    Serial.print("gravou estado: ");
    Serial.println(state);
  }
  rFile.close();
}

String readFile(String path) {
  File rFile = SPIFFS.open("/state.txt");
  if (!rFile) {
    Serial.println("Erro ao abrir arquivo!");
  }
  String content = rFile.readStringUntil('\r'); //desconsidera '\r\n'
  Serial.print("leitura de estado: ");
  Serial.println(content);
  rFile.close();
  return content;
}

void openFS(void) {
  //Abre o sistema de arquivos
  if (!SPIFFS.begin()) {
    Serial.println("\nErro ao abrir o sistema de arquivos");
  } else {
    Serial.println("\nSistema de arquivos aberto com sucesso!");
  }
}

void leitura_fs() {
  tratamento = readFile("/state.txt");
  StaticJsonDocument <1024> doc;
  deserializeJson(doc, tratamento);
  JsonObject root = doc.as<JsonObject>();
  String type = root["type"];
  String last_update = root ["last_update"];
  String id = root ["id"];
  String municipio = root ["municipio"];
  int barramento = root ["barramento"];
  String situacao = root ["situação"];
  String logradouro = root ["logradouro"];
  String lon = root ["lon"];
  String lat = root ["lat"];
  variavel.Type = type;
  variavel.last_update = last_update;
  variavel.id = id;
  variavel.municipio = municipio;
  variavel.barramento = barramento;
  variavel.situacao = situacao;
  variavel.logradouro = logradouro;
  variavel.lon = lon;
  variavel.lat = lat;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  SerialBT.begin("CADIC");
  delay(10);
  openFS();
  leitura_fs();
  pinMode(ledPin, OUTPUT);//Define o LED Onboard como saida.
  configureWatchdog();
}

//INFOJSON
void infojson() {
  StaticJsonDocument<1024> doc;
  JsonObject root = doc.to<JsonObject>();
  root ["type"] = "inforeply";
  root ["last_update"] = variavel.last_update;
  root ["id"] = variavel.id;
  root ["municipio"] = variavel.municipio ;
  root ["barramento"] = variavel.barramento;
  root ["situacao"] = variavel.situacao;
  root ["logradouro"] = variavel.logradouro;
  root ["lon"] = variavel.lon ;
  root ["lat"] = variavel.lat ;
  serializeJsonPretty(root, SerialBT);
}

//UPDATE JSON
void updatejson() {
  StaticJsonDocument<1024> doc;
  JsonObject root = doc.to<JsonObject>();
  root ["type"] = "updatereply";
  root ["last_update"] = variavel.last_update;
  serializeJsonPretty(root, SerialBT);
  delay(20);
}




//LEITURA DO QUE SE RECEBE PELA SERIAL
void leitura() {

  while (SerialBT.available() > 0) {
    caractere = SerialBT.read();
    if (caractere != '\n') {
      conteudo.concat(caractere);
      delay(1);
    }
    conteudo.trim();
    tratamento = conteudo;
  }
  StaticJsonDocument <1024> doc;
  deserializeJson(doc, conteudo);

  JsonObject root = doc.as<JsonObject>();
  String type = root["type"];
  String last_update = root ["last_update"];
  String id = root ["id"];
  String municipio = root ["municipio"];
  int barramento = root ["barramento"];
  String situacao = root ["situação"];
  String logradouro = root ["logradouro"];
  String lon = root ["lon"];
  String lat = root ["lat"];
  type.trim();
  if (type == "info") {

    infojson();
  }
  if (type == "update") {
    writeFile(tratamento, "/state.txt");
    variavel.Type = type;
    variavel.last_update = last_update;
    variavel.id = id;
    variavel.municipio = municipio;
    variavel.barramento = barramento;
    variavel.situacao = situacao;
    variavel.logradouro = logradouro;
    variavel.lon = lon;
    variavel.lat = lat;
    Serial.println(tratamento);
    updatejson();
  }
  conteudo = "";
}

void loop() {
  timerWrite(timer, 0);
  pisca();
  leitura();



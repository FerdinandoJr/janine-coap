#include "coap-simple.h"
#include "WiFi.h"
#include "WiFiUdp.h"

// ——— Pinos do ultrassônico ———
#define TRIG_PIN 13   // GPIO 13 – saída TRIG
#define ECHO_PIN 35   // GPIO 35 – entrada ECHO

// ——— Pinos do sensor de umidade MH-Sensor-Series ———
#define HUMIDITY_DIGITAL_PIN 12  // D0 → GPIO 12 (digital)
#define HUMIDITY_ANALOG_PIN  34  // A0 → GPIO 34 (analógico)

// ——— Credenciais Wi‑Fi ———
const char* ssid     = "Ap 101";
const char* password = "12345687";

// ——— IP e porta do servidor CoAP ———
IPAddress coapServerIP(192,168,100,30);
const int  coapServerPort = COAP_DEFAULT_PORT;   // normalmente 5683

WiFiUDP udp;
Coap    coap(udp);

// ——— Protótipos ———
void initSerial();
void connectWifi();
void initCoap();
void initUltrasonic();
void initHumiditySensor();
void loopCoap();
float readDistanceCM();
int   readHumidityDigital();
int   readHumidityAnalog();
void  sendDistance();
void sendHumidity();

void setup() {
  initSerial();
  connectWifi();
  initCoap();
  initUltrasonic();
  initHumiditySensor();
}

void loop() {
  loopCoap();

  // — Leitura dos sensores —
  float dist            = readDistanceCM();
  int   humidAnalogRaw  = readHumidityAnalog();
  int   humidDigitalRaw = readHumidityDigital();

  // Exibe leituras no console (debug)
  Serial.printf("Distância: %.2f cm | Umidade(A0): %d | Umidade(D0): %d\n",
                dist, humidAnalogRaw, humidDigitalRaw);

  // Envia apenas distância (você adicionará a umidade quando quiser)
  sendDistance();
  sendHumidity();

  delay(2000);
}

// ————————————————— Funções auxiliares —————————————————

void initSerial() {
  Serial.begin(115200);
  delay(100);
  Serial.println();
}

void connectWifi() {
  Serial.println("[Wi-Fi] Conectando...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(500);
  }
  Serial.println("\n[Wi-Fi] Conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void initCoap() {
  Serial.println("[CoAP] Iniciando...");
  if (!coap.start()) {
    Serial.println("[CoAP] Falha ao iniciar!");
    while (true) {
      delay(1000);  // trava aqui em caso de erro
    }
  }
  Serial.println("[CoAP] Iniciado com sucesso");
}

void loopCoap() {
  // processa retransmissões, ACKs e callbacks pendentes
  coap.loop();
}

// ——— Ultrassônico ———
void initUltrasonic() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);
}

float readDistanceCM() {
  // Pulso de 10 µs
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH);
  // Som ~343 m/s ⇒ 0,034 cm/µs, ida e volta ⇒ ÷2
  return (duration * 0.034) / 2.0;
}

void sendDistance() {
  float dist = readDistanceCM();
  char payload[16];
  dtostrf(dist, 6, 2, payload);  // ex.: " 45.68"

  uint16_t msgId = coap.send(
    coapServerIP,
    coapServerPort,
    "distance",              // recurso URI
    COAP_NONCON,              // NON‑confirmável
    COAP_POST,
    nullptr,
    0,
    (const uint8_t*)payload,
    strlen(payload),
    COAP_TEXT_PLAIN
  );
  Serial.printf("[CoAP] MsgID=%u enviado p/ /distance, payload='%s'\n", msgId, payload);
}

// ——— Sensor de umidade ———
void initHumiditySensor() {
  pinMode(HUMIDITY_DIGITAL_PIN, INPUT);
  // HUMIDITY_ANALOG_PIN não requer pinMode()
}

int readHumidityDigital() {
  return digitalRead(HUMIDITY_DIGITAL_PIN);  // 0 = úmido, 1 = seco
}

int readHumidityAnalog() {
  return analogRead(HUMIDITY_ANALOG_PIN);    // 0‑4095 (ESP32 ADC)
}

//  Exemplo de função de envio (você pode adaptar):
void sendHumidity() {
  int humid = readHumidityAnalog();
  float umidadePercentual = 100.0 - ((humid / 4095.0) * 100.0);
  
  char payload[8];
  itoa(umidadePercentual, payload, 10);
  
  uint16_t msgId = coap.send(
    coapServerIP,
    coapServerPort, 
    "humidity",
    COAP_NONCON, 
    COAP_POST,
    nullptr, 
    0,
    (const uint8_t*)payload,
    strlen(payload), 
    COAP_TEXT_PLAIN
  );
  
  Serial.printf("[CoAP] MsgID=%u enviado p/ /humidity, payload='%s'\n", msgId, payload);
}

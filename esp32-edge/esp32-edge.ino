#include <coap-simple.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#define TRIG_PIN 13   // GPIO 13 (D13) – saída para o TRIG
#define ECHO_PIN 35   // GPIO 35 (D35) – entrada para o ECHO

// Credenciais Wi-Fi
const char* ssid     = "mateus_p";
const char* password = "12345678";

// IP e porta do servidor CoAP
IPAddress coapServerIP(192,168,212,146);
const int    coapServerPort = COAP_DEFAULT_PORT; // geralmente 5683

WiFiUDP udp;
Coap    coap(udp);

// Protótipos
void initSerial();
void connectWifi();
void initCoap();
void initUltrasonic();
void loopCoap();
float readDistanceCM();
void sendDistance();

void setup() {
  initSerial();
  connectWifi();
  initCoap();
  initUltrasonic();
}

void loop() {
  loopCoap();
  sendDistance();
  delay(2000);
}

// ——— Funções auxiliares ———

void initSerial() {
  Serial.begin(115200);
  delay(100);
  Serial.println();
}

void connectWifi() {
  Serial.println("[Wi-Fi] Conectando...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
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
      delay(1000); // trava aqui em caso de erro
    }
  }
  Serial.println("[CoAP] Iniciado com sucesso");
}


void loopCoap() {
  // processa retransmissões, ACKs e callbacks pendentes
  coap.loop();
}


void initUltrasonic() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);
}

float readDistanceCM() {
  // Emite pulso de 10µs
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH);
  // velocidade do som ~343m/s ⇒ 0.034 cm/µs, ida e volta ⇒ divide por 2
  return (duration * 0.034) / 2.0;
}

void sendDistance() {
  float dist = readDistanceCM();
  // Converte float para string (até 6 dígitos, 2 decimais)
  char payload[16];
  dtostrf(dist, 6, 2, payload);

  uint16_t msgId = coap.send(
    coapServerIP,
    coapServerPort,
    "distance",                 // recurso URI
    COAP_NONCON,                // tipo NON-confirmável
    COAP_PUT,                   // método PUT
    nullptr,                    // sem token
    0,                          // tamanho do token
    (const uint8_t*)payload,    // payload
    strlen(payload),            // tamanho do payload
    COAP_TEXT_PLAIN             // content format
  );

  Serial.printf("[CoAP] MsgID=%u enviado p/ /distance, payload='%s'\n", msgId, payload);
}

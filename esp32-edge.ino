#include <coap-simple.h>
#include <WiFi.h>
#include <WiFiUdp.h>

// Credenciais Wi-Fi
const char* ssid     = "mateus_p";
const char* password = "12345678";

// IP e porta do servidor CoAP
// 192.168.212.146
IPAddress coapServerIP(192,168,212,146);  // ajuste para o IP do seu servidor
const int    coapServerPort = COAP_DEFAULT_PORT; // geralmente 5683

WiFiUDP udp;
Coap    coap(udp);

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n[Setup] Conectando ao Wi-Fi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\n Wi-Fi conectado!");
  Serial.print("IP: "); Serial.println(WiFi.localIP());

  // Inicializa CoAP (abre socket UDP na porta padrão)
  if (!coap.start()) {
    Serial.println("Falha ao iniciar CoAP");
    while (true) { delay(1000); }
  }
  Serial.println("CoAP iniciado");
}

void loop() {
  // processa respostas ou callbacks pendentes
  coap.loop();

  // monta o payload
  const char* msg = "Hello World";
  size_t      len = strlen(msg);

  // envia um NON-CON PUT para /alive
  uint16_t msgId = coap.send(
    coapServerIP,
    coapServerPort,
    "alive",             // recurso URI
    COAP_NONCON,         // tipo NON-confirmável
    COAP_PUT,            // método PUT
    nullptr,             // sem token
    0,                   // tamanho do token
    (const uint8_t*)msg, // payload
    len,                 // tamanho do payload
    COAP_TEXT_PLAIN      // content format
  );

  Serial.printf("[CoAP] Enviado MsgID=%u para %s, payload='%s'\n", msgId, "/alive", msg);

  delay(5000);
}
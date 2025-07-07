#define COAP_BUF_MAX_SIZE 10240

#include "coap-simple.h"
#include "WiFi.h"
#include "WiFiUdp.h"
#include "esp_camera.h"
#include "HTTPClient.h"

#define CAMERA_MODEL_AI_THINKER
#if defined(CAMERA_MODEL_AI_THINKER)
#define PWDN_GPIO_NUM    32
#define RESET_GPIO_NUM   -1
#define XCLK_GPIO_NUM     0
#define SIOD_GPIO_NUM    26
#define SIOC_GPIO_NUM    27

#define Y9_GPIO_NUM      35
#define Y8_GPIO_NUM      34
#define Y7_GPIO_NUM      39
#define Y6_GPIO_NUM      36
#define Y5_GPIO_NUM      21
#define Y4_GPIO_NUM      19
#define Y3_GPIO_NUM      18
#define Y2_GPIO_NUM       5
#define VSYNC_GPIO_NUM   25
#define HREF_GPIO_NUM    23
#define PCLK_GPIO_NUM    22
#endif

// Credenciais Wi-Fi
const char* ssid     = "Ap 101";
const char* password = "12345687";

// —————— CONFIGURAÇÃO DO COAP ——————
WiFiUDP udp;
Coap coap(udp);
IPAddress coapServerIP(192,168,100,27); /* PC FERDINANDO: 192.168.100.27 */
const int coapServerPort = COAP_DEFAULT_PORT;

#define CAMERA_MODEL_AI_THINKER

void initSerial();
void initCamera();
void connectWifi();
void initCoapServer();
void distanceHandler(CoapPacket &packet, IPAddress ip, int port);
void humidityHandler(CoapPacket &packet, IPAddress ip, int port);
void sendImage();

void scanNetworks() {
  int n = WiFi.scanNetworks();
  Serial.printf("Scan completo: %d redes encontradas\n", n);
  for (int i = 0; i < n; ++i) {
    Serial.printf("%d: %s (RSSI: %d dBm)%s\n",
                  i+1,
                  WiFi.SSID(i).c_str(),
                  WiFi.RSSI(i),
                  WiFi.encryptionType(i)==WIFI_AUTH_OPEN ? " [aberta]" : "");
  }
}


void setup() {
  initSerial();
  scanNetworks();
  connectWifi();
  initCoapServer();
  initCamera();
}

void loop() {
  coap.loop();
  delay(5000);
}

void initSerial() {
  Serial.begin(115200);
  delay(1000); // espera Serial estabilizar
  Serial.println();
}

void connectWifi() {
  Serial.println("[Wi-Fi] Conectando...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    // trocamos Serial.print por Serial.printf
    Serial.printf("[WI-F] Não conectado. Tentando novamente (\"%s\", \"%s\") status = %d \n", ssid, password, WiFi.status());
    delay(1000);
  }
  Serial.println("\n[Wi-Fi] Conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}


void initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA;  // 320x240
  config.jpeg_quality = 12;
  config.fb_count = 1;
  config.grab_mode = CAMERA_GRAB_LATEST;
  config.fb_location = CAMERA_FB_IN_PSRAM;

  Serial.println("[CAM] Inicializando a camera...");
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Erro ao inicializar a camera: 0x%x\n", err);
  } else {
    Serial.printf("[CAM] camera iniciada com sucesso!");  
  }

   
}

void initCoapServer() {
  Serial.println("[CoAP] Iniciando...");
  if (!coap.start()) {
    Serial.println("[CoAP] Falha ao iniciar!");
  }

  Serial.println("[CoAP] escutando /distance");
  coap.server(distanceHandler, "distance");  // Registra o handler separado para o recurso "/distance"
  Serial.println("[CoAP] escutando /humidity");
  coap.server(humidityHandler, "humidity");  // Registra o handler separado para o recurso "/humidity"
  
  Serial.println("[CoAP] Iniciado com sucesso");
}


// ————— Função que trata o recurso "/distance" —————
void distanceHandler(CoapPacket &packet, IPAddress ip, int port) {
  // 1) Copia payload para buffer C-string e garante terminação
  char buf[16] = {0};
  int len = packet.payloadlen;
  if (len > (int)sizeof(buf) - 1) {
    len = sizeof(buf) - 1;
  }
  memcpy(buf, packet.payload, len);
  buf[len] = '\0';

  // 2) Converte para double e imprime
  double distance = atof(buf);
  Serial.printf("Distância recebida: %.2f cm\n", distance);

  // 3) Envia de volta *só* o buf, com o tamanho exato
  uint16_t msgId = coap.send(
    coapServerIP,
    coapServerPort,
    "distance",       // recurso URI
    COAP_NONCON,      // NON-confirmável
    COAP_POST,
    nullptr,
    0,
    (uint8_t*)buf,    // usa buf em vez de packet.payload
    len,              // usa len em vez de packet.payloadlen
    COAP_TEXT_PLAIN
  );

  // 4) Imprime o buf
  Serial.printf("[CoAP] MsgID=%u enviado p/ /distance, payload='%s'\n",
                msgId, buf);

  // 5) Chama sua rotina de imagem
  if (distance < 15) {
    sendImage();
  }
  
}


// ————— Função que trata o recurso "/humidity" —————
void humidityHandler(CoapPacket &packet, IPAddress ip, int port) {
  // 1) Copia payload para buffer C-string
  char buf[16] = {0};
  int len = packet.payloadlen;
  if (len > (int)sizeof(buf)-1) len = sizeof(buf)-1;
  memcpy(buf, packet.payload, len);
  buf[len] = '\0';

  // 2) Converte para double e imprime corretamente
  double humidity = atof(buf);
  Serial.printf("Umidade recebida: %.2f %%\n", humidity);

  // 3) Envia de volta *só* o buf, com o tamanho exato
  uint16_t msgId = coap.send(
    coapServerIP,
    coapServerPort, 
    "humidity",
    COAP_NONCON, 
    COAP_POST,
    nullptr, 
    0,
    (uint8_t*)buf,   // payload pointer
    len,            // payload length
    COAP_TEXT_PLAIN
  );  

  // 4) Imprime o buf, não packet.payload
  Serial.printf("[CoAP] MsgID=%u enviado p/ /humidity, payload='%s'\n", 
                msgId, buf);
}


void sendImage() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Falha ao capturar imagem");
    return;
  }

  size_t chunkSize = 1024;
  size_t payloadLen = fb->len;
  uint8_t *payload = fb->buf;

  uint8_t buffer[chunkSize + 1];  // 1 byte para chunk ID

  for (size_t offset = 0; offset < payloadLen; offset += chunkSize) {
    size_t len = chunkSize;
    if (offset + len > payloadLen) len = payloadLen - offset;

    buffer[0] = (uint8_t)(offset / chunkSize);  // chunk ID
    memcpy(buffer + 1, payload + offset, len);

    uint16_t msgId = coap.send(
      coapServerIP,
      coapServerPort,
      "image",
      COAP_NONCON,
      COAP_POST,
      nullptr,
      0,
      buffer,
      len + 1,
      COAP_APPLICATION_OCTET_STREAM
    );

    Serial.printf("[COAP] MsgId=%d, chunk=%d, size=%d\n", msgId, buffer[0], (int)len);
    delay(50);
  }

  // 🔥 Envia o chunk final EOF
  uint8_t eofMarker[1] = {255}; // chunk ID especial
  uint16_t msgId = coap.send(
    coapServerIP,
    coapServerPort,
    "image",
    COAP_NONCON,
    COAP_POST,
    nullptr,
    0,
    eofMarker,
    1,
    COAP_APPLICATION_OCTET_STREAM
  );
  Serial.printf("[COAP] MsgId=%d, EOF enviado\n", msgId);


  // Envia a imagem via HTTP POST
  //    const char* serverUrl = "http://192.168.100.27:3000/image";
  //
  //    HTTPClient http;
  //    http.begin(serverUrl);
  //    http.addHeader("Content-Type", "image/jpeg");
  //    
  //    int httpResponseCode = http.POST(fb->buf, fb->len);
  //    
  //    if (httpResponseCode == HTTP_CODE_OK) {
  //      Serial.println("Imagem enviada com sucesso!");
  //    } else {
  //      Serial.printf("Erro no envio: %d\n", httpResponseCode);
  //    }
  //
  //    http.end();    

  // Envia a imagem via CoAP

  // Libera o buffer da imagem
  esp_camera_fb_return(fb);  
}

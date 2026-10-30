#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

const int PINO_LED = 19;
const uint8_t CANAL = 1;

// SUBSTITUA pelo MAC STA da placa B.
uint8_t MAC_B[] = {0x24, 0x6F, 0x28, 0x00, 0x00, 0x02};

struct Mensagem {
  char tipo;
  uint16_t adc;
  char texto[24];
};

bool estadoLED = false;

void verificar(esp_err_t resultado, const char *etapa) {
  if (resultado != ESP_OK) {
    Serial.printf("Erro em %s: %s\n",
                  etapa, esp_err_to_name(resultado));

    while (true) {
      delay(1000);
    }
  }
}

void receberMensagem(const esp_now_recv_info_t *info,
                     const uint8_t *dados, int tamanho) {
  if (tamanho != sizeof(Mensagem)) {
    return;
  }

  if (memcmp(info->src_addr, MAC_B, 6) != 0) {
    return;
  }

  Mensagem recebida = {};
  memcpy(&recebida, dados, sizeof(recebida));
  recebida.texto[sizeof(recebida.texto) - 1] = '\0';

  if (recebida.tipo != 'D') {
    return;
  }

  Serial.printf("C: recebeu %s | ADC=%u\n",
                recebida.texto, recebida.adc);

  estadoLED = !estadoLED;
  digitalWrite(PINO_LED, estadoLED);

  Mensagem resposta = {};
  resposta.tipo = 'R';
  resposta.adc = recebida.adc;

  snprintf(resposta.texto, sizeof(resposta.texto),
           "Recebido em C");

  // C responde somente para B.
  esp_err_t resultado = esp_now_send(
    MAC_B,
    reinterpret_cast<const uint8_t *>(&resposta),
    sizeof(resposta)
  );

  if (resultado == ESP_OK) {
    Serial.println("C -> B | resposta solicitada");
  } else {
    Serial.printf("Erro ao enviar resposta: %s\n",
                  esp_err_to_name(resultado));
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(PINO_LED, OUTPUT);
  digitalWrite(PINO_LED, LOW);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  Serial.print("MAC STA da placa C: ");
  Serial.println(WiFi.macAddress());

  verificar(
    esp_wifi_set_channel(CANAL, WIFI_SECOND_CHAN_NONE),
    "configurar canal"
  );

  verificar(esp_now_init(), "iniciar ESP-NOW");

  esp_now_peer_info_t par = {};
  memcpy(par.peer_addr, MAC_B, 6);
  par.channel = CANAL;
  par.ifidx = WIFI_IF_STA;
  par.encrypt = false;

  verificar(esp_now_add_peer(&par), "adicionar B");

  verificar(
    esp_now_register_recv_cb(receberMensagem),
    "registrar recebimento"
  );

  Serial.println("Placa C pronta para receber e responder.");
}

void loop() {
  // O recebimento e a resposta ocorrem no callback.
  delay(10);
}
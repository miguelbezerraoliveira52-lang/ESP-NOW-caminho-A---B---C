#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

const int PINO_LED = 18;
const uint8_t CANAL = 1;

// SUBSTITUA pelos MACs STA das placas A e C.
uint8_t MAC_A[] = {0x24, 0x6F, 0x28, 0x00, 0x00, 0x01};
uint8_t MAC_C[] = {0x24, 0x6F, 0x28, 0x00, 0x00, 0x03};

struct Mensagem {
  char tipo;
  uint16_t adc;
  char texto[24];
};

bool estadoLED = false;

void verificar(esp_err_t resultado, const char *etapa) {
  if (resultado != ESP_OK) {
    Serial.printf("Erro em %s: %s\n", etapa, esp_err_to_name(resultado));

    while (true) {
      delay(1000);
    }
  }
}

void adicionarPar(const uint8_t *mac) {
  esp_now_peer_info_t par = {};
  memcpy(par.peer_addr, mac, 6);
  par.channel = CANAL;
  par.ifidx = WIFI_IF_STA;
  par.encrypt = false;

  verificar(esp_now_add_peer(&par), "adicionar par");
}

void receberMensagem(const esp_now_recv_info_t *info,const uint8_t *dados, int tamanho) {
  if (tamanho != sizeof(Mensagem)) {
    return;
  }

  Mensagem mensagem = {};
  memcpy(&mensagem, dados, sizeof(mensagem));
  mensagem.texto[sizeof(mensagem.texto) - 1] = '\0';

  const uint8_t *destino = nullptr;

  if (mensagem.tipo == 'D' &&
      memcmp(info->src_addr, MAC_A, 6) == 0) {
    destino = MAC_C;
    Serial.println("B: A -> C | proximo=C");

  } else if (mensagem.tipo == 'R' && memcmp(info->src_addr, MAC_C, 6) == 0) {
    destino = MAC_A;
    Serial.println("B: C -> A | proximo=A");

  } else {
    return;
  }

  esp_err_t resultado = esp_now_send(
    destino,
    reinterpret_cast<const uint8_t *>(&mensagem),
    sizeof(mensagem)
  );

  if (resultado == ESP_OK) {
    estadoLED = !estadoLED;
    digitalWrite(PINO_LED, estadoLED);
  } else {
    Serial.printf("Erro ao encaminhar: %s\n",esp_err_to_name(resultado));
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

  Serial.print("MAC STA da placa B: ");
  Serial.println(WiFi.macAddress());

  verificar(
    esp_wifi_set_channel(CANAL, WIFI_SECOND_CHAN_NONE),
    "configurar canal"
  );

  verificar(esp_now_init(), "iniciar ESP-NOW");

  adicionarPar(MAC_A);
  adicionarPar(MAC_C);

  verificar(
    esp_now_register_recv_cb(receberMensagem),
    "registrar recebimento"
  );

  Serial.println("Placa B pronta para encaminhar mensagens.");
}

void loop() {
  // O recebimento e o encaminhamento ocorrem no callback.
  delay(10);
}
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

const int PINO_LDR = 34;
const uint8_t CANAL = 1;

// SUBSTITUA pelo MAC STA da placa B.
uint8_t MAC_B[] = {0x24, 0x6F, 0x28, 0x00, 0x00, 0x02};

struct Mensagem {
  char tipo;
  uint16_t adc;
  char texto[24];
};

void verificar(esp_err_t resultado, const char *etapa) {
  if (resultado != ESP_OK) {
    Serial.printf("Erro em %s: %s\n", etapa, esp_err_to_name(resultado));

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

  // A aceita mensagens somente de B.
  if (memcmp(info->src_addr, MAC_B, 6) != 0) {
    return;
  }

  Mensagem resposta = {};
  memcpy(&resposta, dados, sizeof(resposta));
  resposta.texto[sizeof(resposta.texto) - 1] = '\0';

  if (resposta.tipo == 'R') {
    Serial.printf("A: ACK de C via B | ADC=%u | %s\n",
                  resposta.adc, resposta.texto);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(100);
  delay(1000);

  pinMode(PINO_LDR, INPUT);
  analogReadResolution(12);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  Serial.print("MAC STA da placa A: ");
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

  Serial.println("Placa A pronta.");
  Serial.println("Digite uma frase e pressione Enter.");
  Serial.println("Use a opcao Nova linha no Monitor Serial.");
  Serial.println("Aguarde o ACK antes de enviar outra mensagem.");
}

void loop() {
  if (Serial.available() > 0) {
    String frase = Serial.readStringUntil('\n');
    frase.trim();

    if (frase.length() == 0) {
      return;
    }

    Mensagem mensagem = {};
    mensagem.tipo = 'D';
    mensagem.adc = analogRead(PINO_LDR);

    // Reserva um byte para o terminador '\0'.
    frase.toCharArray(mensagem.texto, sizeof(mensagem.texto));

    if (frase.length() >= sizeof(mensagem.texto)) {
      Serial.println("Aviso: texto limitado a 23 bytes.");
    }

    esp_err_t resultado = esp_now_send(
      MAC_B,
      reinterpret_cast<const uint8_t *>(&mensagem),
      sizeof(mensagem)
    );

    if (resultado == ESP_OK) {
      Serial.printf("A -> B | texto=%s | ADC=%u\n", mensagem.texto, mensagem.adc);
      Serial.println("Envio solicitado. Aguardando ACK de C...");
    } else {
      Serial.printf("Erro ao solicitar envio: %s\n",esp_err_to_name(resultado));
    }
  }
}
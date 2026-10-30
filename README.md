# 📡 Prática — Comunicação ESP-NOW com Retransmissão

Projeto desenvolvido para a disciplina de **Sistemas Ciberfísicos e Colaborativos**, da **Universidade de Fortaleza — Unifor**.

A atividade utiliza três placas ESP32 para demonstrar o encaminhamento de mensagens por um nó intermediário, com leitura de luminosidade e confirmação de recebimento.

## 🎯 Objetivo

Implementar uma comunicação com rota fixa, na qual:

- A placa **A** lê um sensor LDR e envia texto e leitura ADC para **B**.
- A placa **B** encaminha os dados para **C**.
- A placa **C** recebe a mensagem e gera uma confirmação.
- A confirmação retorna para **A**, passando novamente por **B**.

**Caminho dos dados:** A → B → C  
**Caminho da confirmação:** C → B → A

O projeto representa uma **pseudo-mesh estática**, sem descoberta automática de rotas.

## 🛠️ Materiais utilizados

| Quantidade | Componente |
|---|---|
| 3 | Placas ESP32 |
| 3 | Cabos USB para alimentação e programação |
| 1 | Sensor LDR |
| 1 | Resistor de 10 kΩ |
| Conforme necessário | Protoboard e jumpers |
| 1 — opcional | LED amarelo para a placa B |
| 1 — opcional | LED verde para a placa C |
| 2 — opcionais | Resistores de 220 Ω a 330 Ω para os LEDs |

As placas se comunicam por rádio, sem fios de dados entre elas e sem necessidade de conexão a um roteador.

## 💻 Ambiente de desenvolvimento

- **IDE:** Arduino IDE.
- **Linguagem:** C++ com framework Arduino.
- **Pacote de placas:** esp32 by Espressif Systems, versão 3.x.
- **Comunicação:** ESP-NOW.
- **Canal Wi-Fi:** 1.
- **Monitor Serial:** 115200 baud.

Bibliotecas utilizadas:

```cpp
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
```

Essas bibliotecas acompanham o pacote ESP32.

> Os códigos utilizam a assinatura de recebimento com `esp_now_recv_info_t`. Versões antigas do pacote ESP32 podem exigir adaptação.

## 📂 Organização dos códigos

| Projeto | Arquivo | Responsabilidade |
|---|---|---|
| Placa_A | `Placa_A.ino` | Leitura do LDR e envio da mensagem |
| Placa_B | `Placa_B.ino` | Encaminhamento nos dois sentidos |
| Placa_C | `Placa_C.ino` | Recebimento e geração da confirmação |

Cada arquivo deve ficar em uma pasta com o mesmo nome do sketch, sem a extensão `.ino`.

**Os três códigos são projetos independentes. Não devem ser colocados como abas de um único projeto no Arduino IDE.**

## 🔌 Montagem do circuito

### Placa A — LDR

| Componente | Ligação |
|---|---|
| Primeiro terminal do LDR | 3V3 |
| Segundo terminal do LDR | GPIO34 |
| Primeiro terminal do resistor de 10 kΩ | GPIO34, no mesmo ponto do LDR |
| Segundo terminal do resistor de 10 kΩ | GND |

O LDR e o resistor formam um divisor de tensão. Nessa montagem, **mais luz tende a produzir uma leitura ADC maior**.

A leitura utiliza resolução de 12 bits, com valores entre **0 e 4095**. Esses valores são leituras brutas do ADC, não medidas em lux.

### Placa B — LED amarelo opcional

Conecte o GPIO18 a um resistor de **220 Ω a 330 Ω**, seguido pelo ânodo do LED. Ligue o cátodo ao GND.

O LED alterna de estado quando uma solicitação de encaminhamento é aceita pela API.

### Placa C — LED verde opcional

Conecte o GPIO19 a um resistor de **220 Ω a 330 Ω**, seguido pelo ânodo do LED. Ligue o cátodo ao GND.

O LED alterna de estado quando C recebe uma mensagem válida de dados.

> Os LEDs são opcionais e não interferem no funcionamento da comunicação.

## 📦 Estrutura das mensagens

As três placas utilizam a mesma estrutura:

```cpp
struct Mensagem {
  char tipo;
  uint16_t adc;
  char texto[24];
};
```

| Campo | Descrição |
|---|---|
| `tipo` | Identifica dados (`D`) ou resposta (`R`) |
| `adc` | Armazena a leitura do LDR |
| `texto` | Armazena uma frase curta |

O campo `texto` comporta até **23 bytes de conteúdo**, reservando um byte para o terminador `'\0'`. Caracteres acentuados podem ocupar mais de um byte.

## ⚙️ Configuração dos endereços MAC

Cada ESP32 possui um endereço MAC. Os códigos exibem o **MAC STA** da própria placa durante a inicialização.

### Como configurar

1. Grave o código correspondente em cada placa.
2. Abra o Monitor Serial em **115200 baud**.
3. Pressione **EN/RESET** para visualizar a mensagem de inicialização.
4. Anote o MAC STA de cada ESP32.
5. Substitua os endereços de exemplo conforme a tabela.
6. Grave novamente os códigos nas três placas.

| Código | Endereços que precisam ser preenchidos |
|---|---|
| Placa A | `MAC_B` |
| Placa B | `MAC_A` e `MAC_C` |
| Placa C | `MAC_B` |

Por exemplo, o endereço:

```text
AA:BB:CC:11:22:33
```

É representado no código como:

```cpp
uint8_t MAC_B[] = {0xAA, 0xBB, 0xCC, 0x11, 0x22, 0x33};
```

**Os endereços presentes inicialmente nos códigos são exemplos e precisam ser substituídos.**

## 🔄 Funcionamento

### 1. Envio pela placa A

O usuário digita uma frase no Monitor Serial e pressiona Enter.

A placa A lê o GPIO34, cria uma mensagem do tipo `D` e solicita o envio para B.

### 2. Encaminhamento pela placa B

B verifica o tipo da mensagem e o endereço do remetente:

- Dados `D` recebidos de A são encaminhados para C.
- Respostas `R` recebidas de C são encaminhadas para A.

### 3. Recebimento pela placa C

C recebe os dados enviados por B, mostra o texto e a leitura ADC no Monitor Serial e cria uma resposta do tipo `R`.

A resposta preserva a leitura ADC recebida e contém o texto `Recebido em C`.

### 4. Confirmação na placa A

B encaminha a resposta de C para A, que exibe:

```text
A: ACK de C via B | ADC=2700 | Recebido em C
```

Essa mensagem demonstra que os dados chegaram a C e que a resposta percorreu o caminho de volta.

## 🧪 Roteiro de testes

### Teste 1 — Caminho completo

1. Ligue as três placas com os MACs configurados.
2. No Monitor Serial de A, selecione **Nova linha**.
3. Digite `teste rota` e envie.
4. Observe o encaminhamento em B, o recebimento em C e o retorno da confirmação em A.

Exemplo de saída, reunindo os três monitores:

```text
A -> B | texto=teste rota | ADC=2700
Envio solicitado. Aguardando ACK de C...

B: A -> C | proximo=C

C: recebeu teste rota | ADC=2700
C -> B | resposta solicitada

B: C -> A | proximo=A

A: ACK de C via B | ADC=2700 | Recebido em C
```

O valor ADC varia conforme a iluminação e o circuito.

### Teste 2 — Variação de luminosidade

1. Cubra o LDR e envie uma mensagem.
2. Anote o ADC exibido em C.
3. Ilumine o LDR e envie outra mensagem.
4. Compare as leituras.

### Teste 3 — Falha do intermediário

1. Desligue a placa B.
2. Envie uma mensagem pela placa A.
3. Observe que a confirmação de C não chega.
4. Ligue B novamente e aguarde sua inicialização.
5. Envie uma nova mensagem para verificar o retorno da comunicação.

> Envie uma mensagem por vez e aguarde a confirmação antes do próximo envio.

## ⚠️ Limitações e observações

- O retorno `ESP_OK` de `esp_now_send()` indica que a solicitação de envio foi aceita; ele não comprova o recebimento em C.
- A confirmação do percurso completo é o **ACK de C recebido em A**.
- Não há retransmissão automática, temporizador de espera pelo ACK ou bloqueio de novos envios enquanto uma resposta está pendente.
- O projeto não implementa TTL, filas FreeRTOS, controle de duplicatas ou descoberta de rotas.
- A comunicação está configurada sem criptografia.
- A placa A deve possuir o GPIO34, conforme a montagem proposta.
- Em simuladores como o Velxio, a comunicação ESP-NOW entre placas pode não estar disponível. Compilar o código não comprova a comunicação por rádio.

## 📸 Evidências da atividade

Registre:

- O envio da mensagem em A.
- O encaminhamento em B.
- O recebimento do texto e do ADC em C.
- O retorno do ACK em A.
- Leituras do LDR em duas condições de iluminação.
- A ausência de confirmação quando B está desligada.

## 📚 Conceitos

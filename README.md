# Libras ESP32-CAM

**Reconhecimento de letras de Libras em tempo real, direto na borda.**

Projeto que utiliza uma câmera ESP32 para identificar as letras **A**, **B** e **C** da Língua Brasileira de Sinais (Libras). A inferência acontece localmente, com um modelo de rede neural convolucional quantizado e executado via **TensorFlow Lite Micro**, e o resultado é disponibilizado por uma interface web embarcada.

![PlatformIO](https://img.shields.io/badge/PlatformIO-ESP32-orange?logo=platformio&logoColor=white)
![ESP-IDF](https://img.shields.io/badge/ESP--IDF-5.5-red)
![ESP32-S3](https://img.shields.io/badge/ESP32--S3-16%20MB%20Flash-blue)
![TensorFlow Lite Micro](https://img.shields.io/badge/TensorFlow%20Lite-Micro-FF6F00?logo=tensorflow&logoColor=white)
![Status](https://img.shields.io/badge/status-funcional-brightgreen)

[![Assista à demonstração](https://img.youtube.com/vi/DaBcUJlk8tk/maxresdefault.jpg)](https://www.youtube.com/shorts/DaBcUJlk8tk)

> 🎥 **Demo:** [Detector de Libras com ESP32-CAM](https://www.youtube.com/shorts/DaBcUJlk8tk)

---

## 📖 Sobre o projeto

O **Libras ESP32-CAM** é um sistema embarcado de visão computacional desenvolvido para reconhecer sinais estáticos do alfabeto de Libras. Uma câmera acoplada ao ESP32 captura imagens em resolução reduzida, que são processadas por uma CNN leve e classificadas em uma das três letras suportadas.

Todo o processamento ocorre **no próprio microcontrolador**, sem depender de internet ou de servidores externos. Além da inferência, o dispositivo cria uma rede Wi-Fi e disponibiliza uma interface web para acompanhar a captura e o resultado da classificação.

O projeto foi construído com foco em:

- **Baixo custo** — hardware acessível baseado em ESP32 com câmera.
- **Baixo consumo de memória** — modelo quantizado em INT8 com cerca de 242 KB.
- **Execução local** — privacidade e funcionamento offline.
- **Prototipagem rápida** — treinamento em Python/Keras e implantação com ESP-IDF.

---

## ✨ Funcionalidades

- 📷 Captura de imagens em **240×240** com câmera OV2640.
- 🧠 Inferência local com **TensorFlow Lite Micro**.
- 🔢 Classificação das letras **A**, **B** e **C** de Libras.
- 📉 Modelo **quantizado INT8** para reduzir uso de memória e acelerar a execução.
- 📶 Wi-Fi em modo **Access Point (AP)** ou **Station (STA)**.
- 🌐 Servidor web embarcado para visualização dos resultados.
- 🧵 Arquitetura baseada em **FreeRTOS**, com tasks dedicadas para câmera, inferência e servidor web.
- 💾 Uso de **PSRAM** para armazenamento dos frames da câmera.
- 🔁 Pipeline completo de treinamento, conversão e exportação do modelo em `modelo.ipynb`.

---

## 🏗️ Arquitetura

O firmware é organizado em componentes independentes, coordenados por um buffer compartilhado:

```text
        ┌──────────────┐
        │   Câmera     │
        │   OV2640     │
        └──────┬───────┘
               │ frame JPEG
               ▼
        ┌──────────────┐
        │  SharedData  │
        │ frame +      │
        │ resultado    │
        └──────┬───────┘
               │
       ┌───────┴────────┐
       ▼                ▼
┌──────────────┐  ┌──────────────┐
│ Inferencer   │  │  WebServer   │
│ Task         │  │  Task        │
│ TFLite Micro │  │  HTTP + Wi-Fi│
└──────────────┘  └──────────────┘
```

| Módulo | Responsabilidade |
| --- | --- |
| `Shared` | Buffer compartilhado entre as tasks, com o frame atual e o resultado da inferência. |
| `WiFiModule` | Inicialização e gerenciamento da rede nos modos AP e STA. |
| `InferenceModule` | Task de inferência: pré-processa o frame e executa o modelo TFLite. |
| `WebServerModule` | Servidor HTTP responsável pela interface web e pelo stream da câmera. |
| `NNModel` | Camada de abstração para carregar e executar o modelo neural. |
| `my_model` | Vetor do modelo TFLite já convertido e quantizado, embarcado no firmware. |

O `app_main` inicializa o buffer compartilhado, configura a rede, inicializa a câmera e dispara as tasks de inferência e do servidor web.

---

## 🔌 Hardware

O projeto suporta dois ambientes de compilação definidos em `platformio.ini`:

| Item | `esp32dev` | `esp32_s3` |
| --- | --- | --- |
| Placa | Freenove ESP32-WROVER | Freenove ESP32-S3 WROOM |
| Framework | ESP-IDF | ESP-IDF |
| Flash | 4 MB | 16 MB |
| PSRAM | Habilitada | Habilitada (OPI) |
| Partições | `partitions_esp32.csv` | `partitions_esp32_s3.csv` |
| Câmera | AI-Thinker / WROVER | ESP32-S3-EYE |

**Câmera:** OV2640, configurada para 240×240, formato JPEG, qualidade 12 e frame buffer na PSRAM.

A pinagem da câmera fica em [`src/camera_pins.h`](src/camera_pins.h), com suporte a diversas placas. O modelo utilizado é selecionado em [`src/main.cpp`](src/main.cpp):

```cpp
//#define CAMERA_MODEL_AI_THINKER
#define CAMERA_MODEL_ESP32S3_EYE
#include "camera_pins.h"
```

---

## 🤖 Modelo de IA

### Dataset

O conjunto de dados contém imagens de mãos representando as letras **A**, **B** e **C**, capturadas em 240×240 pixels:

| Conjunto | Imagens | Por classe |
| --- | ---: | ---: |
| Treino | 1.470 | 490 | | Validação | 315 | 105 | | Teste | 315 | 105 |
| **Total** | **2.100** | — |

### Arquitetura da CNN

O modelo é uma rede convolucional leve, treinada com Keras/TensorFlow:

| Camada | Configuração | Ativação |
| --- | --- | --- |
| `Rescaling` | Normalização dos pixels para `[0, 1]` | — |
| Data augmentation | Flip horizontal, rotação, zoom e contraste | — |
| `Conv2D` | 12 filtros, kernel 3×3 | ReLU |
| `MaxPooling2D` | Janela 2×2 | — |
| `Conv2D` | 14 filtros, kernel 3×3 | ReLU |
| `MaxPooling2D` | Janela 2×2 | — |
| `Conv2D` | 16 filtros, kernel 3×3 | ReLU |
| `MaxPooling2D` | Janela 2×2 | — |
| `Flatten` | Vetorização das features | — |
| `Dense` | 12 neurônios | ReLU |
| `Dense` | 3 neurônios (A, B, C) | Softmax |

**Treinamento:**

- Otimizador: `Adam` com learning rate `1e-4`
- Função de perda: `SparseCategoricalCrossentropy`
- Batch size: 32
- Máximo de 300 épocas
- `EarlyStopping` com paciência de 5 épocas e restauração dos melhores pesos

### Quantização e implantação

Após o treinamento, o modelo é convertido para TensorFlow Lite e quantizado em **INT8**, utilizando um conjunto representativo extraído do próprio dataset. O resultado é exportado como um vetor C (`model.cc`) e embarcado no firmware:

| Característica | Valor |
| --- | --- |
| Entrada | 240 × 240 × 3 |
| Saída | 3 classes (A, B, C) |
| Formato | TensorFlow Lite |
| Quantização | INT8 |
| Tamanho embarcado | ≈ 242 KB (247.944 bytes) |
| Acurácia no teste | **91,75%** |

---

## 🗂️ Estrutura do repositório

```text
libras-esp32/
├── src/
│   ├── main.cpp              # Inicialização do sistema, câmera, Wi-Fi e tasks
│   └── camera_pins.h         # Pinagem das placas de câmera suportadas
├── components/
│   ├── Shared/               # Buffer e dados compartilhados
│   ├── WiFiModule/           # Gerenciamento da rede Wi-Fi
│   ├── InferenceModule/      # Task de inferência
│   ├── WebServerModule/      # Servidor web embarcado
│   ├── NNModel/              # Abstração do modelo neural
│   ├── my_model/             # Modelo TFLite quantizado (model.cc)
│   ├── esp32-camera/         # Driver da câmera
│   ├── esp-tflite-micro/     # TensorFlow Lite Micro
│   └── esp-nn/               # Kernels otimizados para ESP32
├── dataset/
│   ├── train/                # Imagens de treino (A, B, C)
│   ├── val/                  # Imagens de validação (A, B, C)
│   └── test/                 # Imagens de teste (A, B, C)
├── modelo.ipynb              # Treinamento, avaliação, conversão e exportação
├── platformio.ini            # Ambientes de build
├── partitions_esp32.csv      # Tabela de partições do ESP32
├── partitions_esp32_s3.csv   # Tabela de partições do ESP32-S3
├── sdkconfig.esp32dev        # Configuração do ESP-IDF para ESP32
└── sdkconfig.esp32_s3        # Configuração do ESP-IDF para ESP32-S3
```

---

## 🚀 Como executar

### Pré-requisitos

- [PlatformIO](https://platformio.org/) (extensão do VS Code ou CLI)
- Toolchain do **ESP-IDF 5.5** (instalada automaticamente pelo PlatformIO)
- Placa ESP32 com câmera e PSRAM
- Cabo USB para gravação e monitoramento serial

### 1. Clonar o repositório

```bash
git clone git@github.com:gabrieloak123/libras-esp32.git
cd libras-esp32
```

### 2. Escolher o ambiente

Para a placa ESP32-S3:

```bash
pio run -e esp32_s3
```

Para a placa ESP32-WROVER:

```bash
pio run -e esp32dev
```

### 3. Gravar o firmware

```bash
pio run -e esp32_s3 -t upload
```

### 4. Monitorar a saída serial

```bash
pio device monitor -b 115200
```

### 5. Conectar e acessar a interface

Por padrão, o dispositivo sobe em **modo Access Point**:

1. Conecte-se à rede Wi-Fi `meu-esp32` com a senha `12345678`.
2. Abra o navegador em `http://192.168.4.1`.
3. A interface web exibirá a captura da câmera e a letra reconhecida.

Para usar o dispositivo em uma rede existente, altere `STA_MODE` para `true` em `src/main.cpp` e informe o SSID e a senha da sua rede:

```cpp
#define STA_MODE true

const char* ssid = "SUA_REDE";
const char* password = "SUA_SENHA";
```

---

## ⚙️ Configurações rápidas

| Configuração | Onde alterar | Valor padrão |
| --- | --- | --- |
| Modo Wi-Fi | `src/main.cpp` → `STA_MODE` | `false` (AP) |
| SSID do Access Point | `src/main.cpp` → `ap_ssid` | `meu-esp32` |
| Senha do Access Point | `src/main.cpp` → `ap_password` | `12345678` |
| Máximo de clientes no AP | `src/main.cpp` → `MAXCLIENTSESP32` | `4` |
| SSID da rede STA | `src/main.cpp` → `ssid` | `NPITI-IoT` |
| Senha da rede STA | `src/main.cpp` → `password` | `NPITI-IoT` |
| Modelo da câmera | `src/main.cpp` → `CAMERA_MODEL_*` | `ESP32S3_EYE` |
| Resolução da câmera | `src/main.cpp` → `frame_size` | `FRAMESIZE_240X240` |
| Qualidade JPEG | `src/main.cpp` → `jpeg_quality` | `12` |
| Frequência XCLK | `src/main.cpp` → `xclk_freq_hz` | `20 MHz` |

---

## 🧪 Notebook de treinamento

O arquivo [`modelo.ipynb`](modelo.ipynb) contém o pipeline completo de ciência de dados:

1. Montagem do Google Drive e carregamento do dataset.
2. Criação dos conjuntos de treino, validação e teste com `image_dataset_from_directory`.
3. Definição da CNN com data augmentation.
4. Treinamento com `EarlyStopping`.
5. Avaliação no conjunto de teste.
6. Exportação do modelo Keras (`modelo.keras`) e do artefato `modelofinal`.
7. Conversão para TensorFlow Lite:
   - **Float32** — sem quantização.
   - **INT8** — com dataset representativo e operadores `TFLITE_BUILTINS_INT8`.
8. Geração automática de `model.h` e `model.cc` com `xxd -i`, prontos para embarcar no firmware.

> O notebook foi desenvolvido para o Google Colab e espera o dataset em `/content/drive/MyDrive/Libras/dataset`.

Para atualizar o modelo no firmware, substitua [`components/my_model/model.cc`](components/my_model/model.cc) pelo arquivo gerado ao final do notebook e recompile o projeto.

---

## 📊 Resultados

| Métrica | Valor |
| --- | ---: |
| Acurácia no conjunto de teste | **91,75%** |
| Classes suportadas | 3 (A, B, C) |
| Resolução de entrada | 240 × 240 × 3 |
| Tamanho do modelo | ≈ 242 KB |
| Quantização | INT8 |

O modelo foi projetado para caber confortavelmente na memória do ESP32, permitindo inferência local com baixa latência e sem dependência de conexão com a internet.

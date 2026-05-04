# LaraRadio

[🇺🇸 English](README.md) | [🇪🇸 Español](README.es.md)

O LaraRadio é um software de automação de rádio leve e eficiente para Linux.

---

## Visão Geral

Projetado com foco em engenharia prática:

- Baixo consumo de CPU e memória
- Execução nativa em Linux
- Reprodução de áudio previsível
- Arquitetura modular

---

## Tecnologias

### C++

- Alta performance
- Controle de memória
- Baixa latência

### Qt 6

- Interface nativa
- Arquitetura orientada a eventos
- Separação clara entre UI e lógica

### FFmpeg

- Suporte a múltiplos formatos
- Estável e consolidado
- Compatível com streaming

---

## Build

### Requisitos

- GCC 11+ ou Clang
- CMake 3.16+
- Qt 6.8+ (Quick, Widgets, Multimedia, MultimediaWidgets)
- TagLib
- Suporte FFmpeg em tempo de execução para o Qt Multimedia

### Ubuntu

```bash
sudo apt update
sudo apt install build-essential cmake ninja-build qt6-base-dev qt6-declarative-dev qt6-multimedia-dev libtag1-dev ffmpeg
```

Se a sua versão do Ubuntu não fornecer Qt 6.8 ou superior, instale o Qt pelo instalador oficial e garanta que o CMake encontre essa instalação.

### Compilação

```bash
git clone https://github.com/gutierre69/lararadio.git
cd lararadio

mkdir build
cd build

cmake ..
cmake --build . -j$(nproc)
```

### Execução

```bash
./appLaraRadio
```

---

## Distribuição

- AppImage
- Flatpak (futuro)

---

## Contribuição

- Correção de bugs
- Melhorias de performance
- Interface
- Funcionalidades

---

## Site

https://lararadio.com

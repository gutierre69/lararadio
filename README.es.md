# LaraRadio

[🇺🇸 English](README.md) | [🇧🇷 Português](README.pt-BR.md)

LaraRadio es un software de automatización de radio ligero para Linux.

---

## Descripción

Diseñado con enfoque técnico:

- Bajo consumo de recursos
- Ejecución nativa
- Reproducción estable
- Arquitectura modular

---

## Tecnologías

### C++

- Alto rendimiento
- Control de memoria
- Baja latencia

### Qt 6

- Interfaz nativa
- Arquitectura por eventos
- Separación UI/lógica

### FFmpeg

- Soporte de formatos múltiples
- Estable
- Streaming

---

## Compilación

### Requisitos

- GCC 11+ o Clang
- CMake 3.16+
- Qt 6.8+ (Quick, Widgets, Multimedia, MultimediaWidgets)
- TagLib
- Soporte FFmpeg en tiempo de ejecución para Qt Multimedia

### Ubuntu

```bash
sudo apt update
sudo apt install build-essential cmake ninja-build qt6-base-dev qt6-declarative-dev qt6-multimedia-dev libtag1-dev ffmpeg
```

Si tu versión de Ubuntu no ofrece Qt 6.8 o superior, instala Qt con el instalador oficial y asegúrate de que CMake encuentre esa instalación.

### Build

```bash
git clone https://github.com/gutierre69/lararadio.git
cd lararadio

mkdir build
cd build

cmake ..
cmake --build . -j$(nproc)
```

### Ejecución

```bash
./appLaraRadio
```

---

## Distribución

- AppImage
- Flatpak (planificado)

---

## Contribución

- Corrección de errores
- Rendimiento
- UI
- Funciones

---

## Sitio

https://lararadio.com

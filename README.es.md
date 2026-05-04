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

### C++ (C++20)

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
- CMake 3.20+
- Qt 6
- FFmpeg

### Ubuntu

```bash
sudo apt update
sudo apt install build-essential cmake qt6-base-dev qt6-multimedia-dev ffmpeg
```

### Build

```bash
git clone https://github.com/gutierre69/lararadio.git
cd lararadio

mkdir build
cd build

cmake ..
make -j$(nproc)
```

### Ejecución

```bash
./lararadio
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

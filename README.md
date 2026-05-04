# LaraRadio

[🇧🇷 Português](README.pt-BR.md) | [🇪🇸 Español](README.es.md)

LaraRadio is a lightweight and efficient radio automation software designed for Linux systems.
It focuses on simplicity, performance, and reliability for small to medium radio stations.

---

## Overview

LaraRadio was designed with a pragmatic engineering mindset:

- Minimal CPU and RAM footprint
- Native Linux execution (no heavy runtimes)
- Deterministic audio playback
- Modular and extensible architecture

---

## Tech Stack

### C++
The core is implemented in modern C++, providing:

- High performance and low latency
- Fine-grained control over memory and threads
- Predictable real-time behavior

### Qt 6
Qt is used for the GUI layer:

- Event-driven architecture
- Clean separation between UI and logic
- Cross-platform potential (future)

### FFmpeg
Handles decoding and playback:

- Wide format support (MP3, AAC, OGG, etc.)
- Mature and battle-tested
- Streaming-ready

---

## Build Instructions

### Requirements

- GCC 11+ or Clang
- CMake 3.16+
- Qt 6.8+ (Quick, Widgets, Multimedia, MultimediaWidgets)
- TagLib
- FFmpeg runtime support for Qt Multimedia

### Ubuntu example

```bash
sudo apt update
sudo apt install build-essential cmake ninja-build qt6-base-dev qt6-declarative-dev qt6-multimedia-dev libtag1-dev ffmpeg
```

If your Ubuntu release does not provide Qt 6.8 or newer, install Qt from the official Qt installer and make sure CMake can find that Qt installation.

### Build

```bash
git clone https://github.com/gutierre69/lararadio.git
cd lararadio

mkdir build
cd build

cmake ..
cmake --build . -j$(nproc)
```

### Run

```bash
./appLaraRadio
```

---

## Distribution

- AppImage (primary)
- Flatpak (planned)

---

## Contributing

We welcome contributions:

- Bug fixes
- Performance improvements
- UI/UX enhancements
- New features

---

## Website

https://lararadio.com

#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")"
if ! pkg-config --exists Qt6Widgets libpulse rubberband; then
  echo 'Dependências ausentes. No CachyOS: sudo pacman -S --needed gcc pkgconf qt6-base libpulse rubberband'
  exit 1
fi
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -fPIC src/main.cpp -o gepetas $(pkg-config --cflags --libs Qt6Widgets libpulse rubberband) -pthread
echo 'Pronto: execute ./gepetas ou dê dois cliques em Iniciar-JOCAS.sh.'

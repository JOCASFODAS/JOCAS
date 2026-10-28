#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")"
# The source archive contains engine-source with all pinned submodule files.
pkg-config --exists Qt6Widgets wayland-cursor wayland-protocols egl wayland-egl || {
    echo 'Instale as dependências de desenvolvimento, incluindo wayland-protocols, antes de compilar.'
    exit 1
}
cmake -S engine-source -B engine-source/build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build engine-source/build -j 4
cmake --install engine-source/build --prefix "$PWD/payload/engine"
g++ -std=c++20 -O2 -Wall -Wextra -Werror -fPIC src/main.cpp -o payload/jocas-wallpapers $(pkg-config --cflags --libs Qt6Widgets)
cp integration.py payload/
cp -a engine-source/kde payload/
cp engine-source/LICENSE payload/LICENSE
cp engine-source/build/cef/cef_binary_*/LICENSE.txt payload/LICENSE-CEF.txt
strip --strip-unneeded payload/engine/libcef.so
echo 'Pronto. Execute bash Instalar.sh para instalar no seu usuário.'

#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")"
python3 install.py --activate
echo 'Instalação concluída. Para a primeira ativação, saia e entre na sessão KDE, ou reinicie apenas o Plasma.'
echo 'Depois abra JOCAS Wallpapers pelo menu de aplicativos.'

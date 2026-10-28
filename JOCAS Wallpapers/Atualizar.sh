#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")"
python3 install.py
echo 'Atualizado. Suas preferências foram preservadas e a versão anterior foi guardada em backup.'

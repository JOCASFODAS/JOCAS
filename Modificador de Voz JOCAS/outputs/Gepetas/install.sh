#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")"
[[ -x ./gepetas ]] || ./build.sh
app_dir="${XDG_DATA_HOME:-$HOME/.local/share}/gepetas"
desktop_dir="${XDG_DATA_HOME:-$HOME/.local/share}/applications"
mkdir -p -- "$app_dir" "$desktop_dir"
install -m 755 gepetas "$app_dir/gepetas"
install -m 644 gepetas.svg "$app_dir/gepetas.svg"
# Models live in the user configuration folder and are never overwritten.
cp -R ai "$app_dir/"
chmod +x "$app_dir/ai/tools/uv"
# Escape Desktop Entry reserved characters without evaluating path contents.
python3 - "$app_dir" "$desktop_dir/gepetas.desktop" <<'PY'
import pathlib, sys
directory, target = sys.argv[1:]
executable = str(pathlib.Path(directory) / 'gepetas')
escaped = executable.replace('\\', '\\\\\\\\').replace('"', '\\\\"').replace('`', '\\\\`').replace('$', '\\\\$').replace('%', '%%')
text = f'''[Desktop Entry]
Type=Application
Name=Modificador de Voz JOCAS
Comment=Mude sua voz no Discord e escute o microfone quando quiser
Exec="{escaped}"
Icon={directory}/gepetas.svg
Terminal=false
Categories=AudioVideo;Audio;
StartupNotify=true
'''
pathlib.Path(target).write_text(text)
PY
echo 'Instalado! Procure por Modificador de Voz JOCAS no menu de aplicativos.'

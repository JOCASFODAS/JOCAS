#!/usr/bin/env python3
"""Install or update the staged app. Does not change wallpapers unless --activate is given."""
import argparse
from datetime import datetime, timezone
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parent

def atomic_copy(source,target):
    target=Path(target)
    fd,name=tempfile.mkstemp(prefix='.jocas-install-',dir=target.parent)
    os.close(fd)
    try:
        shutil.copy2(source,name)
        os.replace(name,target)
    finally:
        if os.path.exists(name):os.unlink(name)
    return str(target)

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--activate', action='store_true', help='Activate KDE integration after installing')
    args=parser.parse_args()
    source=ROOT/'payload'
    if not (source/'jocas-wallpapers').is_file() or not (source/'engine/linux-wallpaperengine').is_file():
        raise SystemExit('Pacote incompleto. Extraia todo o arquivo antes de instalar.')
    data=Path(os.environ.get('XDG_DATA_HOME',str(Path.home()/'.local/share')))
    state=Path(os.environ.get('XDG_STATE_HOME',str(Path.home()/'.local/state')))/'jocas-wallpapers'
    target=data/'jocas-wallpapers'
    applications=data/'applications'
    state.mkdir(parents=True,exist_ok=True)
    # Unique backup per install, preserving original desktop configuration and previous app.
    backup=state/'backups'/datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ')
    backup.mkdir(parents=True)
    plasma=Path.home()/'.config/plasma-org.kde.plasma.desktop-appletsrc'
    if plasma.is_file():shutil.copy2(plasma,backup/plasma.name)
    preferences=Path.home()/'.config/JOCAS/Wallpapers.conf'
    if preferences.is_file():shutil.copy2(preferences,backup/preferences.name)
    if target.exists():
        # Refuse to overwrite a running copy; check via /proc rather than killing user processes.
        for proc in Path('/proc').iterdir():
            if not proc.name.isdigit():continue
            try:
                exe=(proc/'exe').resolve()
                if exe.is_relative_to(target):raise SystemExit('Feche o JOCAS Wallpapers pelo menu da bandeja antes de atualizar.')
            except (PermissionError,FileNotFoundError,OSError):pass
        shutil.copytree(target,backup/'app-anterior',ignore=shutil.ignore_patterns('.git','.agents','.aws','.codex'))
    try:
        shutil.copytree(source,target,dirs_exist_ok=True,copy_function=atomic_copy)
    except Exception:
        if (backup/'app-anterior').exists():shutil.copytree(backup/'app-anterior',target,dirs_exist_ok=True,copy_function=atomic_copy)
        raise
    applications.mkdir(parents=True,exist_ok=True)
    exe=str(target/'jocas-wallpapers').replace('\\','\\\\').replace('"','\\"').replace('`','\\`').replace('$','\\$').replace('%','%%')
    (applications/'jocas-wallpapers.desktop').write_text('[Desktop Entry]\nType=Application\nName=JOCAS Wallpapers\nComment=Wallpapers da Steam no CachyOS\nExec="'+exe+'"\nIcon=preferences-desktop-wallpaper\nTerminal=false\nCategories=Utility;Graphics;\n',encoding='utf-8')
    services=Path.home()/'.config/systemd/user'
    services.mkdir(parents=True,exist_ok=True)
    # Optional launch unit. Login restoration is controlled by the app's checkbox.
    (services/'jocas-wallpapers.service').write_text('[Unit]\nDescription=JOCAS Wallpapers\nAfter=plasma-plasmashell.service\n\n[Service]\nType=simple\nExecStart="'+str(target/'jocas-wallpapers').replace('%','%%').replace('"','\\"')+'" --restore\nRestart=no\n',encoding='utf-8')
    if args.activate:
        subprocess.run(['python3',str(target/'integration.py'),'--activate'],check=True)
    print('Instalado em '+str(target))
    print('Backup em '+str(backup))

if __name__=='__main__':main()

#!/usr/bin/env python3
"""Integration setup/restoration. Uses the upstream KDE package, never edits live Plasma files."""
import argparse
import json
from pathlib import Path
import shutil
import subprocess

PLUGIN = 'io.github.linuxwallpaperengine.desktop'
ROOT = Path(__file__).resolve().parent

def plasma(script):
    return subprocess.check_output(['qdbus6', 'org.kde.plasmashell', '/PlasmaShell',
        'org.kde.PlasmaShell.evaluateScript', script], text=True).strip()

def desktops():
    return json.loads(plasma('print(JSON.stringify(desktops().map(d=>({id:d.id,screen:d.screen,plugin:d.wallpaperPlugin}))))'))

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--activate', action='store_true')
    parser.add_argument('--restore', action='store_true')
    parser.add_argument('--list', action='store_true')
    args=parser.parse_args()
    current=desktops()
    if args.list:
        print(json.dumps(current, indent=2)); return
    if args.restore:
        for desktop in current:
            if desktop['screen']>=0 and desktop['plugin']==PLUGIN:
                script=f'var d=desktopById({int(desktop["id"])});d.currentConfigGroup=["Wallpaper",{json.dumps(PLUGIN)},"General"];d.wallpaperPlugin=d.readConfig("PreviousWallpaper","org.kde.image");'
                plasma(script)
        print('Fundo anterior restaurado.'); return
    if args.activate:
        for desktop in current:
            if desktop['screen']>=0 and desktop['plugin']!=PLUGIN:
                subprocess.run(['python3', str(ROOT/'kde/install.py'), '--desktop', str(int(desktop['id'])),
                    '--no-autostart', '--mouse-interaction', 'off'], check=True, stdout=subprocess.DEVNULL)
        print('Integração do KDE ativada.')

if __name__=='__main__': main()

#!/usr/bin/env python3
"""Test default selection with temporary silent devices; restore user's default."""
import json,time,shlex,subprocess as sp,tempfile,uuid
from pathlib import Path
root=Path(__file__).resolve().parents[1]
def pactl(*args):return sp.check_output(['pactl',*args],text=True).strip()
original=pactl('get-default-source'); modules=[]; names=[]
try:
    with tempfile.TemporaryDirectory() as td:
        binary=str(Path(td)/'game-mode')
        flags=shlex.split(sp.check_output(['pkg-config','--cflags','--libs','Qt6Core'],text=True))
        sp.run(['g++','-std=c++17','-fPIC',str(root/'tests/game_mode_test.cpp'),'-o',binary,*flags],check=True)
        prefix='jocas_mode_'+uuid.uuid4().hex[:8]
        for suffix in ('a','b','c'):
            name=prefix+'_'+suffix; names.append(name)
            modules.insert(0,pactl('load-module','module-null-sink','sink_name='+name,'sink_properties=device.description=JOCAS_Test priority.session=1'))
        sp.run([binary,*[n+'.monitor' for n in names]],check=True)
finally:
    # Preserve any external user choice made while the test ran.
    if pactl('get-default-source') in [n+'.monitor' for n in names]:pactl('set-default-source',original)
    for module in modules:
        try:pactl('unload-module',module)
        except sp.SubprocessError:pass
for _ in range(40):
    if pactl('get-default-source')==original:break
    time.sleep(.01)
assert pactl('get-default-source')==original
print('PASS: original system input restored; no microphone audio captured')

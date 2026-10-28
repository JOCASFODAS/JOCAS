#!/usr/bin/env python3
"""Build the same JOCAS sources for Windows x64 using MinGW-w64 + Qt 6.

On MSYS2 MINGW64: python windows/build_windows.py --qt-prefix /mingw64 --compiler g++
On Linux: provide a MinGW-w64 cross compiler and an extracted MSYS2 MINGW64 prefix.
"""
import argparse
import hashlib
from pathlib import Path
import re
import shutil
import subprocess as sp
import tarfile

p = argparse.ArgumentParser()
p.add_argument('--qt-prefix', required=True, type=Path)
p.add_argument('--compiler', default='x86_64-w64-mingw32-g++')
p.add_argument('--out', type=Path)
p.add_argument('--build-dir', type=Path)
args = p.parse_args()
root = Path(__file__).resolve().parents[1]
qt = args.qt_prefix.resolve()
out = (args.out or root/'dist/JOCAS-Windows-x64').resolve()
build = (args.build_dir or root/'build-windows').resolve()
out.mkdir(parents=True, exist_ok=True); build.mkdir(parents=True, exist_ok=True)
compiler = shutil.which(args.compiler) or str(Path(args.compiler).resolve())
bindir = Path(compiler).parent
toolprefix = Path(compiler).name.removesuffix('.exe').removesuffix('g++')
if toolprefix == 'g++': toolprefix = ''
def tool(name):
    candidate = bindir/(toolprefix+name)
    return str(candidate) if candidate.exists() else (shutil.which(toolprefix+name) or name)
archive = root/'third_party/rubberband-4.0.0.tar.gz'
assert hashlib.file_digest(archive.open('rb'), 'sha256').hexdigest() == '24300f48a8014b7c863b573a9647e61b1b19b37875e2cdd92005e64c6424d266'
rubber = build/'rubberband-4.0.0'
if not rubber.exists():
    with tarfile.open(archive) as t: t.extractall(build, filter='data')
common = [compiler, '-std=c++17', '-O2', '-DNOMINMAX', '-D_WIN32_WINNT=0x0A00', '-I'+str(rubber)]
rbobj = build/'rubberband.o'
if not rbobj.exists() or rbobj.stat().st_mtime < archive.stat().st_mtime:
    sp.run([*common, '-c', str(rubber/'single/RubberBandSingle.cpp'), '-o', str(rbobj)], check=True)
rcobj = build/'jocas-resource.o'
sp.run([tool('windres'), '-I', str(root), '-I', str(root/'windows'), str(root/'windows/jocas.rc'), str(rcobj)], check=True)
includes = ['-I'+str(qt/'include/qt6'/sub) for sub in ('', 'QtCore', 'QtGui', 'QtWidgets')]
sp.run([*common, '-Wall', '-Wextra', '-DQT_NO_DEBUG', '-DQT_WIDGETS_LIB', '-DQT_GUI_LIB', '-DQT_CORE_LIB',
        *includes, str(root/'src/main.cpp'), str(rbobj), str(rcobj), '-L'+str(qt/'lib'),
        '-lQt6Widgets', '-lQt6Gui', '-lQt6Core', '-lole32', '-luuid', '-lwinmm', '-lksuser',
        '-mwindows', '-s', '-o', str(out/'JOCAS.exe')], check=True)
shutil.copy2(root/'jocas.ico', out/'jocas.ico')
shutil.copytree(root/'ai', out/'ai', dirs_exist_ok=True, ignore=shutil.ignore_patterns('__pycache__', 'uv'))
(out/'qt.conf').write_text('[Paths]\nPlugins=plugins\n', encoding='utf8')
plugins = qt/'share/qt6/plugins'
pending = [out/'JOCAS.exe']
for rel in ('platforms/qwindows.dll', 'styles/qmodernwindowsstyle.dll'):
    target = out/'plugins'/rel; target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(plugins/rel, target); pending.append(target)
available = {f.name.lower():f for f in (qt/'bin').glob('*.dll')}
system = set()
done = set()
while pending:
    exe = pending.pop()
    imports = re.findall(r'DLL Name:\s*(\S+)', sp.check_output([tool('objdump'), '-p', str(exe)], text=True))
    for dll in imports:
        key = dll.lower()
        if key in done: continue
        done.add(key)
        if key in available:
            target = out/available[key].name; shutil.copy2(available[key], target); pending.append(target)
        else: system.add(dll)
(out/'windows-system-dlls.txt').write_text('\n'.join(sorted(system))+'\n')
shutil.copy2(root/'LICENSE', out/'LICENSE')
(out/'licenses').mkdir(exist_ok=True)
licenses = qt/'share/licenses'
if licenses.exists(): shutil.copytree(licenses, out/'licenses/MSYS2', dirs_exist_ok=True)
shutil.copy2(root/'third_party/RUBBERBAND-LICENSE', out/'licenses/RUBBERBAND-LICENSE')
if (root/'windows/LEIA-ME-WINDOWS.md').exists(): shutil.copy2(root/'windows/LEIA-ME-WINDOWS.md', out/'LEIA-ME.md')
for name in ('DEPENDENCIAS.md', 'msys2-packages.json'):
    source = root/'windows'/name
    if source.exists(): shutil.copy2(source, out/name)
print('Windows package:', out)
print('System DLL imports:', ', '.join(sorted(system)))

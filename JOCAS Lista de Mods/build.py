#!/usr/bin/env python3
"""Reproducible Java 17 bytecode, with real loader APIs (compile-only).
Use Python 3.11+ and a JDK 17+. No Gradle, Minecraft downloads or remapping needed.
"""
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import urllib.request
import zipfile

ROOT = Path(__file__).resolve().parent
VERSION = "1.0.0"
DEPS = {
    "fabric": ("fabric-loader-0.16.14.jar", "https://maven.fabricmc.net/net/fabricmc/fabric-loader/0.16.14/fabric-loader-0.16.14.jar"),
    "forge": ("javafmllanguage-1.20.1-47.4.0.jar", "https://maven.minecraftforge.net/net/minecraftforge/javafmllanguage/1.20.1-47.4.0/javafmllanguage-1.20.1-47.4.0.jar"),
    "neoforge": ("loader-2.0.17.jar", "https://maven.neoforged.net/releases/net/neoforged/fancymodloader/loader/2.0.17/loader-2.0.17.jar"),
}

def java_tool(tool):
    home = os.environ.get("JAVA_HOME")
    if home:
        return str(Path(home) / "bin" / (tool + (".exe" if os.name == "nt" else "")))
    exe = shutil.which(tool)
    if not exe:
        raise SystemExit("Instale um JDK 17 ou superior e defina JAVA_HOME.")
    return exe

def compile_java(target, sources, classpath=None):
    target.mkdir(parents=True, exist_ok=True)
    cmd = [java_tool("javac"), "--release", "17", "-encoding", "UTF-8", "-d", str(target)]
    if classpath:
        cmd += ["-classpath", os.pathsep.join(map(str, classpath))]
    subprocess.run(cmd + list(map(str, sources)), check=True)

def jar(path, roots, extras=None):
    entries = {}
    for root in roots:
        for source in root.rglob("*"):
            if source.is_file(): entries[source.relative_to(root).as_posix()] = source.read_bytes()
    entries.update(extras or {})
    with zipfile.ZipFile(path, "w", zipfile.ZIP_DEFLATED) as archive:
        for name, data in sorted(entries.items()):
            info = zipfile.ZipInfo(name, (2026, 10, 22, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            archive.writestr(info, data)

def build():
    vendor = ROOT / "vendor"; vendor.mkdir(exist_ok=True)
    lock = dict(line.split() for line in (ROOT / "dependencies.sha256").read_text().splitlines())
    for name, url in DEPS.values():
        dest = vendor / name
        if not dest.exists():
            print("Baixando API:", name)
            with urllib.request.urlopen(url) as response: dest.write_bytes(response.read())
        if hashlib.sha256(dest.read_bytes()).hexdigest() != lock[name]:
            raise SystemExit("Checksum incorreto: " + name)
    build_dir = ROOT / "build"
    if build_dir.exists(): shutil.rmtree(build_dir)
    dist = ROOT / "dist"; dist.mkdir(exist_ok=True)
    common = build_dir / "common"
    compile_java(common, sorted((ROOT / "src/main/java").rglob("*.java")))
    viewer = build_dir / "viewer"
    compile_java(viewer, sorted((ROOT / "src/viewer/java").rglob("*.java")), [common])
    manifest = b"Manifest-Version: 1.0\r\nMain-Class: br.jocas.modlist.Viewer\r\n\r\n"
    jar(dist / "JOCAS-Visualizador.jar", [common, viewer], {"META-INF/MANIFEST.MF": manifest})
    for loader, (dep, _) in DEPS.items():
        target = build_dir / loader
        compile_java(target, sorted((ROOT / ("src/" + loader + "/java")).rglob("*.java")), [common, vendor / dep])
        extras = {"jocas-viewer.jar": (dist / "JOCAS-Visualizador.jar").read_bytes(),
                  "LICENSE": (ROOT / "LICENSE").read_bytes(),
                  "META-INF/MANIFEST.MF": b"Manifest-Version: 1.0\r\nFMLModType: MOD\r\nAutomatic-Module-Name: br.jocas.modlist\r\n\r\n"}
        if loader == "neoforge":
            # NeoForge before 1.20.5 uses mods.toml; newer versions use neoforge.mods.toml.
            extras["META-INF/mods.toml"] = (ROOT / "src/neoforge/resources/META-INF/neoforge.mods.toml").read_bytes()
        jar(dist / f"JOCAS-Lista-de-Mods-{VERSION}-{loader}.jar", [common, target, ROOT / "src" / loader / "resources"], extras)
    tests = build_dir / "tests"
    compile_java(tests, sorted((ROOT / "tests").glob("*.java")), [common, viewer, build_dir / "fabric", vendor / DEPS["fabric"][0]])
    cp = os.pathsep.join(map(str, [common, viewer, tests, build_dir / "fabric", vendor / DEPS["fabric"][0]]))
    subprocess.run([java_tool("java"), "-Djava.awt.headless=true", "-cp", cp, "br.jocas.modlist.Tests"], check=True)
    for name in ["LEIA-ME.md", "COMPATIBILIDADE.md", "LICENSE"]: shutil.copy2(ROOT / name, dist / name)
    sums = [hashlib.sha256(p.read_bytes()).hexdigest() + "  " + p.name for p in sorted(dist.glob("*.jar"))]
    (dist / "SHA256SUMS.txt").write_text("\n".join(sums) + "\n")
    print("Pronto:", dist)

if __name__ == "__main__": build()

"""Gera e publica as entregas atuais na pasta central do Codex."""
from pathlib import Path
import hashlib
import shutil
import tempfile
import zipfile
from catalogo import publish

ROOT=Path(__file__).resolve().parents[2]
DOWNLOADS=ROOT/'Downloads e Atualizações'
VOICE=ROOT/'Modificador de Voz JOCAS/outputs'
MOD=ROOT/'JOCAS Lista de Mods'
_staging=tempfile.TemporaryDirectory(prefix='jocas-entregas-')
OUT=Path(_staging.name)

def text(value):
    return value.encode('utf-8')

def folder(path):
    return {p.relative_to(path).as_posix():p.read_bytes() for p in path.rglob('*') if p.is_file()}

def write_zip(filename, entries):
    path = OUT / filename
    with zipfile.ZipFile(path, 'w', zipfile.ZIP_DEFLATED) as z:
        for name, data in sorted(entries.items()):
            info = zipfile.ZipInfo(name, (2026, 10, 22, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = ((0o100755 if name.endswith('.sh') else 0o100644) << 16)
            z.writestr(info, data)
    with zipfile.ZipFile(path) as z:
        assert z.testzip() is None
        assert len(z.namelist()) == len(entries)
        for name, data in entries.items():
            assert hashlib.sha256(z.read(name)).digest() == hashlib.sha256(data).digest(), name
    print(filename, path.stat().st_size, 'bytes — integridade conferida')

from empacotar_voz import publish_voice
publish_voice()

common_mod = {}
for loader in ('fabric', 'forge', 'neoforge'):
    jar = f'JOCAS-Lista-de-Mods-1.0.0-{loader}.jar'
    common_mod[loader.capitalize()+'/'+jar] = (MOD/'dist'/jar).read_bytes()
for name in ('LEIA-ME.md', 'COMPATIBILIDADE.md', 'LICENSE'):
    common_mod[name] = (MOD/'dist'/name).read_bytes()
mod_note = '''
Versão do mod: 1.0.0. Não há versão nova de código nesta entrega.
Escolha SOMENTE UM carregador. Para NeoForge no Minecraft 1.20.1, use Forge.
O pacote não instala Minecraft, Java nem o carregador de mods.
Linux e Windows utilizam os mesmos JARs. Consulte COMPATIBILIDADE.md:
as versões futuras e todas as versões intermediárias não estão garantidas.
'''
install_mod = dict(common_mod)
install_mod['Visualizador/JOCAS-Visualizador.jar'] = (MOD/'dist/JOCAS-Visualizador.jar').read_bytes()
install_mod['COMECE-AQUI.txt'] = text('''JOCAS LISTA DE MODS 1.0.0 — INSTALAÇÃO COMPLETA

1. Feche o Minecraft e extraia este ZIP.
2. Escolha a pasta Fabric, Forge ou Neoforge conforme seu modpack.
3. Copie somente o JAR dessa pasta para a pasta mods da instância.
   No Prism Launcher: editar instância > Mods > Adicionar.
4. Abra o Minecraft. Uma janela auxiliar JOCAS abre para listar e copiar os mods.

Não coloque o Visualizador na pasta mods. Ele é um complemento separado para
reabrir relatórios salvos. O JAR do mod já inclui esse visualizador.
''' + mod_note)
write_zip('JOCAS-Lista-de-Mods-1.0.0-Instalacao.zip', install_mod)
update_mod = dict(common_mod)
update_mod['COMECE-AQUI.txt'] = text('''JOCAS LISTA DE MODS 1.0.0 — ATUALIZAÇÃO / REAPLICAÇÃO

Este é o pacote para reaplicar a versão atual a uma instalação existente.
Como 1.0.0 é a primeira versão, ele não acrescenta recursos novos.

1. Feche o Minecraft e extraia este ZIP.
2. Crie uma pasta de backup FORA da pasta mods da instância.
3. Mova somente os arquivos JOCAS-Lista-de-Mods-*.jar já existentes em mods
   para esse backup. Não mova os outros mods do modpack.
4. Escolha a pasta Fabric, Forge ou Neoforge deste pacote, conforme o carregador.
5. Copie apenas o JAR escolhido para mods. Não deixe duas cópias deste mod ali.
6. Abra o Minecraft. O mod cria um relatório novo durante sua inicialização.

Preserve as pastas config, saves e jocas-modlist da instância.
Caso queira guardar o relatório anterior, copie jocas-modlist para o backup;
o relatório é regenerado a cada inicialização. Não é necessário substituir
manualmente o visualizador: ele será atualizado automaticamente pelo mod.
Para desfazer a troca, feche o jogo, retire o JAR novo e restaure o antigo do backup.
''' + mod_note)
write_zip('JOCAS-Lista-de-Mods-1.0.0-Atualizacao.zip', update_mod)

# Publish only after both installer/update packages and the matching sources are ready.
mod_sources=OUT/'JOCAS-Lista-de-Mods-1.0.0-Fontes.zip'
with zipfile.ZipFile(mod_sources,'w',zipfile.ZIP_DEFLATED) as z:
    for base in ('src','tests'):
        for p in sorted((MOD/base).rglob('*')):
            if p.is_file():z.write(p,p.relative_to(MOD).as_posix())
    for name in ['build.py','dependencies.sha256','LEIA-ME.md','COMPATIBILIDADE.md','LICENSE','.gitignore']:
        z.write(MOD/name,name)
for prefix,version in [('JOCAS-Lista-de-Mods','1.0.0')]:
    publish(DOWNLOADS,prefix,version,sorted(OUT.glob(prefix+'-'+version+'-*.zip')))
_staging.cleanup()
print('Pacotes publicados em',DOWNLOADS)

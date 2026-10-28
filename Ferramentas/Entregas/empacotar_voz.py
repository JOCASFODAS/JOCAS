"""Publica somente o JOCAS de voz, com fontes atuais e integridade validada."""
from pathlib import Path
import hashlib
import io
import json
import tempfile
import zipfile
from catalogo import publish

ROOT = Path(__file__).resolve().parents[2]
APP = ROOT / 'Modificador de Voz JOCAS/outputs'
DEST = ROOT / 'Downloads e Atualizações'

def zip_bytes(entries):
    stream = io.BytesIO()
    with zipfile.ZipFile(stream, 'w', zipfile.ZIP_DEFLATED) as z:
        for name, data in sorted(entries.items()):
            info = zipfile.ZipInfo(name, (2026, 10, 23, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            executable = name.endswith('.sh') or Path(name).name in ('gepetas', 'uv')
            info.external_attr = (0o100755 if executable else 0o100644) << 16
            z.writestr(info, data)
    raw = stream.getvalue()
    with zipfile.ZipFile(io.BytesIO(raw)) as z:
        assert z.testzip() is None
        assert len(z.namelist()) == len(entries)
        for name, data in entries.items():
            assert hashlib.sha256(z.read(name)).digest() == hashlib.sha256(data).digest()
    return raw

def source_entries(linux):
    result = {}
    for base in ('src', 'tests', 'windows', 'third_party', 'ai'):
        for file in sorted((linux/base).rglob('*')):
            if file.is_file() and '__pycache__' not in file.parts and file.name not in ('uv', 'uv.exe'):
                result[file.relative_to(linux).as_posix()] = file.read_bytes()
    for name in ('build.sh', 'install.sh', 'Iniciar-JOCAS.sh', 'gepetas.svg', 'jocas.ico',
                 'LICENSE', 'LEIA-ME.md', 'JOGOS.md', 'VALIDACAO.json'):
        result[name] = (linux/name).read_bytes()
    return result

def publish_voice():
    linux, windows = APP/'Gepetas', APP/'Windows'
    validation = json.loads((linux/'VALIDACAO.json').read_text())
    version = validation['version']
    assert f'setApplicationVersion("{version}")' in (linux/'src/main.cpp').read_text()
    assert (linux/'gepetas').read_bytes().startswith(b'\x7fELF')
    assert (windows/'JOCAS.exe').read_bytes().startswith(b'MZ')
    sources = source_entries(linux)
    source_zip = zip_bytes(sources)
    source_name = f'JOCAS-{version}-Fontes.zip'
    common = {'Windows/'+p.relative_to(windows).as_posix():p.read_bytes()
              for p in windows.rglob('*') if p.is_file()}
    common[source_name] = source_zip
    note = f'''\nVersão {version}. Linux x64: binário para CachyOS, requer Qt 6, libpulse,
Rubber Band 4 e PipeWire/PulseAudio. Em outras distribuições, recompile os fontes.
Windows 11 x64: executável nativo e DLLs incluídos, sem Wine. Para transmitir
voz, instale VB-CABLE separadamente. Driver não incluído.

NOVO: VOZES DE IA
Correção 1.4.1: os botões de IA funcionam durante o uso do microfone. Trocar
de modo pausa a voz para configurar; clique em Iniciar para continuar. A entrada
virtual é mantida durante a pausa.
Usar SEM API: prepare o motor, baixe Cadu/Jeff + reconhecimento ou importe uma
voz Piper .onnx + .onnx.json e uma pasta Faster Whisper multilíngue.
Ativar por API: use sua chave da OpenAI e autorize o envio nesta sessão.
Há vozes públicas e campo para um ID de voz própria já disponível na conta.
Rio não está na lista pública da API de fala. Não inclui clonagem/treinamento.
O motor e os modelos não estão embutidos: Preparar IA baixa Python e dependências
isolados (internet e cerca de 1,5 GB livres, mais modelos adicionais).
Os pacotes incluem o gerenciador uv verificado para o preparo em cada sistema.
Depois dos downloads, o modo local funciona sem internet. A API tem cobrança
separada do ChatGPT. A chave não é salva. Veja Linux/ai/LEIA-ME.md ou
Windows/ai/LEIA-ME.md para instruções e privacidade.
A voz de IA repete suas palavras após a pausa, com atraso por frase e possíveis
erros de reconhecimento; não mistura sua voz original. Efeitos atuais continuam
como outra opção. Escuta e Silenciar funcionam nos dois modos.
Escolha JOCAS Microfone no Discord Linux; CABLE Output no Windows.
Os modelos e ajustes do usuário são preservados nas atualizações.

TESTES
Reconhecimento/síntese locais e envio ao microfone virtual no Linux testados com
fala sintética. Escuta independente e descarte de resultados após silenciar testados.
API validada com simulação, sem chave nem cobrança real. Windows x64 compilado;
dependências Python para Windows verificadas. Não houve chamada real no Discord,
teste de API ao vivo nem validação com hardware Windows 11.
Detalhes em Linux/VALIDACAO.json ou VALIDACAO-LINUX.json.
'''
    install = dict(common)
    install.update({'Linux/'+k:v for k,v in sources.items()})
    install['Linux/gepetas'] = (linux/'gepetas').read_bytes()
    install['Linux/ai/tools/uv'] = (linux/'ai/tools/uv').read_bytes()
    install['COMECE-AQUI.txt'] = (f'''MODIFICADOR DE VOZ JOCAS {version} — INSTALAÇÃO COMPLETA

Extraia TODO o ZIP. Use somente a pasta correspondente ao seu sistema.
Linux: confira as dependências em Linux/LEIA-ME.md e execute bash install.sh
na pasta Linux para criar o atalho. Também pode executar ./gepetas nessa pasta.
Windows: guarde TODA a pasta Windows num local permanente e abra JOCAS.exe.
Mantenha as DLLs e plugins juntos dele. É uma instalação portátil.
Antes de substituir uma instalação, faça backup da pasta e das preferências.
Para uma instalação já existente, prefira o pacote Atualizacao e siga o guia.
Os fontes da mesma versão estão em {source_name}.
''' + note).encode()
    update = dict(common)
    for key,value in sources.items():
        if key.startswith('ai/'):
            update['Linux/'+key] = value
    update['Linux/ai/tools/uv'] = (linux/'ai/tools/uv').read_bytes()
    for name in ('gepetas', 'gepetas.svg'):
        update['Linux/'+name] = (linux/name).read_bytes()
    for name, target in [('LEIA-ME.md','LEIA-ME-LINUX.md'), ('VALIDACAO.json','VALIDACAO-LINUX.json'),
                         ('JOGOS.md','JOGOS.md'), ('LICENSE','LICENSE')]:
        update[target] = (linux/name).read_bytes()
    update['COMECE-AQUI.txt'] = (f'''MODIFICADOR DE VOZ JOCAS {version} — ATUALIZAÇÃO

Contém arquivos completos de substituição; não é uma atualização incremental.
Extraia TODO o ZIP. Feche o JOCAS e pare o microfone virtual antes de copiar.

LINUX
1. Localize a mesma pasta aberta pelo seu atalho. A instalação padrão é
   ~/.local/share/gepetas; uma instalação portátil pode estar em outra pasta.
2. Faça backup dessa pasta fora dela e de ~/.config/Gepetas/Gepetas.conf.
3. Copie TODO o conteúdo da pasta Linux (gepetas, ícone e pasta ai) para a
   MESMA pasta do app, substituindo os arquivos de mesmo nome. Preserve seus modelos
   e preferências na pasta de configurações. Não apague arquivos desconhecidos.
4. Se necessário, execute chmod +x gepetas ai/tools/uv nessa pasta. Use o mesmo atalho.

WINDOWS 11 X64
1. Feche JOCAS.exe. Faça backup da pasta atual fora dela.
2. Copie TODO o conteúdo de Windows para a MESMA pasta do aplicativo atual,
   incluindo DLLs, plugins e licenças. Preserve arquivos pessoais e configurações.
3. Abra pelo mesmo atalho. As preferências do app permanecem no seu usuário.

Este pacote não contém configurações pessoais. Seu perfil Personalizada não é
substituído. As opções de IA ficam em Vozes por IA; os efeitos anteriores permanecem.
Para voltar atrás, feche o app e restaure os arquivos e preferências do backup.
Não tem uma instalação anterior? Use o pacote Instalacao.
Fontes correspondentes: {source_name}.
''' + note).encode()
    prefix = f'Modificador-de-Voz-JOCAS-{version}'
    with tempfile.TemporaryDirectory(prefix='jocas-voz-entregas-') as td:
        archives = []
        for kind, raw in [('Instalacao', zip_bytes(install)), ('Atualizacao', zip_bytes(update)), ('Fontes',source_zip)]:
            path = Path(td)/f'{prefix}-{kind}.zip'
            path.write_bytes(raw); archives.append(path)
            print(path.name, len(raw), 'bytes; integridade conferida')
        publish(DEST, 'Modificador-de-Voz-JOCAS', version, archives)
    print('Pacotes publicados:', DEST)

if __name__ == '__main__':
    publish_voice()

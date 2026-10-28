"""Publicação local: valida o conjunto novo antes de retirar versões antigas."""
from pathlib import Path
import hashlib
import json
import os
import re
import shutil
import tempfile
import zipfile

PRODUCTS = {
    "Modificador-de-Voz-JOCAS": "Modificador de Voz JOCAS",
    "JOCAS-Lista-de-Mods": "JOCAS Lista de Mods",
    "JOCAS-Wallpapers": "JOCAS Wallpapers",
}

def version_key(version):
    if not re.fullmatch(r"\d+\.\d+\.\d+",version):
        raise ValueError("A versão deve ter o formato 1.2.3")
    return tuple(map(int,version.split('.')))

def identify(path):
    for prefix in PRODUCTS:
        match=re.fullmatch(re.escape(prefix)+r"-(\d+\.\d+\.\d+)-(.*)\.zip",Path(path).name)
        if match:
            kind=next((k for k in ('Instalacao','Atualizacao','Fontes')
                if match[2]==k or match[2].endswith('-'+k)),None)
            return prefix,match[1],kind
    return None

def refresh(directory):
    directory=Path(directory)
    files=[];products={}
    for path in sorted(directory.glob('*.zip')):
        info=identify(path)
        if not info:continue
        prefix,version,kind=info
        row={'nome':path.name,'produto':PRODUCTS[prefix],'versao':version,'tipo':kind,
            'bytes':path.stat().st_size,'sha256':hashlib.sha256(path.read_bytes()).hexdigest()}
        files.append(row)
        products.setdefault(PRODUCTS[prefix],{})[kind]={'versao':version,'arquivo':path.name}
    manifest={'produtos':products,'arquivos':files}
    for name,text in [('VERSOES-E-INTEGRIDADE.json',json.dumps(manifest,ensure_ascii=False,indent=2)+'\n'),
        ('SHA256SUMS.txt',''.join(row['sha256']+'  '+row['nome']+'\n' for row in files))]:
        fd,tmp=tempfile.mkstemp(prefix='.catalogo-',dir=directory)
        try:
            with os.fdopen(fd,'w',encoding='utf-8') as f:f.write(text)
            os.replace(tmp,directory/name)
        finally:
            if os.path.exists(tmp):os.unlink(tmp)
    return manifest

def publish(directory,prefix,version,archives):
    directory=Path(directory);key=version_key(version)
    if prefix not in PRODUCTS:raise ValueError('Produto não reconhecido')
    archives=list(map(Path,archives));kinds=set();names=set()
    for path in archives:
        info=identify(path)
        if not info or info[:2]!=(prefix,version) or not info[2]:
            raise ValueError('Pacote de outro produto, versão ou tipo: '+path.name)
        if path.name in names or info[2] in kinds:raise ValueError('Pacote duplicado')
        kinds.add(info[2]);names.add(path.name)
        with zipfile.ZipFile(path) as z:
            if not z.namelist() or z.testzip() is not None:raise ValueError('ZIP inválido: '+path.name)
    if kinds!={'Instalacao','Atualizacao','Fontes'}:
        raise ValueError('Publique instalação, atualização e fontes juntos; os pacotes anteriores foram preservados.')
    directory.mkdir(parents=True,exist_ok=True)
    existing=[p for p in directory.glob('*.zip') if identify(p) and identify(p)[0]==prefix]
    if any(version_key(identify(p)[1])>key for p in existing):
        raise ValueError('Já existe versão mais recente. Nenhum pacote foi removido.')
    with tempfile.TemporaryDirectory(prefix='.publicacao-',dir=directory) as temp:
        temp=Path(temp)
        for path in archives:
            target=temp/path.name;shutil.copy2(path,target)
            if hashlib.sha256(target.read_bytes()).digest()!=hashlib.sha256(path.read_bytes()).digest():
                raise IOError('Falha na cópia de '+path.name)
        # No older release is removed unless every new archive was validated and copied.
        for path in archives:os.replace(temp/path.name,directory/path.name)
        for path in existing:
            info=identify(path)
            if version_key(info[1])<key or (info[1]==version and path.name not in names):path.unlink()
    return refresh(directory)

from pathlib import Path
import subprocess,sys,tempfile,zipfile
ROOT=Path(__file__).resolve().parent
for folder in (ROOT.parent/'Ferramentas/Entregas',ROOT/'ferramentas'):
    if folder.is_dir():sys.path.insert(0,str(folder))
from catalogo import publish
DOWNLOADS=ROOT.parent/'Downloads e Atualizações'

def add(z,p,name):
    info=zipfile.ZipInfo(name,(2026,10,22,0,0,0))
    info.compress_type=zipfile.ZIP_DEFLATED
    info.external_attr=((0o100755 if p.stat().st_mode&0o111 else 0o100644)<<16)
    z.writestr(info,p.read_bytes())

def package():
    common=['install.py','integration.py','LEIA-ME.md','VALIDACAO.json']
    with tempfile.TemporaryDirectory(prefix='jocas-wallpapers-') as temp:
        out=Path(temp)
        for kind in ('Instalacao','Atualizacao'):
            file=out/f'JOCAS-Wallpapers-1.0.0-CachyOS-KDE-Wayland-x64-{kind}.zip'
            with zipfile.ZipFile(file,'w',zipfile.ZIP_DEFLATED) as z:
                for p in sorted((ROOT/'payload').rglob('*')):
                    if p.is_file():add(z,p,p.relative_to(ROOT).as_posix())
                for name in common+['Instalar.sh' if kind=='Instalacao' else 'Atualizar.sh']:add(z,ROOT/name,name)
                msg=('Extraia TODO este ZIP e execute: bash Instalar.sh\nNa primeira instalação, reinicie o Plasma ou saia e entre na sessão KDE.\n'
                    if kind=='Instalacao' else 'Saia pelo ícone do JOCAS na bandeja. Extraia TODO este ZIP e execute: bash Atualizar.sh\nContém arquivos completos de substituição; não é um patch incremental. A versão anterior é guardada em backup.\nComo é a primeira versão, reaplica a 1.0.0.\n')
                z.writestr('COMECE-AQUI.txt','JOCAS Wallpapers 1.0.0 — CachyOS x64 / KDE Wayland\n\n'+msg+'\nLeia LEIA-ME.md para dependências e limites de compatibilidade.\n')
        engine=ROOT/'engine-source'
        source=out/'JOCAS-Wallpapers-1.0.0-Fontes.zip'
        with zipfile.ZipFile(source,'w',zipfile.ZIP_DEFLATED) as z:
            for name in common+['Instalar.sh','Atualizar.sh','build.sh','package.py']:add(z,ROOT/name,name)
            import catalogo
            add(z,Path(catalogo.__file__),'ferramentas/catalogo.py')
            for p in sorted((ROOT/'src').rglob('*')):
                if p.is_file():add(z,p,p.relative_to(ROOT).as_posix())
            files=subprocess.check_output(['git','ls-files','--recurse-submodules','-z'],cwd=engine).decode().split('\0')
            for name in files:
                if name and (engine/name).is_file():add(z,engine/name,'engine-source/'+name)
            z.writestr('COMPILAR.txt','Instale gcc, cmake, pkgconf, wayland-protocols e dependências nativas do motor indicadas em engine-source/README.upstream.md.\nExecute bash build.sh. O CMake baixa o CEF correspondente; pode exigir espaço em disco e acesso à rede.\nO código dos submódulos está incluído e não precisa ser clonado.\n')
        publish(DOWNLOADS,'JOCAS-Wallpapers','1.0.0',sorted(out.glob('*.zip')))
    print('Pacotes publicados em',DOWNLOADS)

if __name__=='__main__':package()

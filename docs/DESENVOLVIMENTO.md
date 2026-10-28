# Desenvolver os projetos JOCAS

Este repositório é uma central de fontes dos produtos existentes, com versões
independentes em `PROJETOS.json`. Alterar um produto não altera a versão dos outros.
As instalações já existentes no computador não são movidas nem substituídas pela
criação deste repositório.

## Obter as fontes

Use `git clone --recurse-submodules https://github.com/JOCASFODAS/JOCAS.git`.
Se já clonou sem submódulos, rode `git submodule update --init --recursive`.
O motor de wallpapers está fixado em `2f41e329d021c1c4c5f765df1a7af8423ffb0ba0`.
Não substitua esse commit por uma branch recente sem compilar e validar a integração.

## Modificador de Voz

O diretório `Modificador de Voz JOCAS/outputs/Gepetas` contém os fontes C++, testes,
helper Python, scripts de instalação e build Linux/Windows. Os binários prontos e
as DLLs ficam nas releases, não no histórico Git.

Linux: use `build.sh` com Qt 6 Widgets, PulseAudio/libpulse e Rubber Band 4. Qt e
bibliotecas devem ser compatíveis com o ambiente da compilação. A versão de Rubber
Band deve oferecer `RubberBandLiveShifter`. Leia `windows/DEPENDENCIAS.md` para
compilar Windows nativo; não é usado Wine para compilar esse produto.

Para preparar os executáveis do gerenciador de IA a partir das fontes, execute
`python3 ai/fetch_tools.py both` dentro do diretório do modificador. Os pacotes
já incluem uv para seu sistema; o motor e os modelos de IA são preparados depois
pelo usuário. As dependências estão fixadas em `ai/requirements.lock`.
`JOCAS_AI_RUNTIME` permite apontar o teste de pipeline a um motor local já instalado.
Testes de áudio usam dispositivos sintéticos; não os aponte ao microfone físico.

## Lista de Mods

Na pasta `JOCAS Lista de Mods`, use `python build.py` com Python 3.11+ e JDK 17+.
O build obtém as APIs de compilação com hashes fixos. Não inclui Minecraft nem
aceita sua EULA automaticamente. Veja `COMPATIBILIDADE.md` antes de anunciar
uma versão como suportada.

## Wallpapers

Na pasta `JOCAS Wallpapers`, use `build.sh` após obter o submódulo e suas
subdependências. Dependências incluem Qt 6, Wayland, FFmpeg/mpv e as bibliotecas
indicadas no motor. O CEF é baixado pelo build; reserve espaço e rede. Nunca
inclua biblioteca Steam, wallpapers ou arquivos da conta no repositório.

## Verificações da central

`python3 tools/verificar_publicacao.py` confere a lista versionada de fontes,
metadados e padrões comuns de dados pessoais/credenciais. É uma verificação
complementar à revisão; não garante detectar todo segredo possível.
`python3 -m unittest discover -s Ferramentas/Entregas -p 'test_*.py'` testa o
catálogo de entregas. Esses testes não substituem as validações de áudio e jogo.

As cópias publicadas dos testes/documentos foram ajustadas para não expor o
caminho da máquina de desenvolvimento. Isso não é uma nova versão dos apps.

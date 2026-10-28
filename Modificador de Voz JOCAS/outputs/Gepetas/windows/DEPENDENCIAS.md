# Dependências — JOCAS 1.2.1

O JOCAS e o Rubber Band 4.0.0 são distribuídos sob GPL-2.0-ou-posterior. O pacote JOCAS-1.2.1-Fontes.zip contém os fontes do aplicativo, scripts de compilação, testes e o arquivo original de fontes Rubber Band usado nesta compilação.

## Bibliotecas Windows

As DLLs Qt 6.12.0 e suas dependências vêm dos pacotes oficiais MSYS2 MINGW64. `msys2-packages.json` registra os pacotes baixados para o ambiente de compilação, suas URLs e SHA-256; nem todos os pacotes desse ambiente são incluídos no aplicativo. A pasta `licenses/MSYS2/` preserva os avisos de licença fornecidos pelos pacotes. As DLLs e os plugins Qt são bibliotecas compartilhadas e podem ser substituídos por versões compatíveis.

- Qt: https://www.qt.io/licensing/open-source-lgpl-obligations
- Fontes Qt: https://code.qt.io/cgit/qt/qtbase.git/
- Receitas e alterações de empacotamento MSYS2: https://github.com/msys2/MINGW-packages
- Pacotes binários MSYS2: https://repo.msys2.org/mingw/mingw64/
- Rubber Band: https://github.com/breakfastquay/rubberband/tree/v4.0.0
- MinGW-w64: https://www.mingw-w64.org/
- GCC e bibliotecas de execução: https://gcc.gnu.org/

O arquivo `windows-system-dlls.txt` lista importações fornecidas pelo próprio Windows. O driver VB-CABLE não está incluído e possui licença própria: https://vb-audio.com/Cable/.

## Linux

A edição Linux usa Qt, libpulse e Rubber Band instalados no sistema. As instruções de dependências e recompilação estão em LEIA-ME.md do pacote Linux e do pacote de fontes.

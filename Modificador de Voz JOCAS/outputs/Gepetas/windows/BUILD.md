# Compilar JOCAS no Windows

Use o ambiente **MINGW64** do [MSYS2](https://www.msys2.org/), com Python 3.12 ou superior:

```bash
pacman -S --needed mingw-w64-x86_64-gcc mingw-w64-x86_64-binutils mingw-w64-x86_64-qt6-base mingw-w64-x86_64-python
python windows/build_windows.py --qt-prefix /mingw64 --compiler g++
```

O resultado fica em `dist/JOCAS-Windows-x64`. O script compila os mesmos `src/main.cpp` e `src/dsp.hpp` usados no Linux, seleciona o backend WASAPI e copia as DLLs importadas e os plugins Qt. O Rubber Band 4.0.0 está incluído como fonte em `third_party/`; seu hash é conferido antes da extração. O driver VB-CABLE deve ser instalado separadamente pelo usuário e não é empacotado.

Para compilar a partir de Linux, use o compilador MinGW-w64 e os arquivos do pacote MSYS2 MINGW64 Qt 6:

```bash
python3 windows/build_windows.py \
  --compiler /caminho/para/x86_64-w64-mingw32-g++ \
  --qt-prefix /caminho/para/mingw64 \
  --out /caminho/para/JOCAS-Windows-x64
```

No Linux, continue usando `./build.sh`. Não compile a edição Windows com as bibliotecas Qt de Linux.

Testes: `tests/dsp_test.cpp` verifica o tom e os controles em 44,1/48/96 kHz; `tests/pcm_test.cpp` verifica os formatos PCM nativos de captura; `tests/windows_audio_test.cpp` recusa abrir dispositivos físicos e só inicia com dispositivos artificiais de nomes específicos. `tests/integration.py` e `tests/monitor_continuity.py` exercitam o PipeWire no Linux.

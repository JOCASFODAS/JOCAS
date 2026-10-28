# Modificador de Voz JOCAS 1.4.1 — Linux e Windows

## Novo: vozes de IA locais e por API

Escolha **Usar SEM API** ou **Ativar por API** em Vozes por IA. O modo local aceita download ou importação de modelos Piper. O modo API usa sua chave e as vozes disponíveis no serviço; Rio não está na lista pública da API de fala. Leia o guia [Vozes por IA](ai/LEIA-ME.md) para preparo, modelos próprios, privacidade e atraso por frase. Não inclui treinamento/clonagem de voz a partir de gravações.

App nativo em português para modificar o microfone em tempo real e usá-lo no Discord, OBS e outros programas que permitem escolher uma entrada de áudio.

## Voz grave clara e saída para Discord (1.4.1)

**Grave** e **Grave clara para Discord** aplicam −5 semitons, abaixam também o
timbre, deixam o corte desligado e usam um realce suave de graves (+1 dB) e
clareza (+2 dB). Não usam o efeito robótico. O tom negativo passa totalmente pelo
modificador: a entrada original não é somada à saída. A intensidade de robô/rádio
mistura somente variantes da voz cujo tom já foi alterado. **Natural** é a opção
explícita para sua voz original. Robô e Rádio agora também usam tom −2.

No Linux, **Conectar a voz modificada ao Discord automaticamente** tenta encaminhar
somente as capturas do Discord nativo (Discord, Canary, PTB ou Vesktop) para
**JOCAS Microfone** enquanto o JOCAS está ao vivo. O app confere a troca antes de
confirmar. Se o Discord bloquear, selecione essa entrada em Voz e vídeo e use
**Conectar Discord** para tentar novamente. Para Discord no navegador, escolha
a entrada manualmente. Uma escolha manual posterior não é sobrescrita repetidamente
na mesma captura. A opção pode ser desligada. Não altera os arquivos de configuração
do Discord, outros programas, nem o microfone físico.

Ao iniciar, os dispositivos virtuais próprios ficam em 100% e sem mudo; o volume
enviado é ajustado no JOCAS. Isso evita uma atenuação antiga restaurada pelo sistema.
A normalização ocorre na inicialização e após 500 ms para absorver a restauração
tardia do PipeWire. Ajustes manuais posteriores não são continuamente sobrescritos.
Os testes cobriram mudança de tom, ausência do tom original, escuta independente,
mudo e encaminhamento apenas do Discord com dispositivos e sinais artificiais.
Uma chamada real e a compreensão de fala humana ainda precisam de confirmação.

Ao atualizar pelos pacotes, suas preferências e a Personalizada continuam salvas.
Escolha **Grave clara para Discord** para ativar o novo som. Nesta máquina, o
perfil Grave foi aplicado a pedido do usuário e a Personalizada anterior foi mantida.

## Uma versão, dois sistemas

A versão 1.4.1 compartilha interface, efeitos e controles entre CachyOS/Linux e Windows 11 x64. Cada sistema tem seu executável nativo: `gepetas` no Linux e `JOCAS.exe` no Windows. Não é necessário Wine.

No Linux, o app cria **JOCAS Microfone** pelo PipeWire/PulseAudio. No Windows, usa WASAPI e o **VB-CABLE**, instalado separadamente. As instruções de Windows estão em `windows/LEIA-ME-WINDOWS.md`; a compilação está em `windows/BUILD.md`. O pacote Windows inclui as DLLs necessárias.

A edição Windows 1.4.1 foi compilada sem Wine. O funcionamento com dispositivos reais e Discord no Windows 11 ainda não foi validado. As preferências são locais a cada sistema.

## Abrir e usar no Linux

Abra **Modificador de Voz JOCAS** no menu de aplicativos, se instalado. Você também pode executar `Iniciar-JOCAS.sh` nesta pasta ou abrir o executável `gepetas`. Se o gerenciador de arquivos perguntar, escolha **Executar**.

1. Selecione seu microfone físico e seus fones.
2. Escolha **Natural**, **Grave**, **Aguda**, **Robô**, **Rádio** ou **Monstro**.
3. Clique em **Iniciar microfone virtual**.
4. No Discord, abra **Configurações → Voz e vídeo → Dispositivo de entrada** e escolha **JOCAS Microfone**. Mantenha a saída do Discord nos seus fones.
5. Marque **Escutar a voz enviada (efeitos ou IA)** quando quiser se ouvir. Desmarque para desligar apenas a escuta: o envio para o Discord continua.

Mantenha o JOCAS aberto enquanto o utiliza. É possível minimizar a janela. A captura e a escuta começam desligadas sempre que você abre o app.

**Silenciar**, ou **Ctrl+M** enquanto a janela está em foco, zera o áudio enviado e ouvido. **Parar microfone virtual** e fechar a janela removem a entrada virtual. Se quiser ficar em silêncio mantendo a entrada selecionada no Discord, use **Silenciar**: ao remover um dispositivo, outros aplicativos podem mudar para outra entrada disponível.

## Ajustes

- **Tom da voz:** de −12 a +12 semitons. Valores negativos deixam a voz grave; positivos deixam aguda.
- **Preservar o timbre:** mantém mais das características originais enquanto altera o tom.
- **Volume enviado:** afeta o áudio recebido pelo Discord e outros apps. O limitador impede valores acima do teto digital; volumes exagerados ainda podem distorcer.
- **Corte de ruído baixo:** silencia trechos abaixo do limiar. Comece desligado; aumente gradualmente se necessário. Não é remoção de ruído por IA.
- **Volume só nos seus fones:** controla apenas a escuta, independentemente do volume enviado.

Use fones durante a escuta para evitar realimentação. A alteração de tom acrescenta atraso de processamento; a biblioteca informa cerca de 50 ms ou mais, além dos buffers do sistema. Bluetooth pode acrescentar atraso. Não é um clonador de voz por IA.

A versão 1.2.0 usa buffer de 96 ms, com pré-carga de 64 ms, na saída virtual Linux; na escuta são 120 ms e 96 ms. A reserva maior evita cortes quando a entrega de áudio oscila, com um pouco mais de atraso. No Windows, a saída virtual reserva 120 ms com pré-carga de 64 ms; o áudio real no Windows ainda precisa de validação.

## Correção de fala e voz Personalizada (1.2.0)

A saída virtual antiga teve 56 lacunas de pelo menos 2 ms em 8 segundos de teste com pausas artificiais de 35 ms no processo. A nova saída teve zero lacunas no mesmo teste. Isso verifica continuidade no PipeWire, não a qualidade de uma chamada real.

O corte de ruído agora aguarda 180 ms antes de começar a fechar e reduz o som gradualmente, preservando melhor as partes baixas entre sílabas. Na primeira abertura após atualizar, o corte fica desligado, o timbre fica preservado, o tom fica entre −5 e +5 e o volume enviado fica limitado a 100%. Os valores anteriores ficam guardados no grupo `beforeClarityUpdate` das preferências. Microfone, fones e volume de escuta são mantidos.

As vozes prontas usam alterações mais leves, corte desligado e volume 100%. O limitador reduz o volume antes da sobrecarga, em vez de achatar os picos da fala. **Grave clara para Discord** seleciona a voz grave clara sem apagar sua criação personalizada.

Clique em **Personalizada** para editar e guardar uma voz própria. Ajustes manuais também selecionam esse modelo. Ele oferece:

- Tom, preservação do timbre, volume enviado e corte de ruído.
- Efeito extra: nenhum, robô/metálico ou rádio/comunicador.
- Intensidade do efeito, de 0% (voz com o tom escolhido, sem o efeito extra) a 100%.
- Frequência do robô e distorção do rádio, disponíveis conforme o efeito selecionado.
- Graves e agudos/clareza, de −12 a +12 dB.

Os controles mudam o som ao vivo. Sua Personalizada é mantida ao escolher outra voz e salva quando o app inicia o áudio ou fecha normalmente. Tom, distorção e corte extremos ainda podem prejudicar a compreensão; use **Grave clara para Discord** para comparar. Iniciar, Parar e Silenciar permanecem visíveis enquanto você rola os ajustes.

Para verificar no Discord: reinicie o JOCAS atualizado, clique em **Grave clara para Discord**, inicie e selecione **JOCAS Microfone**. Comece com Grave e ajuste o tom gradualmente. Se ainda faltar fala, experimente desligar a supressão de ruído/Krisp e ajustar a sensibilidade de entrada nas opções do Discord. Use fones.

## Modo jogos (Linux, 1.4.1)

Para jogos como R.E.P.O., marque **Modo jogos: usar JOCAS como microfone padrão**, inicie o microfone virtual e depois abra o jogo. O nome da entrada é **JOCAS Microfone**. O modo afeta também outros apps que usam a entrada padrão do sistema.

Quando o processamento inicia, o JOCAS guarda a entrada padrão anterior e seleciona a virtual. Ao parar, desmarcar a opção ou fechar normalmente, restaura a entrada anterior se ela ainda estiver disponível. Se você escolher manualmente outro microfone nesse intervalo, essa escolha é preservada. A opção começa desligada e fica salva após ser alterada. Não modifica volumes nem a saída de som.

O modo não substitui uma seleção explícita feita pelo jogo. Se o R.E.P.O. estiver configurado para capturar HDMI/Monitor, escolha **JOCAS Microfone** dentro dele. Se o nome não aparecer, deixe o JOCAS iniciado e reabra o jogo. Jogos que bloqueiam a troca de dispositivo durante a partida precisam reabrir sua captura ou reiniciar. Não é necessário usar Wine para executar o JOCAS Linux.

A seleção e a restauração foram testadas com dispositivos artificiais. O R.E.P.O. estava fechado nesta verificação; a confirmação do som na partida depende do seu próximo teste.

## Se não aparecer no Discord

Inicie primeiro o microfone virtual e reabra a lista de entradas do Discord. Se necessário, reinicie o Discord. Em navegador, permita o uso do microfone e selecione a entrada virtual. Outros apps devem selecionar **JOCAS Microfone** da mesma forma.

Se os efeitos ficarem cortados, experimente desativar a supressão de ruído/Krisp nas opções do Discord, quando disponível, e ajuste a sensibilidade de entrada. O suporte do Discord inclui a desativação do Krisp entre as etapas de diagnóstico.

Se o microfone físico for desconectado, o JOCAS interrompe o processamento e mostra o erro. Reconecte o dispositivo, clique em **Atualizar** e inicie novamente. Se o servidor de áudio estiver indisponível, o app mostra a falha e não inicia a captura.

## Instalação e recompilação

Nesta máquina, as dependências foram encontradas e o executável já está compilado para Linux x86-64. Não precisa de Wine nem de privilégios de administrador para executar.

Para adicionar o app ao menu, execute `./install.sh`. Ele copia o executável e o ícone para `~/.local/share/gepetas` e cria `~/.local/share/applications/gepetas.desktop`, respeitando `XDG_DATA_HOME` quando definido. Não cria inicialização automática.

Dependências de execução: `qt6-base`, `libpulse`, `rubberband` 4 ou superior, servidor PipeWire com `pipewire-pulse` ou PulseAudio compatível. O instalador usa Python 3. Para recompilar, também são necessários `gcc` e `pkgconf`:

```bash
sudo pacman -S --needed gcc pkgconf qt6-base libpulse rubberband python
./build.sh
./Iniciar-JOCAS.sh
```

Não execute o app com sudo. Em outra distribuição ou após uma atualização que altere as bibliotecas, recompile com `build.sh`.

Para remover a instalação, feche o app e apague a pasta `~/.local/share/gepetas` e o arquivo `~/.local/share/applications/gepetas.desktop`. As preferências ficam em `~/.config/Gepetas/Gepetas.conf` e podem ser apagadas separadamente. A pasta com os fontes pode ser guardada.

## O que foi verificado

Testado no CachyOS com Qt 6.12.0, PipeWire 1.6.9 e Rubber Band 4.0.0:

- Compilação nativa e inspeção visual da interface.
- Mudanças de tom em −12, −5, 0, +6 e +12 semitons, em 44,1/48/96 kHz; silêncio, limitador, efeitos e corte de ruído.
- Interface real iniciando e parando o microfone virtual com um sinal artificial de 220 Hz; saída medida em 440 Hz com +12 semitons.
- Escuta ligada e desligada mantendo o envio virtual contínuo; volume da escuta independente.
- Botão **Silenciar** zerando a saída.
- Escuta sem lacunas maiores que 2 ms em um teste de 8 segundos com pausas artificiais de 35 ms no processamento.
- Conversão dos formatos PCM de 16/24/32 bits e float de 32 bits usados pelo backend Windows.
- Remoção dos dispositivos virtuais no encerramento normal e manutenção dos dispositivos padrão originais do sistema.

Os testes de integração usaram somente dispositivos virtuais e áudio artificial. Não gravaram o microfone físico e não fizeram uma chamada no Discord. A confirmação da qualidade com sua voz e da configuração dentro do Discord depende do seu uso.

Os testes estão em `tests/`. O teste de integração pode ser executado com `python3 tests/integration.py` (requer NumPy e as dependências de compilação). Feche o JOCAS antes. Ele cria dispositivos temporários e os remove ao terminar.

O app processa o som localmente e não grava nem envia áudio para servidores próprios. Os aplicativos que recebem o microfone virtual, como o Discord, seguem seus próprios fluxos de áudio.

## Referências e licença

- [PipeWire: remapeamento de fonte](https://pipewire.pages.freedesktop.org/pipewire/page_pulse_module_remap_source.html)
- [Rubber Band: processamento de tom e latência](https://www.breakfastquay.com/rubberband/code-doc/classRubberBand_1_1RubberBandLiveShifter.html)
- [Discord: solução de problemas de voz e vídeo](https://support.discord.com/hc/pt-br/articles/360045138471-Guia-de-solu%C3%A7%C3%A3o-de-problema-de-voz-e-v%C3%ADdeo-no-Discord)

Código do JOCAS sob GPL-2.0-ou-posterior; veja `LICENSE`. Usa Qt, libpulse e Rubber Band instalados no sistema, cada qual com sua licença. Os fontes completos do app acompanham este pacote.

Correção 1.4.1: os botões de modos de IA podem ser usados com a voz ao vivo.
Ao trocar o modo, o áudio pausa e a entrada virtual é mantida. Configure e clique
em Iniciar microfone virtual para continuar.

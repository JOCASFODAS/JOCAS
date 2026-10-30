# Modificador de Voz JOCAS 1.4.2 — Windows 11 x64

Esta edição usa a mesma interface, os mesmos efeitos e o mesmo código de processamento da edição Linux 1.4.2. O executável de Windows usa WASAPI; o de Linux usa PipeWire/PulseAudio.

## Grave clara (1.4.2)

Use **Grave** ou **Grave clara para Discord**: tom −5, timbre mais baixo,
sem robô, volume 100%, corte desligado, graves +1 dB e clareza +2 dB.
O sinal original não é somado ao tom modificado. **Natural** envia a voz original.
Sua Personalizada anterior continua salva. O botão Grave não a substitui.
O encaminhamento automático do Discord é exclusivo do Linux; no Windows,
selecione **CABLE Output** manualmente como entrada e mantenha a saída nos fones.

## Primeiro uso

1. Extraia o ZIP inteiro para uma pasta. Mantenha `JOCAS.exe`, as DLLs e a pasta `plugins` juntos.
2. Instale o **VB-CABLE** pelo [site oficial da VB-Audio](https://vb-audio.com/Cable/). Execute o instalador do driver como administrador e reinicie o Windows após a instalação. O driver é separado do JOCAS e não está incluído neste pacote.
3. Abra `JOCAS.exe`. O JOCAS não precisa ser executado como administrador.
4. Selecione seu microfone físico e seus fones; clique em **Atualizar** se acabou de instalar o cabo.
5. Escolha um efeito e clique em **Iniciar microfone virtual**.
6. No Discord, em **Configurações → Voz e vídeo → Dispositivo de entrada**, escolha **CABLE Output (VB-Audio Virtual Cable)**.
7. Para ouvir sua voz modificada, marque **Escutar a voz enviada (efeitos ou IA)**. Desmarcar desliga apenas a escuta.

O JOCAS envia a voz processada para **CABLE Input**; os outros aplicativos recebem essa voz pela entrada **CABLE Output**. Mantenha o som do Windows e a saída do Discord nos seus fones, para que o áudio do computador não seja enviado ao cabo.

Use o VB-CABLE básico, com os nomes padrão **CABLE Input** e **CABLE Output**: esta versão detecta esses dois dispositivos automaticamente. Se eles forem renomeados ou desativados, o app solicitará a configuração do cabo. Não são necessários Wine, Python, Qt ou compilador no computador de quem usa este pacote.

## Controles

- Natural, Grave, Aguda, Robô, Rádio e Monstro.
- Tom de −12 a +12 semitons e preservação opcional de timbre.
- Volume enviado, corte de ruído baixo e volume de escuta independente.
- **Silenciar** / **Ctrl+M** com a janela em foco: silencia a voz e a escuta.
- **Parar**: interrompe a captura e o envio; o driver VB-CABLE continua instalado.

A captura e a escuta começam desligadas quando o app abre. A escuta tem atraso de processamento e um buffer para dar estabilidade. As preferências do Windows são próprias desse sistema; não são sincronizadas automaticamente com as do Linux.

## Personalizada e fala clara

Clique em **Personalizada** para ajustar tom, timbre, efeito extra, intensidade, frequência do robô, distorção do rádio, graves, agudos, volume e corte de ruído. Sua criação é mantida ao trocar de voz e salva ao iniciar o áudio ou fechar normalmente. **Grave clara para Discord** seleciona voz grave clara com volume 100% e corte desligado sem apagar sua criação.

Na primeira abertura da atualização, o corte de ruído antigo é desligado, o timbre é preservado, o tom fica entre −5 e +5 e o volume não ultrapassa 100%. Os ajustes antigos ficam guardados nas preferências em `beforeClarityUpdate`. O limitador e o corte de ruído foram suavizados. A reserva de áudio enviada ao cabo foi aumentada para absorver oscilações na entrega do microfone.

Se as palavras desaparecerem no Discord, teste a voz natural e experimente desativar a supressão de ruído/Krisp nas opções de Voz e vídeo.

## Se não houver som

Verifique as permissões de microfone para aplicativos da área de trabalho em **Configurações do Windows → Privacidade e segurança → Microfone**. Confirme que o microfone não está silenciado e que **CABLE Output** está selecionado como entrada no Discord. Feche programas que estejam usando o dispositivo em modo exclusivo e clique em Atualizar.

## Validação e limites desta entrega

O executável Windows x64 da versão 1.4.2 foi compilado e suas dependências conferidas. Os testes do processamento compartilhado passaram no Linux em 44,1, 48 e 96 kHz; os testes de troca e persistência da voz Personalizada também passaram. **Ainda é necessário validar os dispositivos reais, o driver VB-CABLE e uma chamada no Windows 11.** Não foi utilizado Wine para esta atualização. A compilação não possui assinatura digital.

O VB-CABLE mantém sua própria licença. O JOCAS é distribuído sob GPL-2.0-ou-posterior; veja `LICENSE`. As bibliotecas de terceiros acompanham suas licenças em `licenses/`. Os fontes completos do JOCAS e do Rubber Band usado na compilação estão no pacote de fontes da mesma versão. As referências das dependências estão em `DEPENDENCIAS.md`.


## Vozes de IA

Os botões **Usar SEM API** e **Ativar por API** usam o mesmo reconhecimento e síntese da edição Linux. Clique **Preparar IA** no primeiro uso; não precisa instalar Python manualmente. No modo local, baixe ou importe modelos Piper e Faster Whisper. No modo API, use sua chave da OpenAI e autorize o envio nesta sessão. Rio não está na lista pública de vozes da API de fala. Veja [o guia completo](ai/LEIA-ME.md). Há atraso por frase. O microfone original não é misturado à síntese.

Na 1.4.2, os botões de modos de IA ficam disponíveis durante o uso: escolher outro
modo pausa o envio para liberar a configuração. Clique em Iniciar para continuar.

## Atualização 1.4.2

O executável foi recompilado a partir dos fontes 1.4.2. A correção de cortes sob
carga de jogos desta edição é específica de PipeWire/PulseAudio no Linux. O backend
WASAPI do Windows permanece igual; não foi validado em hardware Windows 11.

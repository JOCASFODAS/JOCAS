# JOCAS 1.4.2 em jogos

## Proteção contra cortes no Linux

A versão 1.4.2 protege envio e escuta com reserva de 200 ms, prioridade de áudio
quando permitida e recuperação de fragmentos acumulados sem o descarte antigo de
100 ms. A mudança pode acrescentar cerca de 100 ms ao atraso anterior de envio.
Não altera o jogo nem limita seus FPS. Desconexões USB ou bloqueios maiores que a
reserva ainda podem causar interrupções. Um teste de chamada no The Finals/Discord
com fala real continua necessário; veja `VALIDACAO.json`.

## CachyOS / Linux

1. Reabra o JOCAS atualizado.
2. Marque **Modo jogos: usar JOCAS como microfone padrão** na seção Jogos.
3. Clique em **Iniciar microfone virtual**.
4. Abra o R.E.P.O. e selecione **JOCAS Microfone** nas opções de entrada de voz. Se usar a entrada padrão do sistema, o Modo jogos seleciona o JOCAS para ela.
5. Mantenha o JOCAS ativo enquanto joga. Se ele não aparecer na lista, reabra o jogo depois de iniciar o microfone virtual.

A configuração anterior do R.E.P.O. estava capturando o monitor HDMI. O redirecionamento aplicado àquela sessão funcionou no servidor de áudio, mas uma escolha explícita do jogo pode substituí-lo na próxima abertura. Por isso, confirme a entrada nas opções do jogo. Alguns jogos bloqueiam a troca durante uma partida.

O Modo jogos muda a entrada padrão para todos os apps que a utilizam; não altera volumes nem o som de saída. Ao parar o JOCAS, restaura o microfone padrão anterior, se ele estiver disponível. Se você tiver escolhido outro manualmente, respeita essa escolha. A opção pode ser desmarcada a qualquer momento.

## Windows 11

Use o JOCAS nativo com VB-CABLE e escolha **CABLE Output (VB-Audio Virtual Cable)** como entrada do jogo. Inicie o JOCAS antes de abrir o jogo. O modo que troca automaticamente a entrada padrão é específico da edição Linux; no Windows, selecione a entrada nas configurações do jogo ou do próprio sistema.

## Verificação

Os testes de seleção/restauração e do envio de áudio passaram no Linux, usando dispositivos artificiais. Não foi feita uma partida nem capturado áudio do microfone físico. A edição Windows está compilada e continua pendente de validação em hardware Windows 11.

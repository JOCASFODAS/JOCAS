# Vozes por IA — JOCAS 1.4.1

Em **Vozes por IA**, escolha **Usar SEM API** ou **Ativar por API**. O botão
**Preparar IA** instala Python 3.12 isolado e as dependências fixadas em
`requirements.lock`, com conferência dos hashes. Não modifica o Python do sistema.
Requer internet no preparo e aproximadamente 1,5 GB livres, mais modelos adicionais.
Linux e Windows 11 x64 usam a mesma interface e os mesmos modelos.

## Sem API

1. Prepare a IA uma vez.
2. Escolha Cadu ou Jeff e clique **Baixar voz + reconhecimento**. Cada voz usa
   aproximadamente 63 MB e o reconhecimento Base aproximadamente 148 MB.
3. Ou baixe manualmente uma voz Piper eSpeak/text `.onnx` e sua configuração `.onnx.json` e use
   **Adicionar minha voz**. A voz pode ter sido treinada por você ou por terceiros.
   Os arquivos são copiados para sua biblioteca, sem substituir outras vozes.
4. Para reconhecimento manual, baixe todos os quatro arquivos `model.bin`,
   `config.json`, `tokenizer.json` e `vocabulary.txt` de um Faster Whisper multilíngue
   e selecione a pasta. Pode ser Base ou outro tamanho compatível. Modelos `.en`
   não reconhecem português. Modelos maiores usam mais memória e processamento.
5. Ajuste velocidade, variação e, em modelos com vários falantes, o número do
   falante. Para os modelos simples incluídos no catálogo, o número é 0.

Depois do preparo e dos downloads, a conversão é local, sem enviar fala à internet.
O carregamento usa somente caminhos locais. Não importa `.pth`, scripts Python,
modelos RVC ou gravações `.wav` como modelos Piper. Não inclui treinamento nem
clonagem a partir de gravações. Respeite a licença e os direitos das vozes importadas.

Catálogo: https://huggingface.co/rhasspy/piper-voices/tree/main/pt/pt_BR
Reconhecimento: https://huggingface.co/Systran/faster-whisper-base/tree/main
Os downloads do catálogo têm revisão fixa e SHA-256 em `catalog.json`; o cartão
com a origem/licença de cada voz é salvo junto ao modelo. Cadu e Jeff indicam
conjunto de dados CC0. Não representam vozes do ChatGPT.

## API

1. Prepare a IA uma vez. Não precisa baixar os modelos locais para usar API.
2. Selecione **Ativar por API**, digite sua chave da API OpenAI, escolha uma voz e
   autorize o envio de áudio e texto para essa sessão.
3. Opcionalmente informe o ID de uma voz personalizada já disponível na sua conta
   (`voice_…`). Esse ID substitui a seleção da lista. A criação da voz na conta
   não é feita pelo JOCAS. Instruções de estilo ajustam a maneira de falar.

A chave fica somente na memória da sessão e no ambiente do processo de IA; não é
salva em preferências, arquivos, argumentos ou registros. A autorização de envio
não é restaurada ao reabrir. API tem cobrança própria, separada do ChatGPT.
A fala é enviada à OpenAI para reconhecimento e o texto é enviado para síntese.
O app não usa um modelo de conversa e não tenta responder/reformular sua frase.
Vozes públicas: marin, cedar, alloy, ash, ballad, coral, echo, fable, nova, onyx,
sage, shimmer e verse. **Rio não é uma voz pública da API de fala consultada.**
Não rotulamos outras vozes como Rio.

Documentação: https://developers.openai.com/api/docs/guides/text-to-speech
Reconhecimento: https://developers.openai.com/api/docs/guides/speech-to-text

## Falar e transmitir

Escolha o microfone físico em Conexões, o modo de IA e inicie o microfone virtual.
Faça pausas de aproximadamente 0,65 s entre frases. A IA precisa reconhecer e
sintetizar o texto: acrescenta atraso e pode reconhecer palavras incorretamente.
No teste sintético deste PC, a conversão local de uma frase de aproximadamente
5 s levou cerca de 2 s após o áudio ficar disponível; isso não é garantia de latência.
Há limite de 12 s por trecho capturado, duas frases aguardando processamento e
30 s de áudio aguardando reprodução. Quando lotar, o app avisa e descarta o excesso.
Prefira frases curtas e aguarde a voz terminar. Use fones para evitar realimentação.

Somente a voz sintetizada sai nesse modo. Os controles de tom, robô, rádio e EQ dos
efeitos atuais não são aplicados à IA; o Volume enviado continua valendo. A escuta
pode ser ligada ou desligada e o botão Silenciar também descarta a fila de IA e
resultados de frases anteriores. Pequeno áudio já entregue ao dispositivo pode
levar o tempo do buffer para parar. Sem reconhecimento ou em falhas, sai silêncio.
No Discord Linux escolha **JOCAS Microfone**; no Windows, **CABLE Output** do
VB-CABLE. O JOCAS não pode impedir que outro app selecione o microfone físico:
confira a entrada no jogo/Discord. Deixe claro aos participantes que usa voz de IA.

## Dados e atualizações

Os modelos e o motor ficam em `~/.config/Gepetas/ia` no Linux e na pasta
`Gepetas/ia` dentro da localização de configurações do Qt no usuário Windows
(normalmente `%LOCALAPPDATA%`). As preferências antigas continuam no mesmo local.
Atualizações substituem a pasta `ai` do aplicativo, preservando os modelos e ajustes
no usuário. Não mova o motor isolado: recrie com Preparar IA se mudar seu local.
Não são gravadas frases, transcrições ou áudios de conversão em arquivos pelo app.

## Fontes e dependências

Fontes do app: GPL-2.0-or-later; helper de IA: GPL-3.0-or-later. O pacote inclui
GPL-3.0 para Piper; os processos são separados. Piper: GPL-3.0,
https://github.com/OHF-Voice/piper1-gpl (PIPER-LICENSE).
Faster Whisper: MIT, https://github.com/SYSTRAN/faster-whisper .
CTranslate2: MIT; ONNX Runtime: MIT; modelo Whisper Base: MIT.
Os pacotes Python são obtidos do PyPI com versões/hashes fixos no arquivo lock;
suas licenças e metadados ficam na instalação isolada.
`uv` 0.13.5: MIT ou Apache-2.0, https://github.com/astral-sh/uv ; licenças em tools/.
Python isolado: obtido pelo uv do python-build-standalone, com licenças junto ao
runtime: https://github.com/astral-sh/python-build-standalone .
Para montar a partir dos fontes, execute `python3 ai/fetch_tools.py both` antes
de empacotar (ou linux/windows para somente um sistema). Binários do uv não são
incluídos no ZIP Fontes. Não execute os testes com seu microfone real.

## Trocar de modo durante o uso (1.4.1)

Os botões Efeitos atuais, Usar SEM API e Ativar por API ficam disponíveis com o
microfone ligado. Ao escolher outro modo, a voz é pausada e os ajustes são liberados.
Configure a voz e clique em Iniciar microfone virtual para continuar. No Linux,
a entrada virtual continua conectada e em silêncio durante essa pausa.
Os botões ficam temporariamente bloqueados durante preparo ou download dos modelos.

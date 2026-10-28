# JOCAS Lista de Mods 1.0.0

Mod para **Minecraft Java Edition**. Lista os mods reconhecidos pelo carregador e abre uma **janela auxiliar fora do jogo** com busca, botão **Copiar relatório para o GPT** e exportação `.txt`.

## Instalar

1. Feche o Minecraft.
2. Escolha **somente um** arquivo conforme o carregador do seu modpack:
   - Fabric: `JOCAS-Lista-de-Mods-1.0.0-fabric.jar`.
   - Forge: `JOCAS-Lista-de-Mods-1.0.0-forge.jar`.
   - NeoForge: `JOCAS-Lista-de-Mods-1.0.0-neoforge.jar`.
   - Exceção: NeoForge **1.20.1** ainda usa a variante **forge**.
3. Coloque esse arquivo na pasta `mods` da instância. No Prism Launcher: editar instância → Mods → Adicionar.
4. Abra o jogo. Uma janela “JOCAS • Lista de Mods” será aberta automaticamente quando o carregador inicializar este mod. Use Alt+Tab para encontrá-la.
5. Clique em **Copiar relatório para o GPT** e cole no chat com Ctrl+V. No Linux, mantenha a janela aberta até colar.

Não coloque `JOCAS-Visualizador.jar` na pasta `mods`: ele é apenas o visualizador separado. Não é necessário Fabric API. Não é necessário instalar Java adicional se o seu jogo já tem um Java completo compatível.

## O que aparece no relatório

Minecraft, carregador, Java, sistema operacional, data da coleta, nomes, IDs, versões, nomes dos arquivos e dependências declaradas. Inclui bibliotecas internas: o número pode ser maior que a quantidade de arquivos na pasta `mods`.

A busca filtra apenas a tabela. O botão copia **todos** os mods e dependências. A aba “Relatório completo” mostra exatamente o texto copiado.

A lista não diagnostica nem corrige erros sozinha. Descreva o problema ao GPT e forneça logs separadamente se necessário. Se uma incompatibilidade impedir a inicialização deste mod, ele não conseguirá produzir uma lista nova. A data da coleta permite identificar relatórios antigos.

## Relatório salvo e reabertura

Dentro da pasta do jogo, `jocas-modlist/` contém:

- `relatorio-mods.txt`: texto pronto para compartilhar.
- `snapshot.properties`: dados que o visualizador abre.
- `JOCAS-Lista-de-Mods.jar`: visualizador para reabrir por duplo clique, se o sistema associa `.jar` ao Java.
- `janela.log`: mensagens do visualizador, caso ele não abra.

O visualizador distribuído separadamente também permite escolher um `snapshot.properties`. Pela linha de comando: `java -jar JOCAS-Visualizador.jar caminho/para/snapshot.properties`.

Fechar a janela não fecha o Minecraft. A janela aberta automaticamente encerra quando o processo do jogo termina; os arquivos salvos permanecem.

## Linux, Windows e servidores

Os mesmos arquivos Java são destinados a Linux e Windows. A janela requer Java com `java.desktop` e uma sessão gráfica (no Linux, acesso ao X11/XWayland). Não usa Wine. Se a janela falhar, use o arquivo `.txt` salvo.

O mod é opcional no servidor. Se instalado num servidor dedicado, somente salva a lista do servidor; não abre janela nem transmite dados aos jogadores. Instalado no cliente, lista a instância local, não os mods exclusivos de um servidor remoto.

Para desativar a abertura automática, adicione o argumento Java `-Djocas.modlist.noWindow=true` no launcher. A exportação em texto continua funcionando.

## Compatibilidade

Foi projetado para evitar as classes e telas internas do Minecraft: utiliza apenas metadados dos carregadores. Os metadados permitem versões a partir da 1.20.1 (NeoForge separado: 1.20.2), inclusive numeração 26.x. Isso **não equivale a testar ou garantir todas as versões**, especialmente lançamentos futuros. Consulte `COMPATIBILIDADE.md` para o alcance real da validação.

## Privacidade

Nada é enviado pela rede. Não lê logs, contas, tokens ou configurações. O relatório inclui nomes de mods e nomes de arquivos; revise o texto antes de compartilhar. Não inclui os diretórios completos dos arquivos.

## Compilar o código-fonte

Requer Python 3.11+ e JDK 17+. Defina `JAVA_HOME` se necessário e execute `python build.py`. As três APIs de compilação são baixadas de repositórios oficiais, verificadas por SHA-256 e não são incluídas nos mods. Os arquivos ficam em `dist/`. O visualizador é incluído como recurso em cada variante.

Licença MIT.

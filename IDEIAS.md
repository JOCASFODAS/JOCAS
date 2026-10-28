# O que podemos fazer a seguir

Propostas para escolher e desenvolver. Não são funcionalidades disponíveis nas
versões atuais e não possuem data prometida.

## 1. JOCAS Central — novo aplicativo

Uma tela com os apps e mods JOCAS, versão instalada, novidades e botões de
Instalar, Atualizar e Abrir. Antes de atualizar, faz backup e preserva os ajustes.
Mostra o tamanho do download e confere SHA-256. Linux e Windows como destinos,
com detecção do sistema e do tipo de pacote. Atualizações começam por uma ação
do usuário. Dependências e drivers externos continuam identificados.

**Primeira entrega:** catálogo dos três produtos, abrir a release correta e
identificar instalação/atualização. A instalação automática vem depois de validar
cada produto e os caminhos existentes.

## 2. Modificador de Voz — conferir o que está saindo

Uma seção “O que os outros estão recebendo?” que mostra microfone físico,
saída virtual, modo de voz e entrada em uso pelo Discord quando for detectável.
Oferece teste local curto iniciado pelo usuário e instruções para selecionar a
entrada virtual. Não grava nem envia áudio sem uma ação explícita.

Também pode oferecer proteção para vozes modificadas: avisar ou aplicar um tom
modificado quando a Personalizada ficar em zero, mantendo Natural como escolha
explícita. Formantes, intensidade e volume precisam continuar compreensíveis.
Não prometer anonimato, voz de outra pessoa ou clonagem só com mudança de tom.

**Prioridade sugerida:** alta, antes de acrescentar mais efeitos.

## 3. Modificador de Voz — biblioteca de vozes

Prévia por botão, nomes próprios para modelos importados, favoritos, grupos e
checagem de arquivos compatíveis. Predefinições como narrador grave, robô claro
e rádio. No modo IA, mostrar “Ouvindo”, “Reconhecendo” e “Reproduzindo”, com
controle da fila e tempo medido. APIs pagas devem mostrar isso claramente.

**Primeira entrega:** favoritos e prévia; treinamento/clonagem de novas vozes é
um projeto separado e requer modelos e dados adequados.

## 4. JOCAS Doutor do Modpack — novo mod com aplicativo auxiliar

Seleciona a pasta de uma instância e combina lista de mods, versões, dependências
e trechos relevantes de `latest.log`/crash report. Aponta IDs duplicados, arquivos
de carregadores diferentes e dependências declaradas ausentes quando possível.
Não garante detectar todas as incompatibilidades nem substitui um teste real.

O relatório passa por uma prévia para ocultar caminhos pessoais, nomes de usuário
e possíveis tokens antes de copiar/exportar. Nada é enviado sozinho. Um
aplicativo auxiliar permite ler o relatório mesmo se o jogo não inicializar.
Não aceitar automaticamente EULA, apagar mods ou alterar mundos.

**Primeira entrega:** importação manual de lista + log, revisão e exportação;
depois integração com Fabric/Forge/NeoForge por faixas testadas.

## 5. JOCAS Contratos — novo mod de Minecraft

Um quadro de missões cooperativas: coletar itens, explorar biomas e derrotar
criaturas para receber recompensas escolhidas pelo dono do modpack. Missões
configuráveis, progresso por equipe e opção de bloquear recompensas repetidas.

**Primeira entrega:** um carregador e uma versão estável, com três tipos de
missão. Depois portar para outras versões, sem prometer um JAR universal.

## 6. JOCAS Wallpapers — perfis para jogos

Perfis de FPS, som e uso por monitor. Botão “Vou jogar” para suspender a animação
e restaurá-la depois. A pausa automática só entra quando houver um mecanismo
confiável no compositor usado. Perfis de horário podem vir depois.

**Primeira entrega:** botão e perfis manuais no KDE; suporte a outros desktops
precisa de implementação e validação próprias.

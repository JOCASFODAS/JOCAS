# Compatibilidade e validação

## Estratégia

Não há referências a classes `net.minecraft`, mixins, texturas nem telas internas. As três variantes usam Java 17 como mínimo e leem os metadados do carregador. O visualizador roda em outro processo para não depender do modo gráfico/headless do Minecraft.

| Variante | Faixa declarada | API usada para compilar |
|---|---|---|
| Fabric | Minecraft 1.20.1 em diante | Fabric Loader 0.16.14; mínimo declarado 0.14.21 |
| Forge | Minecraft 1.20.1 em diante | JavaFML 47.4.0 |
| NeoForge | Minecraft 1.20.2 em diante | FancyModLoader 2.0.17; aceita descritores antigos e novos |

NeoForge 1.20.1 compartilha o namespace antigo do Forge e deve usar a variante Forge. Quilt e Bedrock não estão incluídos.

As faixas abertas permitem tentar versões novas; não são uma promessa de compatibilidade futura. Mudanças nas APIs dos carregadores podem exigir atualização. A numeração 26.x, por si só, não requer alterar o relatório.

## Verificações automatizadas

O build executa testes de exportação e leitura, caracteres acentuados, lista com 701 entradas, remoção de caminhos completos, metadados Fabric de mods internos e leitura das duas formas de dependências Forge/NeoForge.

Validação realizada em 22/10/2026, no Linux:

| Ambiente | Resultado |
|---|---|
| Fabric 0.16.14 + Minecraft 1.20.1 | O mod foi inicializado e exportou os cinco componentes reconhecidos pelo carregador, incluindo suas dependências |
| Fabric 0.20.1 + Minecraft 26.1.2 | O mesmo JAR foi inicializado e exportou o relatório |
| Forge 47.4.0 + Minecraft 1.20.1 | O JAR passou pela descoberta do carregador; leitura de metadados e assinaturas verificadas contra a API real |
| NeoForge 26.1.2.109 | O JAR passou pela descoberta e foi incluído no carregador de classes; leitura de metadados e assinaturas verificadas contra a API real |
| FancyModLoader 2.0.17, 4.0.43 e 11.0.15 | Contrato de metadados e assinaturas de inicialização conferidos com bibliotecas oficiais reais e dados simulados |
| Janela em tela virtual isolada | Busca, busca com caractere especial, botão Copiar e cópia integral mesmo com filtro ativo passaram |
| Java 17, 21 e 25 | Bytecode Java 17; APIs Forge testadas no Java 17; Fabric executado nos Java 21 e 25 |

Os testes de servidor pararam na verificação inicial da EULA, sem aceitar termos, iniciar mundos ou abrir partidas. Fabric executa este ponto de extensão antes dessa verificação; Forge/NeoForge completam a inicialização de mods depois dela. Portanto, a exportação durante a inicialização completa de Forge/NeoForge ainda precisa de teste real.

## Limite de validação

A execução de uma partida em cada versão do Minecraft não foi validada. Não foi executado no Windows, em Quilt, em snapshots ou em todas as versões intermediárias. Os testes de adaptadores não substituem uma inicialização real do jogo. O programa não deve ser anunciado como “testado em todas as versões”.

## Fontes técnicas

- Fabric Loader API: https://github.com/FabricMC/fabric-loader/tree/master/src/main/java/net/fabricmc/loader/api
- Arquivos de mods NeoForge: https://docs.neoforged.net/docs/gettingstarted/modfiles/
- Separação cliente/servidor no Forge: https://docs.minecraftforge.net/en/1.20.x/concepts/sides/
- Mudanças da versão 26.1: https://fabricmc.net/2026/03/14/261.html

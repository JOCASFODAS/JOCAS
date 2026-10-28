<div align="center">
  <img src="Apps%20do%20JOCAS/logo-apps-do-jocas.png" width="180" alt="Apps do JOCAS">
  <h1>Apps e Mods do JOCAS</h1>
  <p>Voz, Minecraft e wallpapers — reunidos em um só lugar.</p>
</div>

## Baixar

Cada produto tem sua própria versão. Escolha **Instalação completa** na primeira
vez ou **Atualização** se já utiliza o produto. Extraia todo o ZIP e leia o guia.

| Produto | Versão | Downloads | Compatibilidade real |
| --- | --- | --- | --- |
| **Modificador de Voz JOCAS** | 1.4.1 | [Instalação](https://github.com/JOCASFODAS/JOCAS/releases/download/voz-v1.4.1/Modificador-de-Voz-JOCAS-1.4.1-Instalacao.zip) · [Atualização](https://github.com/JOCASFODAS/JOCAS/releases/download/voz-v1.4.1/Modificador-de-Voz-JOCAS-1.4.1-Atualizacao.zip) · [Detalhes](https://github.com/JOCASFODAS/JOCAS/releases/tag/voz-v1.4.1) | Linux x64; Windows 11 x64 compilado, ainda sem teste de hardware. |
| **JOCAS Lista de Mods** | 1.0.0 | [Instalação](https://github.com/JOCASFODAS/JOCAS/releases/download/modlist-v1.0.0/JOCAS-Lista-de-Mods-1.0.0-Instalacao.zip) · [Atualização](https://github.com/JOCASFODAS/JOCAS/releases/download/modlist-v1.0.0/JOCAS-Lista-de-Mods-1.0.0-Atualizacao.zip) · [Detalhes](https://github.com/JOCASFODAS/JOCAS/releases/tag/modlist-v1.0.0) | Minecraft Java, com variantes Fabric, Forge e NeoForge. Veja as versões testadas no guia. |
| **JOCAS Wallpapers** | 1.0.0 | [Instalação](https://github.com/JOCASFODAS/JOCAS/releases/download/wallpapers-v1.0.0/JOCAS-Wallpapers-1.0.0-CachyOS-KDE-Wayland-x64-Instalacao.zip) · [Atualização](https://github.com/JOCASFODAS/JOCAS/releases/download/wallpapers-v1.0.0/JOCAS-Wallpapers-1.0.0-CachyOS-KDE-Wayland-x64-Atualizacao.zip) · [Detalhes](https://github.com/JOCASFODAS/JOCAS/releases/tag/wallpapers-v1.0.0) | CachyOS x64, KDE Plasma e Wayland. Não é uma versão universal para todas as distros/desktops. |

Os pacotes de atualização contêm arquivos completos de substituição; não são
patches incrementais. Faça backup da instalação e das configurações antes de
substituir arquivos. Os ZIPs de fontes também ficam na release de cada produto.
[Confira os hashes SHA-256](distribuicao/SHA256SUMS.txt).

## O que cada projeto faz

**Modificador de Voz JOCAS** — efeitos de voz, voz personalizada e microfone
virtual para Discord e jogos. Permite ligar/desligar a escuta. Tem modos de IA
local (Piper + Faster Whisper) e API OpenAI, com atraso por frase. Rio não está
na lista de vozes públicas de fala consultada para esta versão. No Linux,
selecione **JOCAS Microfone** no app de chamada; no Windows é necessário VB-CABLE.
[Guia e fontes](Modificador%20de%20Voz%20JOCAS/outputs/Gepetas/LEIA-ME.md).

**JOCAS Lista de Mods** — janela auxiliar para listar mods, versões e dependências,
copiar um relatório e exportar texto. Não resolve incompatibilidades sozinho.
[Guia](JOCAS%20Lista%20de%20Mods/LEIA-ME.md) ·
[Compatibilidade](JOCAS%20Lista%20de%20Mods/COMPATIBILIDADE.md).

**JOCAS Wallpapers** — painel para aplicar wallpapers da sua biblioteca do
Wallpaper Engine pelo motor Linux, mantendo os ícones do KDE visíveis. Requer
Wallpaper Engine adquirido/instalado pela Steam. Wallpapers comerciais e dados
da Steam não são redistribuídos.
[Guia](JOCAS%20Wallpapers/LEIA-ME.md).

## Estado e limites

- Os testes e limites de cada versão estão em seus guias e arquivos `VALIDACAO.json`.
- No modificador, **Natural** transmite a voz original. Perfis personalizados com
  tom zero podem conservar sua voz; a seleção do microfone físico no Discord
  também transmite a entrada original. A centralização no GitHub não altera isso.
  O diagnóstico de entrada e a proteção dos perfis estão no [plano de ideias](IDEIAS.md).
- A edição Windows do modificador foi compilada; chamada real no Discord,
  execução da IA no Windows e hardware VB-CABLE ainda precisam de validação.
- O mod declara faixas abertas de versões, mas não promete funcionamento em todos
  os lançamentos intermediários ou futuros. A faixa 26+ não é uma garantia.
- GNOME, Hyprland e outras distribuições não foram validados no JOCAS Wallpapers.

## Ideias para as próximas criações

Veja [IDEIAS.md](IDEIAS.md). Prioridades sugeridas:

1. **JOCAS Central** — reunir downloads, versões, backups e atualizações dos apps.
2. **Modificador: conferir a voz enviada** — mostrar a entrada do Discord e evitar
   perfis modificados que mantenham o tom original por engano.
3. **JOCAS Doutor do Modpack** — combinar mods + logs em um relatório revisável
   antes de copiar para o GPT, sem modificar o modpack automaticamente.

São propostas; não são recursos já implementados.

## Código e contribuição

As fontes ficam nas pastas dos produtos. O motor de wallpapers é um submódulo
fixado em um commit, preservando a autoria e as licenças originais. Para obter
as fontes com ele:

```sh
git clone --recurse-submodules https://github.com/JOCASFODAS/JOCAS.git
```

[Compilar e organizar](docs/DESENVOLVIMENTO.md) ·
[Publicar versões](docs/PUBLICACAO.md) ·
[Licenças e créditos](docs/LICENCAS.md) ·
[Relatar um problema](https://github.com/JOCASFODAS/JOCAS/issues/new/choose).

Esta publicação reúne as versões existentes. Não instala atualizações na sua
máquina e não inclui gravações, modelos de IA do usuário, contas, chaves,
configurações pessoais ou instaladores de programas de terceiros.

# JOCAS Wallpapers 1.0.0 — CachyOS / KDE Wayland / x64

Integra sua biblioteca Steam Workshop do Wallpaper Engine com o KDE. O aplicativo original, via Proton, continua servindo para navegar e baixar wallpapers; o painel JOCAS usa um motor Linux nativo para aplicá-los.

## Usar

1. Abra **JOCAS Wallpapers** no menu de aplicativos do KDE.
2. Use **Abrir Wallpaper Engine** ou **Workshop da Steam** para baixar wallpapers da sua conta Steam.
3. Volte ao painel, clique em **Atualizar lista**, escolha um wallpaper e um monitor e clique em **Aplicar wallpaper**.
4. **Sem som** silencia o wallpaper. O limite de FPS varia de 10 a 60 e é aplicado ao clicar em Aplicar.
5. **Parar animação** exibe o fundo estático salvo. **Restaurar fundo anterior** também devolve o tipo de wallpaper original do KDE.

A janela pode ser fechada: o painel permanece no ícone da bandeja. Use o menu desse ícone para abrir novamente ou sair. A opção **Iniciar com o CachyOS** restaura os wallpapers selecionados ao entrar na sessão. A seleção parada manualmente não volta no próximo login.

O acompanhamento da seleção do Wallpaper Engine lê `config.json` a cada quatro segundos. Quando o programa Windows salva uma seleção com caminho reconhecível, o JOCAS tenta aplicá-la ao monitor correspondente. Se o Proton não salvar esse campo ou a ordem de monitores diferir, escolha o wallpaper e monitor no painel JOCAS. O painel não modifica o config.json da Steam.

Os wallpapers não são copiados para o aplicativo. A Steam continua responsável pelos downloads e atualizações. Dois itens da biblioteca encontrada durante o desenvolvimento não tinham metadados/arquivos suficientes para reprodução; aparecem como incompletos ou não suportados. Use Atualizar lista após concluir downloads.

## Instalar

Extraia todo o pacote. Na pasta extraída, execute `bash Instalar.sh`. Não use sudo. A integração e o painel são instalados somente no seu usuário.

Na primeira instalação é necessário sair e entrar na sessão KDE, ou reiniciar apenas o Plasma: `systemctl --user restart plasma-plasmashell.service`. Isso cria o fundo transparente que deixa a animação aparecer atrás dos ícones. Depois abra o painel pelo menu.

## Atualizar

Saia do aplicativo pelo menu da bandeja, extraia todo o pacote de atualização e execute `bash Atualizar.sh`. Ele contém os arquivos completos de substituição, e não um patch incremental. A versão anterior é guardada automaticamente, e suas configurações permanecem em `~/.config/JOCAS/Wallpapers.conf`.

Como esta é a primeira versão, o pacote de atualização serve para reaplicar a 1.0.0. Após uma atualização do componente KDE, saia e entre na sessão para recarregar seus arquivos.

## Requisitos e limites

- CachyOS x64, KDE Plasma/KWin 6.5 ou mais recente, sessão Wayland, driver gráfico com OpenGL 3.3, e XWayland para prévias.
- Wallpaper Engine adquirido e instalado pela Steam, incluindo sua pasta `assets`; conteúdo baixado pela própria Steam.
- Bibliotecas nativas do CachyOS: Qt6 Widgets, FFmpeg, mpv, GLEW, SDL2, GLFW, GLUT, DBus, FreeType, LZ4, PulseAudio e Wayland. Os arquivos pesados do motor/CEF e kissfft estão incluídos; bibliotecas do sistema não são empacotadas. Não atualize parcialmente bibliotecas do Arch/CachyOS.
- Este binário foi compilado no CachyOS com Qt 6.12, Plasma 6.8.1 e as bibliotecas presentes em 22/10/2026. Versões antigas da distribuição ou futuras mudanças de FFmpeg/Qt podem exigir recompilação.
- Vídeos, cenas e wallpapers web são encaminhados ao motor Linux. Isso não garante compatibilidade com todos os efeitos, scripts ou wallpapers do programa Windows. Não executa wallpapers do tipo aplicativo `.exe`.
- O encaminhamento experimental dos cliques para os efeitos foi deixado desligado. Os cliques continuam com a área de trabalho do KDE.
- A detecção automática de jogos em tela cheia pelo motor não foi disponibilizada pelo compositor nesta sessão. Use Parar animação quando quiser suspender a reprodução; não é prometida pausa automática durante jogos.
- O motor pode acessar áudio para efeitos reativos; **Sem som** controla a reprodução do wallpaper.

## Backups, diagnóstico e remoção

Instalação: `~/.local/share/jocas-wallpapers`. Atalho: `~/.local/share/applications/jocas-wallpapers.desktop`.

Logs: `~/.local/state/jocas-wallpapers/<monitor>.log`. Backups: `~/.local/state/jocas-wallpapers/backups`. Antes de substituir arquivos manualmente, guarde a pasta do app e `~/.config/JOCAS/Wallpapers.conf` fora da instalação.

Para remover: primeiro clique em **Restaurar fundo anterior**, desmarque **Iniciar com o CachyOS** e saia pelo ícone da bandeja. Depois remova apenas a pasta e o atalho do JOCAS. Os downloads Steam permanecem intactos. A integração KDE pode continuar instalada sem reproduzir nada; seu identificador é `io.github.linuxwallpaperengine.desktop`, e o script KWin se chama `linux-wallpaperengine-desktop`.

## Componentes e fontes

Painel JOCAS: código local em `src/main.cpp`, GPL-3.0-or-later. O motor e a integração KDE vêm do projeto [linux-wallpaperengine-kde-desktop](https://github.com/ZenoMetal/linux-wallpaperengine-kde-desktop), commit `2f41e329d021c1c4c5f765df1a7af8423ffb0ba0`, baseado em [Almamu/linux-wallpaperengine](https://github.com/Almamu/linux-wallpaperengine). O motor e a integração não foram criados do zero para esta entrega.

As fontes correspondentes são entregues separadamente no arquivo `JOCAS-Wallpapers-1.0.0-Fontes.zip`. O CEF conserva sua licença em `payload/LICENSE-CEF.txt`. Os assets e os wallpapers comerciais da Steam não são redistribuídos.

## Validação nesta máquina

Em 22/10/2026: compilação com Wayland habilitado, 15 itens Steam encontrados (13 com arquivos prontos), reprodução da cena “rdr2” (3705914362) no HDMI-A-1 a 30 FPS configurados e sem som, estado de integração ativo no KDE, testes de empilhamento/retorno do fundo após falha e testes QML de preservação dos cliques passaram. O usuário confirmou que o wallpaper está animado e os ícones continuam visíveis. O segundo monitor manteve seu fundo estático. Não foram validados todos os wallpapers, outros sistemas ou um novo login completo.

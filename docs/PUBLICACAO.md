# Publicação de versões

Os executáveis e pacotes ficam em Releases. O Git guarda fontes, guias e
metadados. Tags são independentes por produto:

- `voz-v1.4.1`: Modificador de Voz JOCAS.
- `modlist-v1.0.0`: JOCAS Lista de Mods.
- `wallpapers-v1.0.0`: JOCAS Wallpapers.

Em cada versão, entregar **Atualização**, **Instalação completa** e **Fontes**.
Os dois primeiros têm a mesma versão; explicar sistema/carregador, dependências,
backup e limites reais. Os pacotes atuais de atualização têm arquivos completos,
não patches incrementais. O pacote Fontes preserva licenças e dependências.

No computador de desenvolvimento, a pasta central é `Downloads e Atualizações`.
Use `Ferramentas/Entregas/catalogo.py`: ele valida os três ZIPs, copia a nova
versão e só então remove pacotes antigos daquele produto. Outros produtos não
são removidos. Os scripts de empacotamento pressupõem os binários compilados e
as dependências preparadas nas pastas originais; não são instaladores universais.

Ao criar uma release, publique somente os três pacotes auditados e os hashes.
O README deve apontar para releases existentes; não anuncie builds sem assets.
Não atualize nem remova instalações locais por alguém pedir apenas os arquivos.
No GitHub, versões históricas documentam os lançamentos; não apague releases
antigas sem um pedido explícito, mesmo que a pasta local guarde só a versão atual.

## Antes de publicar

1. Compile e rode verificações adequadas ao produto.
2. Confira a correspondência entre versão, binários, fontes e notas de validação.
3. Valide a integridade de instalação/atualização/fontes e gere SHA-256.
4. Revise os ZIPs para excluir dados do usuário e conteúdo comercial de terceiros.
5. Publique a release, teste os links e atualize `PROJETOS.json` e o README.

[Documentação oficial de Releases](https://docs.github.com/en/repositories/releasing-projects-on-github/about-releases).

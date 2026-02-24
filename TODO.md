# Project TODO

## Prioridade Atual: Configuração In-Game (Fim da dependência do .ini)

- [ ] **Interface de Configuração VR ("Virtual MCM")**:
    - Criar um menu dedicado dentro do ImmersiveUI para editar todas as configurações.
    - O usuário não deve precisar abrir o `ImmersiveUI.ini` manualmente.
    - Implementar botões, seletores e sliders para cada parâmetro (escala, posição, botões, etc.).
    - Botão "Salvar e Recarregar" para aplicar mudanças instantaneamente.

- [ ] **Containers de Inventário Dinâmicos**:
    - Implementar uma classe `VRUIInventoryContainer` que herda de `VRUIContainer`.
    - Buscar itens reais do inventário do jogador usando a API do SKSE (`RE::PlayerCharacter::GetSingleton()->GetInventory()`).
    - Popular o container dinamicamente com `VRUIButton` para cada item.
    - Suporte a filtros (Poções, Magias, Armas, etc.).

- [ ] **Container de Configurações em Tempo Real**:
    - Criar interface para ajustar valores do `VRUISettings` (fMenuScale, fButtonSpacing, etc.).
    - Implementar botões de incremento/decremento (+/-) ou sliders simples.
    - Persistência automática no `ImmersiveUI.ini` ao alterar valores.
    - Atualização visual imediata da UI ao modificar escala ou espaçamento.

- [ ] **Menu de Espera (Wait Menu) Customizado**:
    - Criar um container com controles de seleção de horas (sliders ou botões de +/-).
    - Implementar a lógica de avançar o tempo no jogo sem abrir o menu nativo.
    - Mostrar progresso de espera diretamente na UI do mod.

- [ ] **Suporte a Ações de Outros Mods**:
    - Implementar prefixo `Script:` no INI para chamar funções Papyrus diretamente.
    - Suporte a `SetGlobal:` para alterar variáveis globais de outros mods.
    - Presets para mods como *BeSeated* (integrar o sentar/levantar) e *Animated Wings* (abrir/fechar asas).

- [ ] **Sistema de Paginação**:
    - Adicionar suporte a páginas no `VRUIContainer`.
    - Botões "Próximo" e "Anterior" para navegar entre itens quando a lista for longa.
    - Limpeza e repopulação dinâmica do grid de botões.

## Melhorias Técnicas

- [ ] Refinar o laser pointer (estabilidade e escala).
- [x] Otimizar o carregamento de texturas para ícones.
- [ ] Implementar suporte a `BSEffectShaderProperty` para transparência e texturas customizadas.
- [ ] Auto-criação de `NiAlphaProperty` para NIFs que não possuem configuração de transparência.

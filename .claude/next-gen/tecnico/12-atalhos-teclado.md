# Atalhos de Teclado

> Status: **Aprovado**
> Última atualização: 2025-12-28

---

## Visão Geral

Este documento define os atalhos de teclado para o ERP Staccato web, mapeando funcionalidades do sistema desktop e adaptando para o contexto de navegador.

### Princípios

1. **Consistência** - Mesmos atalhos para mesmas ações em todo o sistema
2. **Familiaridade** - Seguir convenções padrão (Ctrl+S = Salvar)
3. **Não conflitar** - Evitar conflitos com atalhos do navegador
4. **Descobribilidade** - Atalhos visíveis em tooltips e menus

### Conflitos com Navegador

| Atalho | Navegador | Solução |
|--------|-----------|---------|
| Ctrl+S | Salvar página | Capturar e prevenir default |
| Ctrl+P | Imprimir | Capturar para impressão customizada |
| Ctrl+F | Buscar na página | Usar para busca do sistema |
| Ctrl+N | Nova janela | Usar Alt+N ou capturar |
| Ctrl+W | Fechar aba | Não usar |
| Ctrl+T | Nova aba | Não usar |
| F5 | Recarregar | Manter comportamento padrão |

---

## Mapeamento Desktop → Web

### Atalhos Globais

| Ação | Desktop (C++) | Web | Observação |
|------|---------------|-----|------------|
| Fechar/Sair | Ctrl+Q | Ctrl+Q | Logout ou fechar modal |
| Novo Orçamento | Ctrl+N | Alt+N | Evita conflito com browser |
| Salvar | Ctrl+S | Ctrl+S | Previne default do browser |
| Buscar | - | Ctrl+K | Busca global (command palette) |
| Ajuda/Atalhos | - | ? | Mostrar lista de atalhos |
| Navegar abas | - | Ctrl+1-9 | Ir para aba N |

### Atalhos em Formulários

| Ação | Desktop | Web | Contexto |
|------|---------|-----|----------|
| Salvar | Ctrl+S | Ctrl+S | Qualquer formulário |
| Cancelar | Esc | Esc | Fechar modal/formulário |
| Próximo campo | Tab | Tab | Navegação padrão |
| Campo anterior | Shift+Tab | Shift+Tab | Navegação padrão |
| Confirmar | Enter | Enter | Em campos simples |
| Abrir seletor | Space/Enter | Space/Enter | Em dropdowns |

### Atalhos em Tabelas

| Ação | Desktop | Web | Observação |
|------|---------|-----|------------|
| Copiar seleção | Ctrl+C | Ctrl+C | Copia células selecionadas |
| Abrir registro | Duplo clique | Enter | Na linha selecionada |
| Editar registro | Duplo clique | E | Na linha selecionada |
| Excluir registro | - | Delete | Com confirmação |
| Selecionar todas | Ctrl+A | Ctrl+A | Seleciona todas as linhas |
| Navegar | Setas | Setas | Entre linhas/colunas |
| Primeira linha | Home | Home | Vai para o início |
| Última linha | End | End | Vai para o fim |
| Página anterior | Page Up | Page Up | Paginação |
| Próxima página | Page Down | Page Down | Paginação |

---

## Implementação Vue

### Composable de Atalhos

```typescript
// composables/useKeyboardShortcuts.ts
import { onMounted, onUnmounted } from 'vue';

interface Shortcut {
  key: string;
  ctrl?: boolean;
  alt?: boolean;
  shift?: boolean;
  handler: () => void;
  description?: string;
  preventDefault?: boolean;
}

export function useKeyboardShortcuts(shortcuts: Shortcut[]) {
  const handleKeydown = (event: KeyboardEvent) => {
    for (const shortcut of shortcuts) {
      const keyMatch = event.key.toLowerCase() === shortcut.key.toLowerCase();
      const ctrlMatch = !!shortcut.ctrl === (event.ctrlKey || event.metaKey);
      const altMatch = !!shortcut.alt === event.altKey;
      const shiftMatch = !!shortcut.shift === event.shiftKey;

      if (keyMatch && ctrlMatch && altMatch && shiftMatch) {
        if (shortcut.preventDefault !== false) {
          event.preventDefault();
        }
        shortcut.handler();
        return;
      }
    }
  };

  onMounted(() => {
    document.addEventListener('keydown', handleKeydown);
  });

  onUnmounted(() => {
    document.removeEventListener('keydown', handleKeydown);
  });

  return { shortcuts };
}
```

### Uso em Componente

```vue
<script setup>
import { useKeyboardShortcuts } from '@/composables/useKeyboardShortcuts';
import { router } from '@inertiajs/vue3';

const form = ref({ /* ... */ });

useKeyboardShortcuts([
  {
    key: 's',
    ctrl: true,
    handler: () => salvar(),
    description: 'Salvar',
  },
  {
    key: 'Escape',
    handler: () => cancelar(),
    description: 'Cancelar',
  },
  {
    key: 'n',
    alt: true,
    handler: () => router.visit('/orcamentos/novo'),
    description: 'Novo Orçamento',
  },
]);

const salvar = () => {
  form.value.post('/api/v1/orcamentos');
};

const cancelar = () => {
  router.visit('/orcamentos');
};
</script>
```

### Provider Global de Atalhos

```vue
<!-- Components/KeyboardShortcutsProvider.vue -->
<script setup>
import { provide, ref, onMounted, onUnmounted } from 'vue';
import { router } from '@inertiajs/vue3';

const showHelp = ref(false);
const registeredShortcuts = ref<Map<string, Shortcut>>(new Map());

// Atalhos globais
const globalShortcuts = [
  {
    key: '?',
    handler: () => showHelp.value = true,
    description: 'Mostrar atalhos',
    global: true,
  },
  {
    key: 'k',
    ctrl: true,
    handler: () => openCommandPalette(),
    description: 'Busca global',
    global: true,
  },
  {
    key: 'Escape',
    handler: () => {
      showHelp.value = false;
      closeCommandPalette();
    },
    description: 'Fechar',
    global: true,
  },
];

// Atalhos de navegação
for (let i = 1; i <= 9; i++) {
  globalShortcuts.push({
    key: String(i),
    ctrl: true,
    handler: () => navigateToTab(i),
    description: `Ir para aba ${i}`,
    global: true,
  });
}

const handleKeydown = (event: KeyboardEvent) => {
  // Ignorar se estiver em input (exceto Escape e Ctrl+S)
  const target = event.target as HTMLElement;
  const isInput = ['INPUT', 'TEXTAREA', 'SELECT'].includes(target.tagName);
  const isContentEditable = target.isContentEditable;

  if (isInput || isContentEditable) {
    // Permitir apenas alguns atalhos em inputs
    const allowedInInput = ['Escape', 's'];
    if (!allowedInInput.includes(event.key.toLowerCase()) || !event.ctrlKey) {
      if (event.key !== 'Escape') return;
    }
  }

  // Verificar atalhos registrados
  for (const [key, shortcut] of registeredShortcuts.value) {
    if (matchesShortcut(event, shortcut)) {
      event.preventDefault();
      shortcut.handler();
      return;
    }
  }
};

provide('registerShortcut', (id: string, shortcut: Shortcut) => {
  registeredShortcuts.value.set(id, shortcut);
});

provide('unregisterShortcut', (id: string) => {
  registeredShortcuts.value.delete(id);
});

provide('showShortcutsHelp', () => showHelp.value = true);

onMounted(() => {
  document.addEventListener('keydown', handleKeydown);
});

onUnmounted(() => {
  document.removeEventListener('keydown', handleKeydown);
});
</script>

<template>
  <slot />

  <!-- Modal de Ajuda de Atalhos -->
  <Modal v-model="showHelp" title="Atalhos de Teclado">
    <div class="space-y-4">
      <div v-for="category in shortcutCategories" :key="category.name">
        <h3 class="font-semibold text-gray-900 mb-2">{{ category.name }}</h3>
        <div class="grid grid-cols-2 gap-2">
          <div
            v-for="shortcut in category.shortcuts"
            :key="shortcut.key"
            class="flex items-center justify-between py-1"
          >
            <span class="text-gray-600">{{ shortcut.description }}</span>
            <kbd class="px-2 py-1 bg-gray-100 rounded text-sm font-mono">
              {{ formatShortcut(shortcut) }}
            </kbd>
          </div>
        </div>
      </div>
    </div>
  </Modal>
</template>
```

---

## Catálogo de Atalhos

### Navegação Global

| Atalho | Ação | Contexto |
|--------|------|----------|
| `?` | Mostrar atalhos | Global |
| `Ctrl+K` | Busca global (Command Palette) | Global |
| `Ctrl+1` | Ir para Orçamentos | Global |
| `Ctrl+2` | Ir para Vendas | Global |
| `Ctrl+3` | Ir para Compras | Global |
| `Ctrl+4` | Ir para Logística | Global |
| `Ctrl+5` | Ir para NFe | Global |
| `Ctrl+6` | Ir para Estoque | Global |
| `Ctrl+7` | Ir para Financeiro | Global |
| `Ctrl+8` | Ir para Relatórios | Global |
| `G` `H` | Ir para Home/Dashboard | Global (sequência) |
| `G` `C` | Ir para Clientes | Global (sequência) |
| `G` `P` | Ir para Produtos | Global (sequência) |

### Ações Comuns

| Atalho | Ação | Contexto |
|--------|------|----------|
| `Ctrl+S` | Salvar | Formulários |
| `Esc` | Cancelar/Fechar | Modais, formulários |
| `Alt+N` | Novo registro | Listagens |
| `E` | Editar selecionado | Listagens |
| `Delete` | Excluir selecionado | Listagens |
| `Ctrl+P` | Imprimir/Exportar PDF | Visualização |
| `Ctrl+E` | Exportar Excel | Listagens |

### Orçamentos

| Atalho | Ação | Contexto |
|--------|------|----------|
| `Alt+N` | Novo orçamento | Lista de orçamentos |
| `Alt+I` | Adicionar item | Edição de orçamento |
| `Alt+C` | Buscar cliente | Edição de orçamento |
| `Alt+F` | Calcular frete | Edição de orçamento |
| `Alt+V` | Converter em venda | Visualização |
| `Ctrl+D` | Duplicar orçamento | Visualização |

### Vendas

| Atalho | Ação | Contexto |
|--------|------|----------|
| `Alt+P` | Adicionar pagamento | Edição de venda |
| `Alt+E` | Agendar entrega | Edição de venda |
| `Alt+N` | Emitir NFe | Visualização |
| `Alt+X` | Cancelar venda | Visualização |

### Estoque

| Atalho | Ação | Contexto |
|--------|------|----------|
| `Alt+R` | Reservar | Seleção de lote |
| `Alt+L` | Liberar reserva | Seleção de lote |
| `Alt+M` | Movimentar | Seleção de lote |

### Tabelas

| Atalho | Ação | Contexto |
|--------|------|----------|
| `↑` `↓` | Navegar linhas | Tabela com foco |
| `←` `→` | Navegar colunas | Tabela com foco |
| `Enter` | Abrir registro | Linha selecionada |
| `Space` | Selecionar/Desselecionar | Linha com checkbox |
| `Ctrl+A` | Selecionar todos | Tabela com foco |
| `Ctrl+C` | Copiar seleção | Células selecionadas |
| `Home` | Primeira linha | Tabela com foco |
| `End` | Última linha | Tabela com foco |
| `Page Up` | Página anterior | Tabela paginada |
| `Page Down` | Próxima página | Tabela paginada |

### Formulários

| Atalho | Ação | Contexto |
|--------|------|----------|
| `Tab` | Próximo campo | Formulário |
| `Shift+Tab` | Campo anterior | Formulário |
| `Enter` | Submeter (em inputs) | Formulário simples |
| `Ctrl+Enter` | Submeter (em textareas) | Formulário |
| `Esc` | Cancelar edição | Formulário |

---

## Command Palette

### Implementação

```vue
<!-- Components/CommandPalette.vue -->
<script setup>
import { ref, computed, watch } from 'vue';
import { router } from '@inertiajs/vue3';
import { useFuse } from '@vueuse/integrations/useFuse';

const props = defineProps<{
  open: boolean;
}>();

const emit = defineEmits(['close']);

const query = ref('');
const selectedIndex = ref(0);

const commands = [
  // Navegação
  { id: 'nav-home', name: 'Ir para Dashboard', icon: 'home', action: () => router.visit('/') },
  { id: 'nav-orcamentos', name: 'Ir para Orçamentos', icon: 'document', action: () => router.visit('/orcamentos') },
  { id: 'nav-vendas', name: 'Ir para Vendas', icon: 'shopping-cart', action: () => router.visit('/vendas') },
  { id: 'nav-clientes', name: 'Ir para Clientes', icon: 'users', action: () => router.visit('/clientes') },
  { id: 'nav-produtos', name: 'Ir para Produtos', icon: 'cube', action: () => router.visit('/produtos') },

  // Ações
  { id: 'action-novo-orcamento', name: 'Criar novo orçamento', icon: 'plus', action: () => router.visit('/orcamentos/novo') },
  { id: 'action-novo-cliente', name: 'Cadastrar cliente', icon: 'user-plus', action: () => router.visit('/clientes/novo') },
  { id: 'action-calcular-frete', name: 'Calcular frete', icon: 'truck', action: () => openFreteModal() },

  // Configurações
  { id: 'config-perfil', name: 'Meu perfil', icon: 'user', action: () => router.visit('/perfil') },
  { id: 'config-preferencias', name: 'Preferências', icon: 'cog', action: () => router.visit('/preferencias') },
  { id: 'config-logout', name: 'Sair', icon: 'logout', action: () => router.post('/logout') },
];

const { results } = useFuse(query, commands, {
  keys: ['name'],
  threshold: 0.3,
});

const filteredCommands = computed(() => {
  if (!query.value) return commands.slice(0, 8);
  return results.value.map(r => r.item).slice(0, 8);
});

watch(() => props.open, (isOpen) => {
  if (isOpen) {
    query.value = '';
    selectedIndex.value = 0;
  }
});

const handleKeydown = (event: KeyboardEvent) => {
  switch (event.key) {
    case 'ArrowDown':
      event.preventDefault();
      selectedIndex.value = Math.min(selectedIndex.value + 1, filteredCommands.value.length - 1);
      break;
    case 'ArrowUp':
      event.preventDefault();
      selectedIndex.value = Math.max(selectedIndex.value - 1, 0);
      break;
    case 'Enter':
      event.preventDefault();
      executeCommand(filteredCommands.value[selectedIndex.value]);
      break;
    case 'Escape':
      emit('close');
      break;
  }
};

const executeCommand = (command) => {
  emit('close');
  command.action();
};
</script>

<template>
  <TransitionRoot :show="open" as="template">
    <Dialog @close="$emit('close')" class="relative z-50">
      <TransitionChild
        enter="ease-out duration-200"
        enter-from="opacity-0"
        enter-to="opacity-100"
        leave="ease-in duration-150"
        leave-from="opacity-100"
        leave-to="opacity-0"
      >
        <div class="fixed inset-0 bg-black/25" />
      </TransitionChild>

      <div class="fixed inset-0 overflow-y-auto pt-[20vh]">
        <div class="flex justify-center px-4">
          <TransitionChild
            enter="ease-out duration-200"
            enter-from="opacity-0 scale-95"
            enter-to="opacity-100 scale-100"
            leave="ease-in duration-150"
            leave-from="opacity-100 scale-100"
            leave-to="opacity-0 scale-95"
          >
            <DialogPanel class="w-full max-w-lg bg-white rounded-xl shadow-2xl overflow-hidden">
              <!-- Input de busca -->
              <div class="flex items-center px-4 border-b">
                <MagnifyingGlassIcon class="h-5 w-5 text-gray-400" />
                <input
                  v-model="query"
                  @keydown="handleKeydown"
                  type="text"
                  class="w-full px-3 py-4 text-gray-900 placeholder-gray-400 focus:outline-none"
                  placeholder="Digite um comando..."
                  autofocus
                />
                <kbd class="px-2 py-1 text-xs bg-gray-100 rounded">Esc</kbd>
              </div>

              <!-- Lista de comandos -->
              <div class="max-h-80 overflow-y-auto py-2">
                <div
                  v-for="(command, index) in filteredCommands"
                  :key="command.id"
                  @click="executeCommand(command)"
                  :class="[
                    'flex items-center px-4 py-2 cursor-pointer',
                    index === selectedIndex ? 'bg-primary-50 text-primary-900' : 'text-gray-700 hover:bg-gray-50'
                  ]"
                >
                  <component :is="getIcon(command.icon)" class="h-5 w-5 mr-3 text-gray-400" />
                  <span>{{ command.name }}</span>
                </div>

                <div v-if="filteredCommands.length === 0" class="px-4 py-8 text-center text-gray-500">
                  Nenhum comando encontrado
                </div>
              </div>

              <!-- Footer -->
              <div class="px-4 py-2 bg-gray-50 border-t text-xs text-gray-500 flex items-center justify-between">
                <span>
                  <kbd class="px-1.5 py-0.5 bg-white rounded border">↑</kbd>
                  <kbd class="px-1.5 py-0.5 bg-white rounded border ml-1">↓</kbd>
                  para navegar
                </span>
                <span>
                  <kbd class="px-1.5 py-0.5 bg-white rounded border">Enter</kbd>
                  para selecionar
                </span>
              </div>
            </DialogPanel>
          </TransitionChild>
        </div>
      </div>
    </Dialog>
  </TransitionRoot>
</template>
```

---

## Acessibilidade

### Focus Management

```vue
<!-- Garantir foco visível -->
<style>
/* Focus ring customizado */
*:focus-visible {
  outline: 2px solid theme('colors.primary.500');
  outline-offset: 2px;
}

/* Skip link */
.skip-link {
  @apply sr-only focus:not-sr-only focus:absolute focus:top-4 focus:left-4 focus:z-50;
  @apply bg-white px-4 py-2 rounded shadow-lg;
}
</style>

<template>
  <a href="#main-content" class="skip-link">
    Pular para conteúdo principal
  </a>

  <nav><!-- ... --></nav>

  <main id="main-content" tabindex="-1">
    <!-- Conteúdo principal -->
  </main>
</template>
```

### Navegação por Tab

```vue
<!-- Ordem de tabulação lógica -->
<form @submit.prevent="submit">
  <div class="grid grid-cols-2 gap-4">
    <!-- Ordem visual: esquerda para direita, cima para baixo -->
    <input tabindex="1" name="nome" />
    <input tabindex="2" name="email" />
    <input tabindex="3" name="telefone" />
    <input tabindex="4" name="cpf" />
  </div>

  <div class="flex justify-end gap-2 mt-4">
    <!-- Cancelar antes de Salvar (menos destrutivo primeiro) -->
    <button type="button" tabindex="5">Cancelar</button>
    <button type="submit" tabindex="6">Salvar</button>
  </div>
</form>
```

### ARIA para Atalhos

```vue
<button
  @click="salvar"
  aria-keyshortcuts="Control+S"
  title="Salvar (Ctrl+S)"
>
  Salvar
</button>

<nav aria-label="Navegação principal">
  <a href="/orcamentos" aria-keyshortcuts="Control+1">Orçamentos</a>
  <a href="/vendas" aria-keyshortcuts="Control+2">Vendas</a>
</nav>
```

---

## Indicadores Visuais

### Tooltips com Atalhos

```vue
<template>
  <button
    v-tooltip="{
      content: 'Salvar <kbd>Ctrl+S</kbd>',
      html: true,
    }"
    @click="salvar"
  >
    <SaveIcon class="h-5 w-5" />
  </button>
</template>
```

### Menu com Atalhos

```vue
<template>
  <Menu>
    <MenuButton>Arquivo</MenuButton>
    <MenuItems>
      <MenuItem v-slot="{ active }">
        <button :class="{ 'bg-primary-50': active }" class="flex justify-between w-full px-4 py-2">
          <span>Novo Orçamento</span>
          <kbd class="text-gray-400">Alt+N</kbd>
        </button>
      </MenuItem>
      <MenuItem v-slot="{ active }">
        <button :class="{ 'bg-primary-50': active }" class="flex justify-between w-full px-4 py-2">
          <span>Salvar</span>
          <kbd class="text-gray-400">Ctrl+S</kbd>
        </button>
      </MenuItem>
    </MenuItems>
  </Menu>
</template>
```

---

## Configuração do Usuário

### Preferências de Atalhos

```typescript
// stores/shortcuts.ts
import { defineStore } from 'pinia';

export const useShortcutsStore = defineStore('shortcuts', {
  state: () => ({
    enabled: true,
    customMappings: {} as Record<string, string>,
  }),

  actions: {
    toggleShortcuts() {
      this.enabled = !this.enabled;
    },

    setCustomMapping(action: string, shortcut: string) {
      this.customMappings[action] = shortcut;
    },

    resetToDefaults() {
      this.customMappings = {};
    },
  },

  persist: true,
});
```

---

## Checklist de Implementação

- [ ] Criar composable `useKeyboardShortcuts`
- [ ] Implementar `KeyboardShortcutsProvider`
- [ ] Criar Command Palette (`Ctrl+K`)
- [ ] Implementar modal de ajuda (`?`)
- [ ] Adicionar atalhos em tooltips
- [ ] Configurar navegação por tabs (Ctrl+1-9)
- [ ] Implementar atalhos de formulário (Ctrl+S, Esc)
- [ ] Adicionar atalhos em tabelas (Enter, Delete)
- [ ] Testar conflitos com navegadores
- [ ] Documentar atalhos para usuários

---

## Documentos Relacionados

- [10-design-system.md](./10-design-system.md) - Componentes de UI
- [../estrategia/11-treinamento.md](../estrategia/11-treinamento.md) - Treinamento

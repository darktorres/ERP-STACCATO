# Design System / Guia de UI

> Status: **Aprovado**
> Última atualização: 2025-12-28

---

## Visão Geral

Este documento define o Design System do ERP Staccato, estabelecendo padrões visuais, componentes e diretrizes de UX para garantir consistência na interface.

### Stack de UI

| Tecnologia   | Uso                       |
| ------------ | ------------------------- |
| Vue 3        | Framework frontend        |
| Tailwind CSS | Utility-first CSS         |
| PrimeVue     | Biblioteca de componentes |
| Heroicons    | Iconografia               |
| Headless UI  | Componentes acessíveis    |

---

## Paleta de Cores

### Cores Primárias

```css
/* Azul Staccato - Cor principal da marca */
--color-primary-50: #eff6ff;
--color-primary-100: #dbeafe;
--color-primary-200: #bfdbfe;
--color-primary-300: #93c5fd;
--color-primary-400: #60a5fa;
--color-primary-500: #3b82f6; /* Principal */
--color-primary-600: #2563eb;
--color-primary-700: #1d4ed8;
--color-primary-800: #1e40af;
--color-primary-900: #1e3a8a;
```

### Cores Secundárias

```css
/* Cinza Neutro */
--color-gray-50: #f9fafb;
--color-gray-100: #f3f4f6;
--color-gray-200: #e5e7eb;
--color-gray-300: #d1d5db;
--color-gray-400: #9ca3af;
--color-gray-500: #6b7280;
--color-gray-600: #4b5563;
--color-gray-700: #374151;
--color-gray-800: #1f2937;
--color-gray-900: #111827;
```

### Cores de Status

```css
/* Sucesso */
--color-success-50: #f0fdf4;
--color-success-500: #22c55e;
--color-success-700: #15803d;

/* Aviso */
--color-warning-50: #fffbeb;
--color-warning-500: #f59e0b;
--color-warning-700: #b45309;

/* Erro */
--color-error-50: #fef2f2;
--color-error-500: #ef4444;
--color-error-700: #b91c1c;

/* Informação */
--color-info-50: #eff6ff;
--color-info-500: #3b82f6;
--color-info-700: #1d4ed8;
```

### Cores de Contexto de Negócio

```css
/* Status de Venda */
--color-venda-pendente: #f59e0b; /* Amarelo */
--color-venda-confirmada: #3b82f6; /* Azul */
--color-venda-entregue: #22c55e; /* Verde */
--color-venda-cancelada: #ef4444; /* Vermelho */

/* Status Financeiro */
--color-pago: #22c55e;
--color-pendente: #f59e0b;
--color-vencido: #ef4444;
--color-parcial: #8b5cf6;

/* NFe Status */
--color-nfe-autorizada: #22c55e;
--color-nfe-pendente: #f59e0b;
--color-nfe-rejeitada: #ef4444;
--color-nfe-cancelada: #6b7280;
```

### Configuração Tailwind

```javascript
// tailwind.config.js
module.exports = {
  theme: {
    extend: {
      colors: {
        primary: {
          50: "#eff6ff",
          100: "#dbeafe",
          200: "#bfdbfe",
          300: "#93c5fd",
          400: "#60a5fa",
          500: "#3b82f6",
          600: "#2563eb",
          700: "#1d4ed8",
          800: "#1e40af",
          900: "#1e3a8a",
        },
        // Cores de status de negócio
        venda: {
          pendente: "#f59e0b",
          confirmada: "#3b82f6",
          entregue: "#22c55e",
          cancelada: "#ef4444",
        },
      },
    },
  },
};
```

---

## Tipografia

### Família de Fontes

```css
/* Fonte principal */
--font-sans:
  "Inter", -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;

/* Fonte mono (para códigos, IDs) */
--font-mono: "JetBrains Mono", "Fira Code", Consolas, monospace;
```

### Escala Tipográfica

| Nome        | Tamanho | Peso | Uso                       |
| ----------- | ------- | ---- | ------------------------- |
| `text-xs`   | 12px    | 400  | Labels, badges            |
| `text-sm`   | 14px    | 400  | Texto secundário, tabelas |
| `text-base` | 16px    | 400  | Texto padrão              |
| `text-lg`   | 18px    | 500  | Subtítulos                |
| `text-xl`   | 20px    | 600  | Títulos de seção          |
| `text-2xl`  | 24px    | 700  | Títulos de página         |
| `text-3xl`  | 30px    | 700  | Títulos principais        |

### Hierarquia de Títulos

```html
<!-- Título de página -->
<h1 class="text-2xl font-bold text-gray-900">Orçamentos</h1>

<!-- Título de seção -->
<h2 class="text-xl font-semibold text-gray-800">Itens do Pedido</h2>

<!-- Subtítulo -->
<h3 class="text-lg font-medium text-gray-700">Endereço de Entrega</h3>

<!-- Label -->
<label class="text-sm font-medium text-gray-600">Cliente</label>
```

---

## Espaçamento

### Sistema de Grid

```css
/* Base: 4px */
--spacing-0: 0;
--spacing-1: 4px;
--spacing-2: 8px;
--spacing-3: 12px;
--spacing-4: 16px;
--spacing-5: 20px;
--spacing-6: 24px;
--spacing-8: 32px;
--spacing-10: 40px;
--spacing-12: 48px;
--spacing-16: 64px;
```

### Padrões de Layout

```html
<!-- Card padrão -->
<div class="bg-white rounded-lg shadow p-6 space-y-4">
  <!-- Conteúdo -->
</div>

<!-- Seção com margem -->
<section class="mb-8">
  <!-- Conteúdo -->
</section>

<!-- Grid de formulário -->
<div class="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
  <!-- Campos -->
</div>
```

---

## Componentes Base

### Botões

```vue
<!-- Botão Primário -->
<button
  class="
  px-4 py-2
  bg-primary-600 hover:bg-primary-700
  text-white font-medium
  rounded-lg
  transition-colors
  focus:outline-none focus:ring-2 focus:ring-primary-500 focus:ring-offset-2
  disabled:opacity-50 disabled:cursor-not-allowed
"
>
  Salvar
</button>

<!-- Botão Secundário -->
<button
  class="
  px-4 py-2
  bg-white hover:bg-gray-50
  text-gray-700 font-medium
  border border-gray-300
  rounded-lg
  transition-colors
  focus:outline-none focus:ring-2 focus:ring-primary-500 focus:ring-offset-2
"
>
  Cancelar
</button>

<!-- Botão de Perigo -->
<button
  class="
  px-4 py-2
  bg-red-600 hover:bg-red-700
  text-white font-medium
  rounded-lg
  transition-colors
  focus:outline-none focus:ring-2 focus:ring-red-500 focus:ring-offset-2
"
>
  Excluir
</button>

<!-- Botão Ghost -->
<button
  class="
  px-4 py-2
  text-primary-600 hover:text-primary-700 hover:bg-primary-50
  font-medium
  rounded-lg
  transition-colors
"
>
  Cancelar
</button>
```

### Variantes de Tamanho

```vue
<!-- Pequeno -->
<button class="px-3 py-1.5 text-sm">Pequeno</button>

<!-- Médio (padrão) -->
<button class="px-4 py-2 text-base">Médio</button>

<!-- Grande -->
<button class="px-6 py-3 text-lg">Grande</button>
```

### Inputs

```vue
<!-- Input de Texto -->
<div class="space-y-1">
  <label class="block text-sm font-medium text-gray-700">
    Nome do Cliente
  </label>
  <input
    type="text"
    class="
      w-full px-3 py-2
      border border-gray-300 rounded-lg
      text-gray-900 placeholder-gray-400
      focus:outline-none focus:ring-2 focus:ring-primary-500 focus:border-primary-500
      disabled:bg-gray-100 disabled:cursor-not-allowed
    "
    placeholder="Digite o nome..."
  />
</div>

<!-- Input com Erro -->
<div class="space-y-1">
  <label class="block text-sm font-medium text-gray-700">CPF</label>
  <input
    type="text"
    class="
      w-full px-3 py-2
      border border-red-500 rounded-lg
      text-gray-900
      focus:outline-none focus:ring-2 focus:ring-red-500 focus:border-red-500
    "
    aria-invalid="true"
  />
  <p class="text-sm text-red-600">CPF inválido</p>
</div>

<!-- Input com Ícone -->
<div class="relative">
  <div class="absolute inset-y-0 left-0 pl-3 flex items-center pointer-events-none">
    <MagnifyingGlassIcon class="h-5 w-5 text-gray-400" />
  </div>
<input
  type="text"
  class="
      w-full pl-10 pr-3 py-2
      border border-gray-300 rounded-lg
      focus:outline-none focus:ring-2 focus:ring-primary-500
    "
  placeholder="Buscar..."
/>
```

### Selects

```vue
<!-- Select Nativo Estilizado -->
<select
  class="
  w-full px-3 py-2
  border border-gray-300 rounded-lg
  text-gray-900 bg-white
  focus:outline-none focus:ring-2 focus:ring-primary-500
"
>
  <option value="">Selecione...</option>
  <option value="1">Opção 1</option>
  <option value="2">Opção 2</option>
</select>

<!-- Com PrimeVue Dropdown -->
<Dropdown
  v-model="selected"
  :options="options"
  optionLabel="name"
  placeholder="Selecione..."
  class="w-full"
/>
```

### Checkboxes e Radios

```vue
<!-- Checkbox -->
<label class="flex items-center space-x-2">
  <input
    type="checkbox"
    class="
      h-4 w-4
      text-primary-600
      border-gray-300 rounded
      focus:ring-primary-500
    "
  />
  <span class="text-sm text-gray-700">Ativo</span>
</label>

<!-- Radio -->
<label class="flex items-center space-x-2">
  <input
    type="radio"
    name="tipo"
    class="
      h-4 w-4
      text-primary-600
      border-gray-300
      focus:ring-primary-500
    "
  />
  <span class="text-sm text-gray-700">Pessoa Física</span>
</label>
```

---

## Componentes Complexos

### Cards

```vue
<!-- Card Básico -->
<div class="bg-white rounded-lg shadow-sm border border-gray-200 p-6">
  <h3 class="text-lg font-semibold text-gray-900 mb-4">Título</h3>
  <p class="text-gray-600">Conteúdo do card</p>
</div>

<!-- Card com Header -->
<div class="bg-white rounded-lg shadow-sm border border-gray-200 overflow-hidden">
  <div class="px-6 py-4 bg-gray-50 border-b border-gray-200">
    <h3 class="text-lg font-semibold text-gray-900">Dados do Cliente</h3>
  </div>
  <div class="p-6">
    <!-- Conteúdo -->
  </div>
</div>

<!-- Card de Status -->
<div class="bg-white rounded-lg shadow-sm border-l-4 border-l-green-500 p-4">
  <div class="flex items-center">
    <CheckCircleIcon class="h-5 w-5 text-green-500 mr-2" />
    <span class="text-sm font-medium text-green-700">Venda confirmada</span>
  </div>
</div>
```

### Tabelas

```vue
<div class="overflow-x-auto">
  <table class="min-w-full divide-y divide-gray-200">
    <thead class="bg-gray-50">
      <tr>
        <th class="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">
          Cliente
        </th>
        <th class="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">
          Valor
        </th>
        <th class="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">
          Status
        </th>
        <th class="px-6 py-3 text-right text-xs font-medium text-gray-500 uppercase tracking-wider">
          Ações
        </th>
      </tr>
    </thead>
    <tbody class="bg-white divide-y divide-gray-200">
      <tr class="hover:bg-gray-50">
        <td class="px-6 py-4 whitespace-nowrap">
          <div class="text-sm font-medium text-gray-900">João Silva</div>
          <div class="text-sm text-gray-500">joao@email.com</div>
        </td>
        <td class="px-6 py-4 whitespace-nowrap text-sm text-gray-900">
          R$ 1.500,00
        </td>
        <td class="px-6 py-4 whitespace-nowrap">
          <StatusBadge status="confirmada" />
        </td>
        <td class="px-6 py-4 whitespace-nowrap text-right text-sm">
          <button class="text-primary-600 hover:text-primary-900">
            Editar
          </button>
        </td>
      </tr>
    </tbody>
  </table>
</div>
```

### Modais

```vue
<template>
  <TransitionRoot appear :show="isOpen" as="template">
    <Dialog as="div" class="relative z-50" @close="close">
      <!-- Overlay -->
      <TransitionChild
        enter="ease-out duration-300"
        enter-from="opacity-0"
        enter-to="opacity-100"
        leave="ease-in duration-200"
        leave-from="opacity-100"
        leave-to="opacity-0"
      >
        <div class="fixed inset-0 bg-black bg-opacity-25" />
      </TransitionChild>

      <!-- Modal -->
      <div class="fixed inset-0 overflow-y-auto">
        <div class="flex min-h-full items-center justify-center p-4">
          <TransitionChild
            enter="ease-out duration-300"
            enter-from="opacity-0 scale-95"
            enter-to="opacity-100 scale-100"
            leave="ease-in duration-200"
            leave-from="opacity-100 scale-100"
            leave-to="opacity-0 scale-95"
          >
            <DialogPanel
              class="
              w-full max-w-md
              bg-white rounded-xl shadow-xl
              transform transition-all
            "
            >
              <!-- Header -->
              <div class="px-6 py-4 border-b border-gray-200">
                <DialogTitle class="text-lg font-semibold text-gray-900">
                  Confirmar Ação
                </DialogTitle>
              </div>

              <!-- Content -->
              <div class="px-6 py-4">
                <p class="text-gray-600">Tem certeza que deseja continuar?</p>
              </div>

              <!-- Footer -->
              <div class="px-6 py-4 bg-gray-50 flex justify-end space-x-3">
                <button @click="close" class="btn-secondary">Cancelar</button>
                <button @click="confirm" class="btn-primary">Confirmar</button>
              </div>
            </DialogPanel>
          </TransitionChild>
        </div>
      </div>
    </Dialog>
  </TransitionRoot>
</template>
```

### Toasts/Notificações

```vue
<!-- Toast de Sucesso -->
<div class="
  fixed bottom-4 right-4
  flex items-center
  bg-white rounded-lg shadow-lg
  border-l-4 border-l-green-500
  p-4
  max-w-sm
">
  <CheckCircleIcon class="h-6 w-6 text-green-500 mr-3" />
  <div>
    <p class="text-sm font-medium text-gray-900">Salvo com sucesso!</p>
    <p class="text-sm text-gray-500">Os dados foram atualizados.</p>
  </div>
  <button class="ml-4 text-gray-400 hover:text-gray-600">
    <XMarkIcon class="h-5 w-5" />
  </button>
</div>

<!-- Toast de Erro -->
<div class="
  fixed bottom-4 right-4
  flex items-center
  bg-white rounded-lg shadow-lg
  border-l-4 border-l-red-500
  p-4
  max-w-sm
">
  <ExclamationCircleIcon class="h-6 w-6 text-red-500 mr-3" />
  <div>
    <p class="text-sm font-medium text-gray-900">Erro ao salvar</p>
    <p class="text-sm text-gray-500">Verifique os campos obrigatórios.</p>
  </div>
</div>
```

### Badges de Status

```vue
<script setup>
const statusConfig = {
  pendente: { bg: "bg-yellow-100", text: "text-yellow-800", label: "Pendente" },
  confirmada: { bg: "bg-blue-100", text: "text-blue-800", label: "Confirmada" },
  entregue: { bg: "bg-green-100", text: "text-green-800", label: "Entregue" },
  cancelada: { bg: "bg-red-100", text: "text-red-800", label: "Cancelada" },
};
</script>

<template>
  <span
    :class="[
      'inline-flex items-center px-2.5 py-0.5 rounded-full text-xs font-medium',
      statusConfig[status].bg,
      statusConfig[status].text,
    ]"
  >
    {{ statusConfig[status].label }}
  </span>
</template>
```

---

## Estados de Interação

### Estados de Botão

```css
/* Default */
.btn {
  @apply transition-colors;
}

/* Hover */
.btn:hover {
  @apply bg-primary-700;
}

/* Focus */
.btn:focus {
  @apply outline-none ring-2 ring-primary-500 ring-offset-2;
}

/* Active */
.btn:active {
  @apply bg-primary-800;
}

/* Disabled */
.btn:disabled {
  @apply opacity-50 cursor-not-allowed;
}

/* Loading */
.btn-loading {
  @apply relative pointer-events-none;
}
.btn-loading::after {
  content: "";
  @apply absolute inset-0 flex items-center justify-center;
  /* Spinner animation */
}
```

### Estados de Input

```css
/* Default */
.input {
  @apply border-gray-300;
}

/* Focus */
.input:focus {
  @apply border-primary-500 ring-2 ring-primary-500;
}

/* Error */
.input-error {
  @apply border-red-500 focus:border-red-500 focus:ring-red-500;
}

/* Disabled */
.input:disabled {
  @apply bg-gray-100 cursor-not-allowed;
}

/* Read-only */
.input:read-only {
  @apply bg-gray-50;
}
```

### Estados de Linha de Tabela

```css
/* Hover */
tr:hover {
  @apply bg-gray-50;
}

/* Selected */
tr.selected {
  @apply bg-primary-50;
}

/* Striped */
tbody tr:nth-child(even) {
  @apply bg-gray-50;
}
```

---

## Iconografia

### Biblioteca: Heroicons

```vue
<script setup>
import {
  // Navegação
  HomeIcon,
  ChevronRightIcon,
  ChevronDownIcon,
  ArrowLeftIcon,

  // Ações
  PlusIcon,
  PencilIcon,
  TrashIcon,
  MagnifyingGlassIcon,
  ArrowDownTrayIcon,
  PrinterIcon,

  // Status
  CheckCircleIcon,
  ExclamationCircleIcon,
  InformationCircleIcon,
  XCircleIcon,

  // Objetos
  UserIcon,
  DocumentIcon,
  CurrencyDollarIcon,
  TruckIcon,
  BuildingStorefrontIcon,

  // UI
  Bars3Icon,
  XMarkIcon,
  EllipsisVerticalIcon,
} from "@heroicons/vue/24/outline";
</script>
```

### Tamanhos

```html
<!-- Pequeno (16px) -->
<PlusIcon class="h-4 w-4" />

<!-- Médio (20px) - Padrão em botões -->
<PlusIcon class="h-5 w-5" />

<!-- Grande (24px) -->
<PlusIcon class="h-6 w-6" />
```

### Cores

```html
<!-- Cor primária -->
<CheckCircleIcon class="h-5 w-5 text-primary-600" />

<!-- Sucesso -->
<CheckCircleIcon class="h-5 w-5 text-green-500" />

<!-- Erro -->
<XCircleIcon class="h-5 w-5 text-red-500" />

<!-- Neutro -->
<InformationCircleIcon class="h-5 w-5 text-gray-400" />
```

---

## Responsividade

### Breakpoints

```javascript
// tailwind.config.js
module.exports = {
  theme: {
    screens: {
      sm: "640px", // Mobile landscape
      md: "768px", // Tablet
      lg: "1024px", // Desktop
      xl: "1280px", // Large desktop
      "2xl": "1536px", // Extra large
    },
  },
};
```

### Padrões de Layout Responsivo

```vue
<!-- Grid responsivo -->
<div
  class="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 xl:grid-cols-4 gap-4"
>
  <!-- Items -->
</div>

<!-- Sidebar + Content -->
<div class="flex flex-col lg:flex-row">
  <aside class="w-full lg:w-64 lg:flex-shrink-0">
    <!-- Sidebar -->
  </aside>
  <main class="flex-1 min-w-0">
    <!-- Content -->
  </main>
</div>

<!-- Stack to Row -->
<div class="flex flex-col sm:flex-row sm:items-center sm:justify-between">
  <h1 class="text-xl font-bold">Título</h1>
  <button class="mt-4 sm:mt-0">Ação</button>
</div>

<!-- Hide on mobile -->
<div class="hidden md:block">
  <!-- Visible only on md+ -->
</div>

<!-- Show only on mobile -->
<div class="md:hidden">
  <!-- Visible only on mobile -->
</div>
```

---

## Acessibilidade

### Requisitos WCAG 2.1 AA

| Critério         | Requisito           | Implementação               |
| ---------------- | ------------------- | --------------------------- |
| Contraste        | 4.5:1 texto, 3:1 UI | Verificar com DevTools      |
| Foco visível     | Todos interativos   | `focus:ring-2`              |
| Labels           | Todos inputs        | `<label for="">`            |
| Tamanho clicável | Mínimo 44x44px      | `min-h-[44px] min-w-[44px]` |
| Skip links       | Navegação principal | Link no topo                |

### Padrões de Acessibilidade

```vue
<!-- Label associada -->
<label for="email" class="block text-sm font-medium text-gray-700">
  Email
</label>
<input id="email" type="email" aria-describedby="email-hint" class="..." />
<p id="email-hint" class="text-sm text-gray-500">
  Usaremos para enviar notificações.
</p>

<!-- Botão com aria-label -->
<button aria-label="Fechar modal" class="p-2 hover:bg-gray-100 rounded">
  <XMarkIcon class="h-5 w-5" />
</button>

<!-- Estado de loading -->
<button :aria-busy="isLoading" :disabled="isLoading">
  <span v-if="isLoading">Salvando...</span>
  <span v-else>Salvar</span>
</button>

<!-- Mensagens de erro -->
<input
  :aria-invalid="hasError"
  :aria-describedby="hasError ? 'error-msg' : undefined"
/>
<p v-if="hasError" id="error-msg" class="text-red-600" role="alert">
  Campo obrigatório
</p>

<!-- Skip link -->
<a
  href="#main-content"
  class="sr-only focus:not-sr-only focus:absolute focus:top-4 focus:left-4"
>
  Pular para conteúdo principal
</a>
```

---

## Temas

### Light Theme (Padrão)

```css
:root {
  --bg-primary: #ffffff;
  --bg-secondary: #f9fafb;
  --text-primary: #111827;
  --text-secondary: #6b7280;
  --border-color: #e5e7eb;
}
```

### Dark Theme (Futuro)

```css
[data-theme="dark"] {
  --bg-primary: #1f2937;
  --bg-secondary: #111827;
  --text-primary: #f9fafb;
  --text-secondary: #9ca3af;
  --border-color: #374151;
}
```

### Implementação com Tailwind

```javascript
// tailwind.config.js
module.exports = {
  darkMode: "class", // ou 'media'
};
```

```vue
<!-- Componente com suporte a dark mode -->
<div
  class="
  bg-white dark:bg-gray-800
  text-gray-900 dark:text-white
  border-gray-200 dark:border-gray-700
"
>
  Conteúdo
</div>
```

---

## Componentes Vue

### Estrutura de Componentes

```text
resources/js/Components/
├── Base/
│   ├── Button.vue
│   ├── Input.vue
│   ├── Select.vue
│   ├── Checkbox.vue
│   └── Radio.vue
├── Feedback/
│   ├── Toast.vue
│   ├── Alert.vue
│   ├── Modal.vue
│   └── ConfirmDialog.vue
├── Data/
│   ├── DataTable.vue
│   ├── Pagination.vue
│   └── StatusBadge.vue
├── Layout/
│   ├── Sidebar.vue
│   ├── Header.vue
│   ├── PageHeader.vue
│   └── Card.vue
└── Form/
    ├── FormSection.vue
    ├── FormField.vue
    ├── MoneyInput.vue
    ├── CpfCnpjInput.vue
    └── DatePicker.vue
```

### Exemplo: Button Component

```vue
<!-- Components/Base/Button.vue -->
<script setup>
defineProps({
  variant: {
    type: String,
    default: "primary",
    validator: (v) => ["primary", "secondary", "danger", "ghost"].includes(v),
  },
  size: {
    type: String,
    default: "md",
    validator: (v) => ["sm", "md", "lg"].includes(v),
  },
  loading: Boolean,
  disabled: Boolean,
});

const variantClasses = {
  primary: "bg-primary-600 hover:bg-primary-700 text-white",
  secondary: "bg-white hover:bg-gray-50 text-gray-700 border border-gray-300",
  danger: "bg-red-600 hover:bg-red-700 text-white",
  ghost: "text-primary-600 hover:bg-primary-50",
};

const sizeClasses = {
  sm: "px-3 py-1.5 text-sm",
  md: "px-4 py-2 text-base",
  lg: "px-6 py-3 text-lg",
};
</script>

<template>
  <button
    :class="[
      'inline-flex items-center justify-center font-medium rounded-lg',
      'transition-colors focus:outline-none focus:ring-2 focus:ring-offset-2',
      variantClasses[variant],
      sizeClasses[size],
      { 'opacity-50 cursor-not-allowed': disabled || loading },
    ]"
    :disabled="disabled || loading"
    :aria-busy="loading"
  >
    <svg
      v-if="loading"
      class="animate-spin -ml-1 mr-2 h-4 w-4"
      fill="none"
      viewBox="0 0 24 24"
    >
      <circle
        class="opacity-25"
        cx="12"
        cy="12"
        r="10"
        stroke="currentColor"
        stroke-width="4"
      />
      <path
        class="opacity-75"
        fill="currentColor"
        d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4z"
      />
    </svg>
    <slot />
  </button>
</template>
```

---

## Checklist de Implementação

- [ ] Configurar Tailwind CSS
- [ ] Instalar e configurar PrimeVue
- [ ] Criar componentes base (Button, Input, Select)
- [ ] Criar componentes de feedback (Toast, Modal, Alert)
- [ ] Criar componentes de dados (DataTable, StatusBadge)
- [ ] Criar componentes de layout (Sidebar, Header, Card)
- [ ] Criar componentes de formulário (FormField, MoneyInput)
- [ ] Implementar tema light
- [ ] Documentar uso de componentes
- [ ] Testar acessibilidade

---

## Documentos Relacionados

- [03-frontend.md](./03-frontend.md) - Arquitetura frontend
- [12-atalhos-teclado.md](./12-atalhos-teclado.md) - Atalhos de teclado
- [../estrategia/11-treinamento.md](../estrategia/11-treinamento.md) - Treinamento

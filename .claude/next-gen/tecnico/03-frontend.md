# Avaliação de Framework Frontend

> Status: **Em Aberto - Decisão Necessária**
> Última atualização: 2025-12-27

---

## Visão Geral das Opções

| Opção | Tipo | Curva de Aprendizado | Interatividade | Complexidade |
|--------|------|----------------|---------------|------------|
| **Livewire** | Renderizado no servidor | Baixa | Média | Baixa |
| **Inertia + Vue** | Estilo SPA | Média | Alta | Média |
| **Inertia + React** | Estilo SPA | Média-Alta | Alta | Média |
| **SPA Completo + API** | Desacoplado | Alta | Máxima | Alta |

---

## Opção 1: Livewire

### O que é?
Renderização no servidor com atualizações reativas via AJAX. Componentes escritos em PHP.

### Prós
- Permanece no ecossistema PHP (sem framework JS para aprender)
- Modelo mental mais simples
- Menos ferramentas de build
- Bom para aplicações com muito CRUD
- Recursos de tempo real com Livewire 3

### Contras
- Mais carga no servidor (toda interação acessa o servidor)
- UX menos fluida comparada a SPA verdadeira
- Interações complexas podem ser complicadas
- Capacidade offline limitada

### Exemplo de Componente

```php
<?php
// app/Livewire/Compras/ListaCompras.php

namespace App\Livewire\Compras;

use Livewire\Component;
use Livewire\WithPagination;
use App\Models\Compra;

class ListaCompras extends Component
{
    use WithPagination;

    public $status = '';
    public $search = '';
    public $sortField = 'created_at';
    public $sortDirection = 'desc';

    protected $queryString = ['status', 'search'];

    public function updatingSearch()
    {
        $this->resetPage();
    }

    public function sortBy($field)
    {
        if ($this->sortField === $field) {
            $this->sortDirection = $this->sortDirection === 'asc' ? 'desc' : 'asc';
        } else {
            $this->sortField = $field;
            $this->sortDirection = 'asc';
        }
    }

    public function render()
    {
        $compras = Compra::query()
            ->when($this->status, fn($q) => $q->where('status', $this->status))
            ->when($this->search, fn($q) => $q->where('id', 'like', "%{$this->search}%"))
            ->orderBy($this->sortField, $this->sortDirection)
            ->paginate(20);

        return view('livewire.compras.lista-compras', [
            'compras' => $compras,
        ]);
    }
}
```

```blade
{{-- resources/views/livewire/compras/lista-compras.blade.php --}}
<div>
    <div class="flex gap-4 mb-4">
        <input wire:model.live.debounce.300ms="search"
               type="text"
               placeholder="Buscar..."
               class="input">

        <select wire:model.live="status" class="select">
            <option value="">Todos</option>
            @foreach(\App\Enums\CompraStatus::cases() as $s)
                <option value="{{ $s->value }}">{{ $s->label() }}</option>
            @endforeach
        </select>
    </div>

    <table class="table">
        <thead>
            <tr>
                <th wire:click="sortBy('id')" class="cursor-pointer">ID</th>
                <th wire:click="sortBy('fornecedor_id')" class="cursor-pointer">Fornecedor</th>
                <th wire:click="sortBy('status')" class="cursor-pointer">Status</th>
                <th wire:click="sortBy('total')" class="cursor-pointer">Total</th>
            </tr>
        </thead>
        <tbody>
            @foreach($compras as $compra)
                <tr>
                    <td>{{ $compra->id }}</td>
                    <td>{{ $compra->fornecedor->razao_social }}</td>
                    <td>
                        <span class="badge badge-{{ $compra->status->color() }}">
                            {{ $compra->status->label() }}
                        </span>
                    </td>
                    <td>R$ {{ number_format($compra->total, 2, ',', '.') }}</td>
                </tr>
            @endforeach
        </tbody>
    </table>

    {{ $compras->links() }}
</div>
```

---

## Opção 2: Inertia + Vue

### O que é?
Experiência estilo SPA sem construir uma API. Servidor renderiza dados, Vue cuida da UI.

### Prós
- Sensação de SPA sem complexidade de API
- Ótima experiência de desenvolvimento
- Reatividade do Vue para formulários complexos
- Reutilização de componentes entre páginas
- Bom suporte a TypeScript
- Recarregamentos parciais para performance

### Contras
- Precisa conhecer Vue.js
- Etapa de build necessária (Vite)
- Ligeiramente mais complexo que Livewire
- Tamanho inicial do bundle maior

### Exemplo de Componente

```php
<?php
// app/Http/Controllers/CompraController.php

namespace App\Http\Controllers;

use App\Models\Compra;
use Inertia\Inertia;

class CompraController extends Controller
{
    public function index(Request $request)
    {
        return Inertia::render('Compras/Index', [
            'compras' => Compra::query()
                ->with('fornecedor:id,razao_social')
                ->when($request->status, fn($q) => $q->where('status', $request->status))
                ->when($request->search, fn($q) => $q->where('id', 'like', "%{$request->search}%"))
                ->orderBy($request->sort ?? 'created_at', $request->direction ?? 'desc')
                ->paginate(20)
                ->withQueryString(),
            'filters' => $request->only(['status', 'search', 'sort', 'direction']),
            'statusOptions' => collect(CompraStatus::cases())->map(fn($s) => [
                'value' => $s->value,
                'label' => $s->label(),
            ]),
        ]);
    }
}
```

```vue
<!-- resources/js/Pages/Compras/Index.vue -->
<script setup lang="ts">
import { ref, watch } from 'vue'
import { router } from '@inertiajs/vue3'
import { useDebounceFn } from '@vueuse/core'
import Layout from '@/Layouts/AppLayout.vue'
import Pagination from '@/Components/Pagination.vue'
import Badge from '@/Components/Badge.vue'

interface Compra {
  id: number
  fornecedor: { id: number; razao_social: string }
  status: string
  status_label: string
  status_color: string
  total: number
}

const props = defineProps<{
  compras: { data: Compra[]; links: any[] }
  filters: { status?: string; search?: string }
  statusOptions: { value: string; label: string }[]
}>()

const search = ref(props.filters.search ?? '')
const status = ref(props.filters.status ?? '')

const applyFilters = useDebounceFn(() => {
  router.get('/compras', {
    search: search.value || undefined,
    status: status.value || undefined,
  }, {
    preserveState: true,
    replace: true,
  })
}, 300)

watch([search, status], applyFilters)

const sortBy = (field: string) => {
  router.get('/compras', {
    ...props.filters,
    sort: field,
    direction: props.filters.sort === field && props.filters.direction === 'asc' ? 'desc' : 'asc',
  }, {
    preserveState: true,
  })
}
</script>

<template>
  <Layout title="Compras">
    <div class="flex gap-4 mb-4">
      <input
        v-model="search"
        type="text"
        placeholder="Buscar..."
        class="input"
      />

      <select v-model="status" class="select">
        <option value="">Todos</option>
        <option v-for="s in statusOptions" :key="s.value" :value="s.value">
          {{ s.label }}
        </option>
      </select>
    </div>

    <table class="table">
      <thead>
        <tr>
          <th @click="sortBy('id')" class="cursor-pointer">ID</th>
          <th @click="sortBy('fornecedor_id')" class="cursor-pointer">Fornecedor</th>
          <th @click="sortBy('status')" class="cursor-pointer">Status</th>
          <th @click="sortBy('total')" class="cursor-pointer">Total</th>
        </tr>
      </thead>
      <tbody>
        <tr v-for="compra in compras.data" :key="compra.id">
          <td>{{ compra.id }}</td>
          <td>{{ compra.fornecedor.razao_social }}</td>
          <td>
            <Badge :color="compra.status_color">{{ compra.status_label }}</Badge>
          </td>
          <td>R$ {{ compra.total.toLocaleString('pt-BR', { minimumFractionDigits: 2 }) }}</td>
        </tr>
      </tbody>
    </table>

    <Pagination :links="compras.links" />
  </Layout>
</template>
```

---

## Opção 3: Inertia + React

Similar ao Vue mas usando o ecossistema React.

### Prós
- Ecossistema maior que Vue
- Melhor suporte a TypeScript
- Maior demanda no mercado de trabalho
- Mais bibliotecas de componentes (shadcn/ui, Radix)

### Contras
- Mais boilerplate que Vue
- Curva de aprendizado do JSX
- Hooks podem ser confusos inicialmente

---

## Opção 4: SPA Completo + API

Frontend completamente separado (React/Vue) com Laravel apenas como API.

### Prós
- Máxima flexibilidade
- App mobile pode usar mesma API
- Frontend pode ser hospedado em CDN
- Clara separação de responsabilidades

### Contras
- Duas bases de código para manter
- Complexidade de versionamento de API
- Autenticação mais complexa (tokens)
- CORS, mais infraestrutura

---

## Comparação para Casos de Uso de ERP

| Caso de Uso | Livewire | Inertia+Vue | SPA Completo |
|----------|----------|-------------|----------|
| **Tabelas de Dados** | Bom | Excelente | Excelente |
| **Formulários Complexos** | Médio | Excelente | Excelente |
| **Atualizações em Tempo Real** | Bom (polling) | Médio | Excelente (WS) |
| **Suporte Offline** | Nenhum | Limitado | Possível |
| **Impressão/Relatórios** | Fácil | Médio | Complexo |
| **Curva de Aprendizado** | Baixa | Média | Alta |
| **Familiaridade da Equipe** | Apenas PHP | PHP + Vue | PHP + React/Vue |

---

## Recursos Desktop Atuais a Considerar

### 1. Tabelas de Dados Pesadas
- TableView do Qt com delegates
- Edição inline
- Formatação customizada de células (moeda, datas)
- Ordenação, filtragem

**Melhor opção**: Inertia + Vue com TanStack Table ou AG Grid

### 2. Formulários Multi-Etapas Complexos
- Cadastro de produto com abas
- Criação de venda com itens de linha
- Visibilidade dinâmica de campos

**Melhor opção**: Inertia + Vue (melhor gerenciamento de estado)

### 3. Recursos em Tempo Real
- Atualizações de nível de estoque
- Mudanças de status de pedidos
- Polling de status de NFe

**Melhor opção**: SPA completo com WebSockets, ou Livewire com polling

### 4. Navegação por Teclado
- Tab entre campos
- Enter para enviar
- Atalhos de teclado

**Melhor opção**: Qualquer opção com gerenciamento de foco adequado

---

## Recomendação

**Inertia + Vue** parece ser o melhor equilíbrio:

1. **Experiência estilo SPA** sem complexidade de API
2. **Reatividade do Vue** lida bem com formulários complexos
3. **Recarregamentos parciais** reduzem transferência de dados
4. **Sem versionamento de API** necessário
5. **Suporte a TypeScript** para segurança de tipos
6. **Ecossistema crescente** (PrimeVue, Headless UI)

### Stack Sugerida
- **Inertia.js** - Integração com Laravel
- **Vue 3** - Composition API
- **TypeScript** - Segurança de tipos
- **Tailwind CSS** - Estilização utility-first
- **PrimeVue** - Componentes de UI (tabelas, formulários, diálogos)
- **VueUse** - Composables utilitários

---

## Questões a Resolver

1. **Experiência da equipe com Vue?** Se nenhuma, Livewire pode ser mais rápido para começar
2. **App mobile planejado?** Se sim, considere abordagem API-first
3. **Requisitos offline?** Crítico para trabalho em campo?
4. **Necessidades de tempo real?** Chat, atualizações ao vivo, notificações?

---

## Próximos Passos

1. Construir um protótipo de um módulo (Cadastro Fornecedor) em cada abordagem
2. Comparar velocidade de desenvolvimento e qualidade de código
3. Obter feedback da equipe sobre experiência de desenvolvimento
4. Tomar decisão final

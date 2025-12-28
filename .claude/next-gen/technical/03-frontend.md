# Frontend Framework Evaluation

> Status: **Open - Decision Needed**
> Last updated: 2025-12-27

---

## Options Overview

| Option | Type | Learning Curve | Interactivity | Complexity |
|--------|------|----------------|---------------|------------|
| **Livewire** | Server-rendered | Low | Medium | Low |
| **Inertia + Vue** | SPA-like | Medium | High | Medium |
| **Inertia + React** | SPA-like | Medium-High | High | Medium |
| **Full SPA + API** | Decoupled | High | Maximum | High |

---

## Option 1: Livewire

### What is it?
Server-side rendering with reactive updates via AJAX. Components written in PHP.

### Pros
- Stays in PHP ecosystem (no JS framework to learn)
- Simpler mental model
- Less build tooling
- Good for CRUD-heavy applications
- Real-time features with Livewire 3

### Cons
- More server load (every interaction hits server)
- Less smooth UX compared to true SPA
- Complex interactions can be tricky
- Limited offline capability

### Example Component

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

## Option 2: Inertia + Vue

### What is it?
SPA-like experience without building an API. Server renders data, Vue handles UI.

### Pros
- SPA feel without API complexity
- Great developer experience
- Vue's reactivity for complex forms
- Component reuse across pages
- Good TypeScript support
- Partial reloads for performance

### Cons
- Need to know Vue.js
- Build step required (Vite)
- Slightly more complex than Livewire
- Initial bundle size larger

### Example Component

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

## Option 3: Inertia + React

Similar to Vue but using React ecosystem.

### Pros
- Larger ecosystem than Vue
- Better TypeScript support
- More job market demand
- More component libraries (shadcn/ui, Radix)

### Cons
- More boilerplate than Vue
- JSX learning curve
- Hooks can be confusing initially

---

## Option 4: Full SPA + API

Completely separate frontend (React/Vue) with Laravel as API only.

### Pros
- Maximum flexibility
- Can have mobile app use same API
- Frontend can be hosted on CDN
- Clear separation of concerns

### Cons
- Two codebases to maintain
- API versioning complexity
- Authentication more complex (tokens)
- CORS, more infrastructure

---

## Comparison for ERP Use Cases

| Use Case | Livewire | Inertia+Vue | Full SPA |
|----------|----------|-------------|----------|
| **Data Tables** | Good | Excellent | Excellent |
| **Complex Forms** | Medium | Excellent | Excellent |
| **Real-time Updates** | Good (polling) | Medium | Excellent (WS) |
| **Offline Support** | None | Limited | Possible |
| **Print/Reports** | Easy | Medium | Complex |
| **Learning Curve** | Low | Medium | High |
| **Team Familiarity** | PHP only | PHP + Vue | PHP + React/Vue |

---

## Current Desktop Features to Consider

### 1. Heavy Data Tables
- Qt TableView with delegates
- Inline editing
- Custom cell formatting (currency, dates)
- Sorting, filtering

**Best fit**: Inertia + Vue with TanStack Table or AG Grid

### 2. Complex Multi-Step Forms
- Product registration with tabs
- Sale creation with line items
- Dynamic field visibility

**Best fit**: Inertia + Vue (better state management)

### 3. Real-Time Features
- Stock level updates
- Order status changes
- NFe status polling

**Best fit**: Full SPA with WebSockets, or Livewire with polling

### 4. Keyboard Navigation
- Tab through fields
- Enter to submit
- Keyboard shortcuts

**Best fit**: Any option with proper focus management

---

## Recommendation

**Inertia + Vue** appears to be the best balance:

1. **SPA-like experience** without API complexity
2. **Vue's reactivity** handles complex forms well
3. **Partial reloads** reduce data transfer
4. **No API versioning** needed
5. **TypeScript support** for type safety
6. **Growing ecosystem** (PrimeVue, Headless UI)

### Suggested Stack
- **Inertia.js** - Laravel integration
- **Vue 3** - Composition API
- **TypeScript** - Type safety
- **Tailwind CSS** - Utility-first styling
- **PrimeVue** - UI components (tables, forms, dialogs)
- **VueUse** - Utility composables

---

## Questions to Resolve

1. **Team Vue experience?** If none, Livewire might be faster to start
2. **Mobile app planned?** If yes, consider API-first approach
3. **Offline requirements?** Critical for field work?
4. **Real-time needs?** Chat, live updates, notifications?

---

## Next Steps

1. Build a prototype of one module (Cadastro Fornecedor) in each approach
2. Compare development speed and code quality
3. Get team feedback on developer experience
4. Make final decision

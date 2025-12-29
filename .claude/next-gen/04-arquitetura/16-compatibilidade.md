# Matriz de Suporte Browser/Dispositivo

> Status: **Aprovado**
> Última atualização: 2025-12-28

---

## Visão Geral

Este documento define os browsers e dispositivos suportados pelo ERP Staccato Web, incluindo funcionalidades específicas por plataforma.

---

## Browsers Suportados

### Desktop (Prioridade Alta)

| Browser | Versão Mínima | Status | Market Share* |
|---------|---------------|--------|---------------|
| Google Chrome | 100+ | Suportado | ~65% |
| Microsoft Edge | 100+ | Suportado | ~15% |
| Mozilla Firefox | 100+ | Suportado | ~5% |
| Safari (macOS) | 15+ | Suportado | ~10% |
| Opera | 90+ | Compatível | ~2% |

*Market share estimado no contexto empresarial brasileiro

### Mobile (Prioridade Média)

| Browser | Versão Mínima | Status | Notas |
|---------|---------------|--------|-------|
| Chrome Mobile | 100+ | Suportado | Android |
| Safari Mobile | 15+ | Suportado | iOS |
| Samsung Internet | 18+ | Compatível | Android Samsung |

### Não Suportados

| Browser | Motivo |
|---------|--------|
| Internet Explorer | Descontinuado pela Microsoft |
| Edge Legacy | Descontinuado |
| Opera Mini | Limitações de JavaScript |

---

## Dispositivos Suportados

### Desktop

| Dispositivo | Resolução Mínima | Status |
|-------------|------------------|--------|
| Desktop/Laptop | 1280 x 720 | Suportado |
| Desktop HD | 1920 x 1080 | Recomendado |
| Desktop 4K | 3840 x 2160 | Suportado |

### Tablet

| Dispositivo | Resolução | Status | Limitações |
|-------------|-----------|--------|------------|
| iPad (10"+) | 1024 x 768+ | Suportado | Alguns modals podem ser apertados |
| iPad Pro | 2048 x 1536+ | Suportado | Experiência completa |
| Android Tablet | 1024 x 768+ | Suportado | Depende do browser |

### Mobile

| Dispositivo | Resolução | Status | Limitações |
|-------------|-----------|--------|------------|
| iPhone | 375+ | Limitado | Visualização apenas |
| Android | 360+ | Limitado | Visualização apenas |

---

## Funcionalidades por Plataforma

### Matriz de Funcionalidades

| Funcionalidade | Desktop | Tablet | Mobile |
|----------------|---------|--------|--------|
| Login/Logout | ✅ | ✅ | ✅ |
| Dashboard | ✅ | ✅ | ✅ |
| Listagem de registros | ✅ | ✅ | ✅ |
| Criar/Editar orçamento | ✅ | ✅ | ❌ |
| Criar/Editar venda | ✅ | ✅ | ❌ |
| Emitir NFe | ✅ | ⚠️ | ❌ |
| Cadastro de clientes | ✅ | ✅ | ⚠️ |
| Relatórios | ✅ | ✅ | ⚠️ |
| Impressão | ✅ | ⚠️ | ❌ |
| Certificado Digital | ✅ | ❌ | ❌ |
| Atalhos de teclado | ✅ | ❌ | ❌ |
| Notificações push | ✅ | ✅ | ✅ |

**Legenda:**

- ✅ Totalmente suportado
- ⚠️ Funcionalidade limitada
- ❌ Não disponível

### Funcionalidades Exclusivas Desktop

1. **Emissão de NFe**
   - Requer certificado digital A1/A3
   - Extensão de browser ou WebUSB

2. **Impressão Direta**
   - Etiquetas térmicas
   - Impressoras específicas

3. **Atalhos de Teclado**
   - Navegação rápida
   - Command Palette (Ctrl+K)

4. **Operações em Massa**
   - Seleção múltipla
   - Drag and drop

---

## Requisitos Técnicos

### JavaScript

```javascript
// Recursos ES6+ necessários
const features = {
  'Arrow functions': true,
  'Template literals': true,
  'Destructuring': true,
  'Spread operator': true,
  'Async/await': true,
  'Optional chaining': true,  // ES2020
  'Nullish coalescing': true, // ES2020
  'BigInt': false,            // Não utilizado
  'Top-level await': false,   // Não utilizado
};
```

### CSS

```css
/* Recursos CSS necessários */
.features {
  /* Flexbox - suportado em todos */
  display: flex;

  /* Grid - suportado em todos */
  display: grid;

  /* Custom Properties - suportado em todos */
  --primary: #2563eb;

  /* Backdrop filter - suportado com prefixo no Safari */
  backdrop-filter: blur(10px);
  -webkit-backdrop-filter: blur(10px);

  /* Container queries - verificar suporte */
  container-type: inline-size;
}
```

### APIs Web

| API | Chrome | Firefox | Safari | Edge | Uso |
|-----|--------|---------|--------|------|-----|
| Fetch | ✅ | ✅ | ✅ | ✅ | Requisições HTTP |
| LocalStorage | ✅ | ✅ | ✅ | ✅ | Preferências |
| IndexedDB | ✅ | ✅ | ✅ | ✅ | Cache offline |
| WebSocket | ✅ | ✅ | ✅ | ✅ | Tempo real |
| Notifications | ✅ | ✅ | ⚠️ | ✅ | Alertas |
| Clipboard | ✅ | ✅ | ✅ | ✅ | Copiar/Colar |
| File API | ✅ | ✅ | ✅ | ✅ | Upload |
| WebUSB | ✅ | ❌ | ❌ | ✅ | Certificado A3 |
| Web Crypto | ✅ | ✅ | ✅ | ✅ | Criptografia |

---

## Resoluções e Breakpoints

### Breakpoints (Tailwind)

```javascript
// tailwind.config.js
module.exports = {
  theme: {
    screens: {
      'sm': '640px',   // Mobile landscape
      'md': '768px',   // Tablet portrait
      'lg': '1024px',  // Tablet landscape / Desktop
      'xl': '1280px',  // Desktop
      '2xl': '1536px', // Large desktop
    },
  },
};
```

### Layout por Resolução

| Resolução | Layout | Sidebar | Tabela |
|-----------|--------|---------|--------|
| < 640px | Mobile | Oculta | Scroll horizontal |
| 640-768px | Mobile | Oculta | Cards |
| 768-1024px | Tablet | Colapsada | Tabela simplificada |
| 1024-1280px | Desktop | Fixa | Tabela completa |
| > 1280px | Desktop | Fixa | Tabela + detalhes |

---

## Testes de Compatibilidade

### Ferramentas

| Ferramenta | Propósito | Custo |
|------------|-----------|-------|
| BrowserStack | Cross-browser testing | Pago |
| Sauce Labs | Cross-browser testing | Pago |
| Chrome DevTools | Emulação de dispositivos | Gratuito |
| Firefox DevTools | Emulação de dispositivos | Gratuito |
| Safari Web Inspector | Debug iOS/macOS | Gratuito |

### Checklist de Testes

#### Por Browser

- [ ] Chrome (Windows)
- [ ] Chrome (macOS)
- [ ] Edge (Windows)
- [ ] Firefox (Windows)
- [ ] Firefox (macOS)
- [ ] Safari (macOS)

#### Por Resolução

- [ ] 1920x1080 (Desktop HD)
- [ ] 1366x768 (Laptop comum)
- [ ] 1280x720 (Mínimo desktop)
- [ ] 1024x768 (Tablet portrait)
- [ ] 768x1024 (Tablet landscape)
- [ ] 375x812 (iPhone X+)
- [ ] 360x640 (Android comum)

#### Por Funcionalidade

- [ ] Login/Logout
- [ ] Navegação
- [ ] Formulários
- [ ] Tabelas com paginação
- [ ] Modals
- [ ] Toasts/Notificações
- [ ] Upload de arquivos
- [ ] Impressão

---

## Polyfills e Fallbacks

### Polyfills Incluídos

```javascript
// vite.config.js com @vitejs/plugin-legacy
import legacy from '@vitejs/plugin-legacy';

export default {
  plugins: [
    legacy({
      targets: ['defaults', 'not IE 11'],
      polyfills: [
        'es.promise',
        'es.array.iterator',
        'es.object.assign',
        'es.symbol',
      ],
    }),
  ],
};
```

### Fallbacks de CSS

```css
/* Fallback para container queries */
.container {
  /* Fallback usando media query */
  width: 100%;
}

@supports (container-type: inline-size) {
  .container {
    container-type: inline-size;
  }
}

/* Fallback para backdrop-filter */
.modal-backdrop {
  background: rgba(0, 0, 0, 0.5);
}

@supports (backdrop-filter: blur(10px)) {
  .modal-backdrop {
    background: rgba(0, 0, 0, 0.3);
    backdrop-filter: blur(10px);
  }
}
```

---

## Progressive Enhancement

### Níveis de Experiência

```text
┌─────────────────────────────────────────────────────────────┐
│                     Nível de Experiência                     │
├─────────────────────────────────────────────────────────────┤
│ BÁSICO (todos os browsers)                                  │
│ ├── Navegação funcional                                     │
│ ├── Formulários funcionais                                  │
│ ├── Visualização de dados                                   │
│ └── Operações CRUD básicas                                  │
├─────────────────────────────────────────────────────────────┤
│ INTERMEDIÁRIO (browsers modernos)                           │
│ ├── Animações CSS                                           │
│ ├── Lazy loading de imagens                                 │
│ ├── Autocomplete                                            │
│ └── Drag and drop                                           │
├─────────────────────────────────────────────────────────────┤
│ AVANÇADO (Chrome/Edge)                                      │
│ ├── WebUSB (certificado A3)                                 │
│ ├── Impressão direta                                        │
│ ├── Notificações push                                       │
│ └── PWA offline                                             │
└─────────────────────────────────────────────────────────────┘
```

---

## Mensagens de Compatibilidade

### Detecção de Browser

```typescript
// composables/useBrowserCheck.ts
export function useBrowserCheck() {
  const isSupported = computed(() => {
    const ua = navigator.userAgent;

    // IE não suportado
    if (ua.includes('MSIE') || ua.includes('Trident/')) {
      return false;
    }

    // Verificar versões mínimas
    // Chrome 100+, Firefox 100+, Safari 15+, Edge 100+
    return true;
  });

  const showWarning = computed(() => {
    // Mobile com funcionalidade limitada
    return /iPhone|iPad|Android/i.test(navigator.userAgent);
  });

  return { isSupported, showWarning };
}
```

### Componente de Aviso

```vue
<!-- components/BrowserWarning.vue -->
<template>
  <div v-if="!isSupported" class="bg-red-50 p-4">
    <h3>Navegador não suportado</h3>
    <p>Por favor, use Chrome, Firefox, Edge ou Safari.</p>
  </div>

  <div v-else-if="showWarning" class="bg-yellow-50 p-4">
    <h3>Funcionalidade limitada</h3>
    <p>Para melhor experiência, use um computador desktop.</p>
  </div>
</template>
```

---

## Atualizações Futuras

### Roadmap de Compatibilidade

| Versão | Mudança | Impacto |
|--------|---------|---------|
| 1.0 | Baseline atual | - |
| 1.1 | PWA com offline | Requer Service Worker |
| 1.2 | Suporte a impressoras USB | WebUSB expandido |
| 2.0 | App nativo (Capacitor) | iOS/Android |

### Monitoramento

- Coletar dados de browser via analytics
- Alertar quando browsers antigos ultrapassarem threshold
- Revisar matriz a cada 6 meses

---

## Documentos Relacionados

- [10-design-system.md](./10-design-system.md) - Design System e responsividade
- [12-atalhos-teclado.md](./12-atalhos-teclado.md) - Atalhos (desktop only)
- [../estrategia/11-treinamento.md](../estrategia/11-treinamento.md) - FAQ sobre dispositivos

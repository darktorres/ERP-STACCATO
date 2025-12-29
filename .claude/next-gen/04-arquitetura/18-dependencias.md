# Auditoria de Dependências

> Status: **Aprovado**
> Última atualização: 2025-12-28

---

## Visão Geral

Este documento lista todas as dependências do ERP Staccato Laravel, incluindo versões, licenças, riscos e estratégias de atualização.

---

## Dependências PHP (Composer)

### Produção

| Pacote | Versão | Licença | Criticidade | Descrição |
|--------|--------|---------|-------------|-----------|
| `laravel/framework` | ^12.0 | MIT | Crítico | Framework principal |
| `laravel/sanctum` | ^4.0 | MIT | Crítico | Autenticação SPA/API |
| `laravel/fortify` | ^1.21 | MIT | Alta | Autenticação features |
| `inertiajs/inertia-laravel` | ^2.0 | MIT | Crítico | Inertia.js adapter |
| `spatie/laravel-permission` | ^6.0 | MIT | Crítico | Roles e permissões |
| `spatie/laravel-activitylog` | ^4.8 | MIT | Alta | Auditoria |
| `spatie/laravel-query-builder` | ^6.0 | MIT | Alta | Query filters/sorts |
| `barryvdh/laravel-dompdf` | ^3.0 | MIT | Alta | Geração de PDF |
| `maatwebsite/excel` | ^3.1 | MIT | Alta | Import/Export Excel |
| `sentry/sentry-laravel` | ^4.0 | MIT | Média | Error tracking |
| `predis/predis` | ^2.2 | MIT | Alta | Redis client |
| `guzzlehttp/guzzle` | ^7.8 | MIT | Alta | HTTP client |
| `league/flysystem-aws-s3-v3` | ^3.0 | MIT | Baixa | S3 storage |
| `laravel/horizon` | ^5.0 | MIT | Média | Queue dashboard |
| `laravel/pulse` | ^1.0 | MIT | Média | Monitoring |

### Desenvolvimento

| Pacote | Versão | Licença | Descrição |
|--------|--------|---------|-----------|
| `laravel/pint` | ^1.18 | MIT | Code style |
| `phpstan/phpstan` | ^2.0 | MIT | Static analysis |
| `larastan/larastan` | ^3.0 | MIT | PHPStan for Laravel |
| `pestphp/pest` | ^3.0 | MIT | Testing framework |
| `mockery/mockery` | ^1.6 | BSD-3 | Mocking |
| `laravel/telescope` | ^5.0 | MIT | Debug assistant |
| `barryvdh/laravel-debugbar` | ^3.0 | MIT | Debug toolbar |
| `fakerphp/faker` | ^1.23 | MIT | Fake data |

### Integrações Brasileiras

| Pacote | Versão | Licença | Descrição |
|--------|--------|---------|-----------|
| `nfe/sped-nfe` | ^5.0 | LGPL-3.0 | NFe library |
| `nfe/sped-common` | ^5.0 | LGPL-3.0 | Common SPED |
| `eduardokum/laravel-boleto` | ^2.0 | MIT | Boletos bancários |
| `eduardokum/laravel-cnab` | ^2.0 | MIT | CNAB 240/400 |

---

## Dependências NPM

### Produção

| Pacote | Versão | Licença | Criticidade | Descrição |
|--------|--------|---------|-------------|-----------|
| `vue` | ^3.5 | MIT | Crítico | Framework frontend |
| `@inertiajs/vue3` | ^2.0 | MIT | Crítico | Inertia.js Vue |
| `@vueuse/core` | ^12.0 | MIT | Alta | Vue composables |
| `primevue` | ^4.0 | MIT | Crítico | UI components |
| `tailwindcss` | ^4.0 | MIT | Crítico | CSS framework |
| `axios` | ^1.7 | MIT | Alta | HTTP client |
| `pinia` | ^2.2 | MIT | Alta | State management |
| `vue-router` | ^4.4 | MIT | Alta | Routing |
| `chart.js` | ^4.4 | MIT | Média | Charts |
| `vue-chartjs` | ^5.3 | MIT | Média | Chart.js for Vue |
| `date-fns` | ^4.0 | MIT | Média | Date utilities |
| `zod` | ^3.23 | MIT | Alta | Schema validation |

### Desenvolvimento

| Pacote | Versão | Licença | Descrição |
|--------|--------|---------|-----------|
| `vite` | ^6.0 | MIT | Build tool |
| `@vitejs/plugin-vue` | ^5.0 | MIT | Vue plugin |
| `typescript` | ^5.6 | Apache-2.0 | TypeScript |
| `vue-tsc` | ^2.0 | MIT | Vue type check |
| `eslint` | ^9.0 | MIT | Linting |
| `@typescript-eslint/parser` | ^8.0 | BSD-2 | TS parser |
| `prettier` | ^3.4 | MIT | Formatting |
| `cypress` | ^13.0 | MIT | E2E testing |
| `vitest` | ^2.0 | MIT | Unit testing |

---

## Análise de Licenças

### Por Tipo de Licença

| Licença | Quantidade | Permissiva | Uso Comercial |
|---------|------------|------------|---------------|
| MIT | 45 | ✅ Sim | ✅ Sim |
| BSD-2 | 2 | ✅ Sim | ✅ Sim |
| BSD-3 | 1 | ✅ Sim | ✅ Sim |
| Apache-2.0 | 1 | ✅ Sim | ✅ Sim |
| LGPL-3.0 | 2 | ⚠️ Parcial | ✅ Sim |

### Licenças que Requerem Atenção

#### LGPL-3.0 (sped-nfe, sped-common)

**Implicações:**
- Código do pacote pode ser modificado
- Modificações devem ser disponibilizadas sob LGPL
- **Não afeta** código do ERP Staccato (uso como biblioteca)

**Mitigação:**
- Usar como dependência externa (OK)
- Não modificar código fonte
- Se modificar, criar fork público

---

## Análise de Riscos

### Pacotes Críticos sem Alternativa

| Pacote | Risco | Motivo | Mitigação |
|--------|-------|--------|-----------|
| `laravel/framework` | Baixo | Muito ativo, Anthropic-backed | Manter atualizado |
| `inertiajs/inertia-laravel` | Médio | Mantenedor único | Monitorar, ter plano B |
| `primevue` | Médio | Comunidade menor | Ter fallback para Headless UI |
| `nfe/sped-nfe` | Alto | Específico Brasil | Alternativa: ACBr API |

### Pacotes com Vulnerabilidades Conhecidas

```bash
# Verificar vulnerabilidades PHP
composer audit

# Verificar vulnerabilidades NPM
npm audit
```

**Política:**
- Vulnerabilidades críticas: Corrigir em 24h
- Vulnerabilidades altas: Corrigir em 1 semana
- Vulnerabilidades médias: Próximo sprint
- Vulnerabilidades baixas: Próxima release

---

## Estratégia de Atualização

### Dependabot Configuration

```yaml
# .github/dependabot.yml
version: 2
updates:
  # Composer
  - package-ecosystem: "composer"
    directory: "/"
    schedule:
      interval: "weekly"
    groups:
      laravel:
        patterns:
          - "laravel/*"
      spatie:
        patterns:
          - "spatie/*"
    ignore:
      - dependency-name: "*"
        update-types: ["version-update:semver-major"]

  # NPM
  - package-ecosystem: "npm"
    directory: "/"
    schedule:
      interval: "weekly"
    groups:
      vue:
        patterns:
          - "vue"
          - "@vue/*"
          - "@vueuse/*"
      dev:
        patterns:
          - "eslint*"
          - "prettier*"
          - "typescript*"
```

### Processo de Atualização

```
┌─────────────────────────────────────────────────────────────┐
│                   Processo de Atualização                    │
├─────────────────────────────────────────────────────────────┤
│ 1. Dependabot cria PR                                       │
│    ↓                                                        │
│ 2. CI roda testes automaticamente                           │
│    ↓                                                        │
│ 3. Aprovação manual para major updates                      │
│    ↓                                                        │
│ 4. Deploy em staging                                        │
│    ↓                                                        │
│ 5. Testes manuais (24h)                                     │
│    ↓                                                        │
│ 6. Deploy em produção                                       │
└─────────────────────────────────────────────────────────────┘
```

### Calendário de Atualizações

| Tipo | Frequência | Processo |
|------|------------|----------|
| Patch (0.0.x) | Semanal | Automático |
| Minor (0.x.0) | Quinzenal | Semi-automático |
| Major (x.0.0) | Trimestral | Manual com planejamento |
| Segurança | Imediato | Hotfix |

---

## Pacotes Abandonados

### Verificação de Atividade

| Pacote | Último Commit | Status | Ação |
|--------|---------------|--------|------|
| Todos ativos | < 6 meses | OK | - |

**Critérios de Abandono:**
- Sem commits há 12+ meses
- Issues críticos sem resposta
- Vulnerabilidades não corrigidas

**Alternativas Planejadas:**

| Se Abandonado | Alternativa |
|---------------|-------------|
| PrimeVue | Headless UI + TailwindUI |
| Inertia.js | Livewire ou API + SPA |
| sped-nfe | ACBr REST API |

---

## Auditoria de Segurança

### Comando de Auditoria

```bash
# PHP
composer audit --locked

# NPM
npm audit

# Gerar relatório
npm audit --json > npm-audit.json
```

### Integração CI

```yaml
# .github/workflows/security.yml
name: Security Audit

on:
  schedule:
    - cron: '0 6 * * 1'  # Segunda às 6h
  workflow_dispatch:

jobs:
  audit:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4

      - name: PHP Audit
        run: composer audit --locked

      - name: NPM Audit
        run: npm audit --audit-level=high

      - name: Notify on vulnerabilities
        if: failure()
        uses: slackapi/slack-github-action@v1
        with:
          channel-id: security-alerts
          payload: |
            {
              "text": "Security vulnerabilities found!"
            }
```

---

## Tamanho do Bundle

### Análise de Bundle JS

```javascript
// vite.config.js
import { visualizer } from 'rollup-plugin-visualizer';

export default {
  plugins: [
    visualizer({
      filename: 'bundle-stats.html',
      open: true,
    }),
  ],
};
```

### Alvos de Tamanho

| Chunk | Alvo | Atual | Status |
|-------|------|-------|--------|
| vendor.js | < 200KB | TBD | - |
| app.js | < 100KB | TBD | - |
| primevue.js | < 150KB | TBD | - |
| charts.js | < 80KB | TBD | - |
| Total | < 500KB | TBD | - |

### Otimizações

```javascript
// Tree-shaking PrimeVue
import Button from 'primevue/button';
import DataTable from 'primevue/datatable';
// Não: import PrimeVue from 'primevue';

// Lazy loading de charts
const ChartComponent = defineAsyncComponent(() =>
  import('./components/Chart.vue')
);
```

---

## Lock Files

### Gerenciamento

| Arquivo | Versionado | Propósito |
|---------|------------|-----------|
| `composer.lock` | ✅ Sim | Reprodutibilidade PHP |
| `package-lock.json` | ✅ Sim | Reprodutibilidade NPM |

### Políticas

- **Nunca** editar lock files manualmente
- **Sempre** commitar lock files
- Usar `composer install` (não `update`) em CI
- Usar `npm ci` (não `install`) em CI

---

## Checklist de Revisão

### Antes de Adicionar Dependência

- [ ] A dependência é realmente necessária?
- [ ] Existe alternativa nativa do Laravel/Vue?
- [ ] Qual a licença? É compatível?
- [ ] Quantos downloads/stars tem?
- [ ] Está sendo mantida ativamente?
- [ ] Qual o tamanho do bundle?
- [ ] Tem vulnerabilidades conhecidas?

### Antes de Atualizar Major Version

- [ ] Ler changelog completo
- [ ] Verificar breaking changes
- [ ] Testar em ambiente local
- [ ] Testar em staging
- [ ] Ter plano de rollback
- [ ] Comunicar equipe

---

## Documentos Relacionados

- [07-testes.md](./07-testes.md) - Testes de dependências
- [14-devops.md](./14-devops.md) - CI/CD com auditorias
- [08-erros-monitoramento.md](./08-erros-monitoramento.md) - Monitoramento

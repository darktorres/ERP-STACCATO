# Reorganização da Documentação

> Status: **Em Progresso**
> Criado: 2025-12-28
> Última atualização: 2025-12-28

---

## Visão Geral

Resultado da auditoria de organização dos 47 arquivos markdown em `.claude/next-gen/`.

---

## Estatísticas

| Métrica | Valor |
|---------|-------|
| Total de arquivos | 47 |
| Pastas | 6 (root, estrategia, tecnico, tecnico/modulos, negocios, meta) |
| Arquivos >1000 linhas | 14 |
| Arquivos 500-1000 linhas | 14 |
| Arquivos <500 linhas | 19 |

---

## Problemas Identificados

### 1. Numeração Duplicada (CRÍTICO)

```
tecnico/17-validacao.md     (1287 linhas) - Estratégia de Validação
tecnico/17-dependencias.md  (388 linhas)  - Auditoria de Dependências
```

**Ambos têm número 17. Um deve ser renumerado.**

- [ ] Renumerar `17-dependencias.md` → `18-dependencias.md`

---

### 2. Arquivos de Índice Desatualizados

#### `00-indice.md`

Lista apenas:
- tecnico/01-04 (faltam 05-17 = **13 arquivos**)
- modulos/ menciona só compras.md e nfe.md (faltam **6 arquivos**)
- negocios/ falta 05-regras-negocio.md
- estrategia/ faltam 09, 10, 11

- [ ] Atualizar `00-indice.md` com todos os 47 arquivos

#### `meta/rastreador.md`

Mesma estrutura desatualizada do índice principal.

- [ ] Atualizar `meta/rastreador.md` com estrutura atual

#### `tecnico/modulos/_indice.md`

| Lista | Status |
|-------|--------|
| compras.md | ✅ Existe |
| estoque.md | ✅ Existe |
| financeiro.md | ✅ Existe |
| nfe.md | ✅ Existe |
| vendas.md | ✅ Existe |
| logistica.md | ✅ Existe |
| galpao.md | ❌ **NÃO EXISTE** |
| cadastros.md | ✅ Existe mas **NÃO LISTADO** |
| relatorios.md | ✅ Existe mas **NÃO LISTADO** |

- [ ] Remover referência a `galpao.md` (não existe)
- [ ] Adicionar `cadastros.md` ao índice
- [ ] Adicionar `relatorios.md` ao índice

---

### 3. Sobreposição de Conteúdo (Schema)

| Arquivo | Linhas | Conteúdo |
|---------|--------|----------|
| `tecnico/02-banco-dados.md` | 519 | Princípios PostgreSQL, problemas |
| `estrategia/07-esquema-redesenhado.md` | 1155 | Schema redesenhado completo |

**Decisão necessária:** Mesclar ou definir escopos distintos com cross-references.

- [ ] Decidir: mesclar OU clarificar escopo de cada documento
- [ ] Adicionar cross-references entre os dois

---

### 4. Sobreposição de Conteúdo (NFe)

| Arquivo | Linhas | Conteúdo |
|---------|--------|----------|
| `tecnico/09-integracoes.md` | 989 | Protocolo ACBr, todas integrações |
| `tecnico/modulos/nfe.md` | 1040 | Implementação do módulo NFe |

**Status:** Sobreposição aceitável - focos diferentes (protocolo vs módulo).

- [ ] Adicionar cross-references entre os dois

---

## Tarefas

### Prioridade 0 (Crítico) ✅ COMPLETO

- [x] Renumerar `tecnico/17-dependencias.md` → `tecnico/18-dependencias.md`
- [x] Atualizar `00-indice.md` com estrutura completa
- [x] Atualizar `meta/rastreador.md` com estrutura completa
- [x] Corrigir `tecnico/modulos/_indice.md` (remover galpao, adicionar cadastros/relatorios)

### Prioridade 1 (Importante) ✅ COMPLETO

- [x] Adicionar cross-references entre documentos relacionados:
  - [x] `02-banco-dados.md` ↔ `07-esquema-redesenhado.md`
  - [x] `09-integracoes.md` ↔ `modulos/nfe.md`

### Prioridade 2 (Decisão) ✅ COMPLETO

- [x] Decidir sobre `banco-dados` + `esquema-redesenhado`: **MANTER SEPARADOS** com escopos claros
  - `02-banco-dados.md`: Justificativa PostgreSQL, análise de problemas, princípios DB
  - `07-esquema-redesenhado.md`: Schema completo, ENUMs, state machines, eventos, migração

---

## Estrutura Atual Completa

```
.claude/next-gen/
├── 00-indice.md
│
├── estrategia/
│   ├── 00-comparativo-legado-novo.md
│   ├── 01-plano-migracao.md
│   ├── 02-decisoes.md
│   ├── 03-melhorias.md
│   ├── 04-simplificacao-l1l2.md
│   ├── 05-correcao-fifo.md
│   ├── 06-normalizacao-fornecedor.md
│   ├── 07-esquema-redesenhado.md
│   ├── 08-design-greenfield.md
│   ├── 09-migracao-dados.md
│   ├── 10-paridade-funcionalidades.md
│   └── 11-treinamento.md
│
├── negocios/
│   ├── 01-visao-geral-fluxos.md
│   ├── 02-fluxos-estoque.md
│   ├── 03-fluxos-entrega-nfe.md
│   ├── 04-fluxos-cadastros.md
│   └── 05-regras-negocio.md
│
├── tecnico/
│   ├── 01-arquitetura.md
│   ├── 02-banco-dados.md
│   ├── 03-frontend.md
│   ├── 04-infraestrutura.md
│   ├── 05-seguranca.md
│   ├── 06-api.md
│   ├── 07-testes.md
│   ├── 08-erros-monitoramento.md
│   ├── 09-integracoes.md
│   ├── 10-design-system.md
│   ├── 11-concorrencia.md
│   ├── 12-atalhos-teclado.md
│   ├── 13-impressao.md
│   ├── 14-devops.md
│   ├── 15-dicionario-dados.md
│   ├── 16-compatibilidade.md
│   ├── 17-validacao.md
│   ├── 18-dependencias.md  ← RENUMERAR
│   └── modulos/
│       ├── _indice.md
│       ├── cadastros.md
│       ├── compras.md
│       ├── estoque.md
│       ├── financeiro.md
│       ├── logistica.md
│       ├── nfe.md
│       ├── relatorios.md
│       └── vendas.md
│
└── meta/
    ├── backlog-documentacao.md
    ├── rastreador.md
    └── reorganizacao-docs.md  ← ESTE ARQUIVO
```

---

## Log de Progresso

### 2025-12-28

- [x] Auditoria completa dos 47 arquivos
- [x] Identificados problemas de organização
- [x] Criado este documento de tracking
- [x] Executar correções P0 ✅
- [x] Executar correções P1 ✅

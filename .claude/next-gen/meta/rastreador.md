# Rastreador de Documentação

> Status: **Completo**
> Última atualização: 2025-12-27

---

## Propósito

Acompanhar o progresso da documentação de todos os fluxos de negócio para o projeto de migração web.

---

## Estrutura da Documentação

Reorganizado em 2025-12-27 em pastas categóricas:

```
.claude/next-gen/
├── 00-indice.md                      # Navegação principal
├── tecnico/                          # Arquitetura técnica
│   ├── 01-arquitetura.md             # Design Laravel
│   ├── 02-banco-dados.md             # Schema PostgreSQL
│   ├── 03-frontend.md                # Framework de UI
│   ├── 04-infraestrutura.md          # Auditoria, busca, temporal
│   └── modulos/                      # Specs por módulo
│       ├── _indice.md
│       ├── compras.md
│       └── nfe.md
├── negocios/                         # Fluxos de negócio
│   ├── 01-visao-geral-fluxos.md      # Visão geral de alto nível
│   ├── 02-fluxos-estoque.md          # Análise profunda de estoque
│   ├── 03-fluxos-entrega-nfe.md      # Entrega, NFe, Financeiro
│   └── 04-fluxos-cadastros.md        # Dados mestres, Permissões
├── estrategia/                       # Estratégia de migração
│   ├── 01-plano-migracao.md          # Fases
│   ├── 02-decisoes.md                # ADRs
│   ├── 03-melhorias.md
│   ├── 04-simplificacao-l1l2.md
│   ├── 05-correcao-fifo.md
│   ├── 06-normalizacao-fornecedor.md
│   ├── 07-esquema-redesenhado.md
│   └── 08-design-greenfield.md
└── meta/
    └── rastreador.md                 # Este arquivo
```

---

## Matriz de Cobertura de Fluxos

**Todos os 17 fluxos de negócio documentados (100% de cobertura)**

| # | Fluxo | Documento | Status |
|---|-------|-----------|--------|
| 1 | Cadastros (Fornecedores/Produtos/Clientes) | negocios/04-fluxos-cadastros.md | ✅ Feito |
| 2 | Orçamento (Criação/precificação) | negocios/04-fluxos-cadastros.md | ✅ Feito |
| 3 | Orçamento → Venda | negocios/01-visao-geral-fluxos.md | ✅ Feito |
| 4 | Venda → Compra | negocios/01-visao-geral-fluxos.md | ✅ Feito |
| 5 | Confirmação de Compra (Importação NFe) | negocios/02-fluxos-estoque.md | ✅ Feito |
| 6 | Criação de Estoque (a partir de NFe) | negocios/02-fluxos-estoque.md | ✅ Feito |
| 7 | Consumo de Estoque | negocios/02-fluxos-estoque.md | ✅ Feito |
| 8 | Divisões de Estoque (parear, dividir) | negocios/02-fluxos-estoque.md | ✅ Feito |
| 9 | Emissão de NFe (Saída - para cliente) | negocios/03-fluxos-entrega-nfe.md | ✅ Feito |
| 10 | Entrega (para cliente) | negocios/03-fluxos-entrega-nfe.md | ✅ Feito |
| 11 | Financeiro - Recebíveis | negocios/01-visao-geral-fluxos.md | ✅ Feito |
| 12 | Financeiro - Pagáveis | negocios/03-fluxos-entrega-nfe.md | ✅ Feito |
| 13 | Financeiro - CNAB/Banco | negocios/03-fluxos-entrega-nfe.md | ✅ Feito |
| 14 | Cálculo de Comissão | negocios/03-fluxos-entrega-nfe.md | ✅ Feito |
| 15 | Devoluções | negocios/02-fluxos-estoque.md | ✅ Feito |
| 16 | Galpão (Blocos de armazém) | negocios/04-fluxos-cadastros.md | ✅ Feito |
| 17 | Permissões de Usuário | negocios/04-fluxos-cadastros.md | ✅ Feito |

---

## Documentação Técnica

| Documento | Propósito | Status |
|-----------|-----------|--------|
| tecnico/01-arquitetura.md | Estrutura Laravel, serviços | Rascunho |
| tecnico/02-banco-dados.md | Schema PostgreSQL | Rascunho |
| tecnico/03-frontend.md | Avaliação de framework frontend | Rascunho |
| tecnico/04-infraestrutura.md | Auditoria, temporal, busca | Rascunho |
| tecnico/modulos/_indice.md | Prioridade dos módulos | Rascunho |
| tecnico/modulos/compras.md | Módulo de Compras | Rascunho |
| tecnico/modulos/nfe.md | Integração NFe | Rascunho |

---

## Documentação de Estratégia

| Documento | Propósito | Status |
|-----------|-----------|--------|
| estrategia/00-comparativo-legado-novo.md | Comparativo consolidado | Completo |
| estrategia/01-plano-migracao.md | Fases Strangler Fig | Rascunho |
| estrategia/02-decisoes.md | Registros de Decisão de Arquitetura | Rascunho |
| estrategia/03-melhorias.md | Pontos problemáticos e melhorias | Rascunho |
| estrategia/04-simplificacao-l1l2.md | Simplificação de tabelas L1/L2 | Rascunho |
| estrategia/05-correcao-fifo.md | Correção do consumo FIFO | Rascunho |
| estrategia/06-normalizacao-fornecedor.md | Normalização de FK fornecedor | Rascunho |
| estrategia/07-esquema-redesenhado.md | Schema redesenhado completo | Rascunho |
| estrategia/08-design-greenfield.md | Design greenfield | Rascunho |

---

## Log de Progresso

### 2025-12-28

- [x] Traduzidos nomes de pastas e arquivos para português
- [x] Criado documento comparativo consolidado (00-comparativo-legado-novo.md)

### 2025-12-27

- [x] Criada documentação inicial (série 00-12)
- [x] Auditoria de cobertura completa - 17/17 fluxos documentados
- [x] Reorganizado em estrutura de pastas categóricas
- [x] Criado índice principal (00-indice.md)
- [x] Extraídos conceitos de infraestrutura para tecnico/04-infraestrutura.md

---

## Documentação Completa

**Lógica de Negócio**: 100% documentada
**Design Técnico**: Rascunho completo, decisões pendentes
**Estratégia de Migração**: Rascunho completo, revisão de stakeholders necessária

# Especificações de Migração de Módulos

> Esta pasta contém especificações detalhadas de migração para cada módulo de negócio.
> Última atualização: 2025-12-28

---

## Prioridade dos Módulos

| Prioridade | Módulo     | Complexidade | Dependências       |
| ---------- | ---------- | ------------ | ------------------ |
| 1          | Cadastros  | Baixa        | Nenhuma            |
| 2          | Compras    | Média        | Cadastros          |
| 3          | Estoque    | Média        | Compras            |
| 4          | Financeiro | Média        | Compras, Vendas    |
| 5          | Vendas     | Alta         | Cadastros, Estoque |
| 6          | NFe        | Alta         | Vendas, Compras    |
| 7          | Logistica  | Média        | Vendas             |
| 8          | Relatorios | Média        | Todos              |

---

## Arquivos dos Módulos

| Arquivo                            | Módulo                 | Status   |
| ---------------------------------- | ---------------------- | -------- |
| [cadastros.md](./cadastros.md)     | Cadastros (CRUD base)  | Rascunho |
| [compras.md](./compras.md)         | Gestão de Compras      | Rascunho |
| [estoque.md](./estoque.md)         | Gestão de Estoque      | Rascunho |
| [financeiro.md](./financeiro.md)   | Gestão Financeira      | Rascunho |
| [vendas.md](./vendas.md)           | Gestão de Vendas       | Rascunho |
| [nfe.md](./nfe.md)                 | Nota Fiscal Eletrônica | Rascunho |
| [logistica.md](./logistica.md)     | Logística e Entregas   | Rascunho |
| [relatorios.md](./relatorios.md)   | Relatórios e Dashboards| Rascunho |

---

## Ordem de Migração Sugerida

### Fase 1: Fundação

1. **Cadastros** (Cliente, Fornecedor, Produto, Transportadora)
   - Operações CRUD simples
   - Estabelece padrões para outros módulos
   - Baixo risco

### Fase 2: Transações Principais

1. **Compras** - Fluxo de pedidos de compra
2. **Estoque** - Recebimento e rastreamento de estoque
3. **Financeiro** - Contas a pagar/receber

### Fase 3: Vendas e Conformidade

1. **Vendas** - Fluxo de vendas (mais complexo)
2. **NFe** - Integração com nota fiscal eletrônica

### Fase 4: Módulos de Suporte

1. **Logistica** - Calendário de entregas e agendamento
2. **Relatorios** - Relatórios e dashboards (pode ser feito incrementalmente)

---

## Documentos Relacionados

- [../01-arquitetura.md](../01-arquitetura.md) - Arquitetura geral Laravel
- [../../01-contexto/01-visao-geral-fluxos.md](../../01-contexto/01-visao-geral-fluxos.md) - Fluxos de negocio
- [../../05-execucao/01-plano-migracao.md](../../05-execucao/01-plano-migracao.md) - Plano de migracao

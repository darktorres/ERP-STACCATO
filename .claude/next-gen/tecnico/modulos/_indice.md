# Especificações de Migração de Módulos

> Esta pasta contém especificações detalhadas de migração para cada módulo de negócio.

---

## Prioridade dos Módulos

| Prioridade | Módulo | Complexidade | Dependências |
|------------|--------|--------------|--------------|
| 1 | Cadastros | Baixa | Nenhuma |
| 2 | Compras | Média | Cadastros |
| 3 | Estoque | Média | Compras |
| 4 | Financeiro | Média | Compras, Vendas |
| 5 | Vendas | Alta | Cadastros, Estoque |
| 6 | NFe | Alta | Vendas, Compras |
| 7 | Logistica | Média | Vendas |
| 8 | Galpao | Baixa | Estoque |
| 9 | RH | Baixa | Cadastros |
| 10 | Relatorios | Média | Todos |

---

## Arquivos dos Módulos

| Arquivo | Módulo | Status |
|---------|--------|--------|
| [compras.md](./compras.md) | Gestão de Compras | Rascunho |
| [estoque.md](./estoque.md) | Gestão de Estoque | Rascunho |
| [financeiro.md](./financeiro.md) | Gestão Financeira | Rascunho |
| [nfe.md](./nfe.md) | Nota Fiscal Eletrônica | Rascunho |
| [vendas.md](./vendas.md) | Gestão de Vendas | Pendente |
| [logistica.md](./logistica.md) | Logística | Pendente |
| [galpao.md](./galpao.md) | Galpão | Pendente |

---

## Ordem de Migração Sugerida

### Fase 1: Fundação
1. **Cadastros** (Cliente, Fornecedor, Produto, Transportadora)
   - Operações CRUD simples
   - Estabelece padrões para outros módulos
   - Baixo risco

### Fase 2: Transações Principais
2. **Compras** - Fluxo de pedidos de compra
3. **Estoque** - Recebimento e rastreamento de estoque
4. **Financeiro** - Contas a pagar/receber

### Fase 3: Vendas e Conformidade
5. **Vendas** - Fluxo de vendas (mais complexo)
6. **NFe** - Integração com nota fiscal eletrônica

### Fase 4: Módulos de Suporte
7. **Logistica** - Calendário de entregas e agendamento
8. **Galpao** - Gestão de blocos do galpão
9. **RH** - Folha de pagamento e funcionários
10. **Relatorios** - Relatórios (pode ser feito incrementalmente)

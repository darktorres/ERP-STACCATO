# Module Migration Specs

> This folder contains detailed migration specifications for each business module.

---

## Module Priority

| Priority | Module | Complexity | Dependencies |
|----------|--------|------------|--------------|
| 1 | Cadastros | Low | None |
| 2 | Compras | Medium | Cadastros |
| 3 | Estoque | Medium | Compras |
| 4 | Financeiro | Medium | Compras, Vendas |
| 5 | Vendas | High | Cadastros, Estoque |
| 6 | NFe | High | Vendas, Compras |
| 7 | Logistica | Medium | Vendas |
| 8 | Galpao | Low | Estoque |
| 9 | RH | Low | Cadastros |
| 10 | Relatorios | Medium | All |

---

## Module Files

| File | Module | Status |
|------|--------|--------|
| [compras.md](./compras.md) | Purchase Management | Draft |
| [estoque.md](./estoque.md) | Inventory Management | Draft |
| [financeiro.md](./financeiro.md) | Financial Management | Draft |
| [nfe.md](./nfe.md) | Electronic Invoice | Draft |
| [vendas.md](./vendas.md) | Sales Management | Pending |
| [logistica.md](./logistica.md) | Logistics | Pending |
| [galpao.md](./galpao.md) | Warehouse | Pending |

---

## Suggested Migration Order

### Phase 1: Foundation
1. **Cadastros** (Cliente, Fornecedor, Produto, Transportadora)
   - Simple CRUD operations
   - Establishes patterns for other modules
   - Low risk

### Phase 2: Core Transactions
2. **Compras** - Purchase order workflow
3. **Estoque** - Inventory receiving and tracking
4. **Financeiro** - Accounts payable/receivable

### Phase 3: Sales & Compliance
5. **Vendas** - Sales workflow (most complex)
6. **NFe** - Electronic invoice integration

### Phase 4: Supporting Modules
7. **Logistica** - Delivery calendar and scheduling
8. **Galpao** - Warehouse block management
9. **RH** - Payroll and employees
10. **Relatorios** - Reporting (can be done incrementally)

# Rastreador de Documentação

> Status: **Completo**
> Última atualização: 2025-12-28

---

## Propósito

Acompanhar o progresso da documentação de todos os fluxos de negócio para o projeto de migração web.

---

## Estrutura da Documentação

Reorganizado em 2025-12-27 em pastas categóricas. Atualizado em 2025-12-28 com todos os documentos.

```text
.claude/next-gen/
├── 00-indice.md                      # Navegação principal
│
├── estrategia/                       # Estratégia de migração
│   ├── 00-comparativo-legado-novo.md # Comparativo legado vs novo
│   ├── 01-plano-migracao.md          # Fases Strangler Fig
│   ├── 02-decisoes.md                # ADRs
│   ├── 03-melhorias.md               # Pontos problemáticos
│   ├── 04-simplificacao-l1l2.md      # Achatamento L1/L2
│   ├── 05-correcao-fifo.md           # Correção FIFO
│   ├── 06-normalizacao-fornecedor.md # FK fornecedor
│   ├── 07-esquema-redesenhado.md     # Schema completo
│   ├── 08-design-greenfield.md       # Design greenfield
│   ├── 09-migracao-dados.md          # Migração de dados
│   ├── 10-paridade-funcionalidades.md# Checklist paridade
│   └── 11-treinamento.md             # Treinamento e rollout
│
├── negocios/                         # Fluxos de negócio
│   ├── 01-visao-geral-fluxos.md      # Visão geral
│   ├── 02-fluxos-estoque.md          # Estoque
│   ├── 03-fluxos-entrega-nfe.md      # Entrega, NFe, Financeiro
│   ├── 04-fluxos-cadastros.md        # Dados mestres
│   └── 05-regras-negocio.md          # Regras detalhadas
│
├── tecnico/                          # Arquitetura técnica
│   ├── 01-arquitetura.md             # Design Laravel
│   ├── 02-banco-dados.md             # Schema PostgreSQL
│   ├── 03-frontend.md                # Framework UI
│   ├── 04-infraestrutura.md          # Auditoria, busca, cache
│   ├── 05-seguranca.md               # Autenticação, RBAC
│   ├── 06-api.md                     # API REST
│   ├── 07-testes.md                  # Testes
│   ├── 08-erros-monitoramento.md     # Erros, logging
│   ├── 09-integracoes.md             # ACBr, CNAB, etc
│   ├── 10-design-system.md           # Design system
│   ├── 11-concorrencia.md            # Locks, transações
│   ├── 12-atalhos-teclado.md         # Keyboard shortcuts
│   ├── 13-impressao.md               # PDF, Excel, etiquetas
│   ├── 14-devops.md                  # Docker, CI/CD
│   ├── 15-dicionario-dados.md        # Glossário
│   ├── 16-compatibilidade.md         # Browsers, dispositivos
│   ├── 17-validacao.md               # Validação multicamada
│   ├── 18-dependencias.md            # Auditoria dependências
│   └── modulos/                      # Specs por módulo
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
    ├── backlog-documentacao.md       # Backlog de melhorias
    ├── rastreador.md                 # Este arquivo
    └── reorganizacao-docs.md         # Tracking reorganização
```

---

## Matriz de Cobertura de Fluxos

Todos os 17 fluxos de negócio estão documentados (100% de cobertura).

| #   | Fluxo                                      | Documento                         | Status   |
| --- | ------------------------------------------ | --------------------------------- | -------- |
| 1   | Cadastros (Fornecedores/Produtos/Clientes) | negocios/04-fluxos-cadastros.md   | ✅ Feito |
| 2   | Orçamento (Criação/precificação)           | negocios/04-fluxos-cadastros.md   | ✅ Feito |
| 3   | Orçamento → Venda                          | negocios/01-visao-geral-fluxos.md | ✅ Feito |
| 4   | Venda → Compra                             | negocios/01-visao-geral-fluxos.md | ✅ Feito |
| 5   | Confirmação de Compra (Importação NFe)     | negocios/02-fluxos-estoque.md     | ✅ Feito |
| 6   | Criação de Estoque (a partir de NFe)       | negocios/02-fluxos-estoque.md     | ✅ Feito |
| 7   | Consumo de Estoque                         | negocios/02-fluxos-estoque.md     | ✅ Feito |
| 8   | Divisões de Estoque (parear, dividir)      | negocios/02-fluxos-estoque.md     | ✅ Feito |
| 9   | Emissão de NFe (Saída - para cliente)      | negocios/03-fluxos-entrega-nfe.md | ✅ Feito |
| 10  | Entrega (para cliente)                     | negocios/03-fluxos-entrega-nfe.md | ✅ Feito |
| 11  | Financeiro - Recebíveis                    | negocios/01-visao-geral-fluxos.md | ✅ Feito |
| 12  | Financeiro - Pagáveis                      | negocios/03-fluxos-entrega-nfe.md | ✅ Feito |
| 13  | Financeiro - CNAB/Banco                    | negocios/03-fluxos-entrega-nfe.md | ✅ Feito |
| 14  | Cálculo de Comissão                        | negocios/03-fluxos-entrega-nfe.md | ✅ Feito |
| 15  | Devoluções                                 | negocios/02-fluxos-estoque.md     | ✅ Feito |
| 16  | Galpão (Blocos de armazém)                 | negocios/04-fluxos-cadastros.md   | ✅ Feito |
| 17  | Permissões de Usuário                      | negocios/04-fluxos-cadastros.md   | ✅ Feito |

---

## Documentação Técnica

| Documento                       | Propósito                         | Status   |
| ------------------------------- | --------------------------------- | -------- |
| tecnico/01-arquitetura.md       | Estrutura Laravel, serviços       | Rascunho |
| tecnico/02-banco-dados.md       | Schema PostgreSQL                 | Rascunho |
| tecnico/03-frontend.md          | Framework frontend                | Rascunho |
| tecnico/04-infraestrutura.md    | Auditoria, temporal, busca, perf  | Rascunho |
| tecnico/05-seguranca.md         | Autenticação, autorização         | Rascunho |
| tecnico/06-api.md               | Design API REST                   | Rascunho |
| tecnico/07-testes.md            | Estratégia de testes              | Rascunho |
| tecnico/08-erros-monitoramento.md| Erros, logging, Sentry           | Rascunho |
| tecnico/09-integracoes.md       | ACBr, CNAB, CEP, etc              | Rascunho |
| tecnico/10-design-system.md     | Design system, componentes        | Rascunho |
| tecnico/11-concorrencia.md      | Locks, transações                 | Rascunho |
| tecnico/12-atalhos-teclado.md   | Atalhos de teclado                | Rascunho |
| tecnico/13-impressao.md         | PDF, Excel, etiquetas             | Rascunho |
| tecnico/14-devops.md            | Docker, CI/CD, deploy             | Rascunho |
| tecnico/15-dicionario-dados.md  | Glossário, convenções             | Rascunho |
| tecnico/16-compatibilidade.md   | Browsers, dispositivos            | Rascunho |
| tecnico/17-validacao.md         | Validação multicamada             | Rascunho |
| tecnico/18-dependencias.md      | Auditoria dependências            | Rascunho |

### Módulos

| Documento                       | Propósito                         | Status   |
| ------------------------------- | --------------------------------- | -------- |
| tecnico/modulos/_indice.md      | Prioridade dos módulos            | Rascunho |
| tecnico/modulos/cadastros.md    | Módulo de Cadastros               | Rascunho |
| tecnico/modulos/compras.md      | Módulo de Compras                 | Rascunho |
| tecnico/modulos/estoque.md      | Módulo de Estoque                 | Rascunho |
| tecnico/modulos/financeiro.md   | Módulo Financeiro                 | Rascunho |
| tecnico/modulos/logistica.md    | Módulo de Logística               | Rascunho |
| tecnico/modulos/nfe.md          | Módulo NFe                        | Rascunho |
| tecnico/modulos/relatorios.md   | Módulo de Relatórios              | Rascunho |
| tecnico/modulos/vendas.md       | Módulo de Vendas                  | Rascunho |

---

## Documentação de Estratégia

| Documento                                | Propósito                           | Status   |
| ---------------------------------------- | ----------------------------------- | -------- |
| estrategia/00-comparativo-legado-novo.md | Comparativo consolidado             | Completo |
| estrategia/01-plano-migracao.md          | Fases Strangler Fig                 | Rascunho |
| estrategia/02-decisoes.md                | Registros de Decisão de Arquitetura | Rascunho |
| estrategia/03-melhorias.md               | Pontos problemáticos e melhorias    | Rascunho |
| estrategia/04-simplificacao-l1l2.md      | Simplificação de tabelas L1/L2      | Rascunho |
| estrategia/05-correcao-fifo.md           | Correção do consumo FIFO            | Rascunho |
| estrategia/06-normalizacao-fornecedor.md | Normalização de FK fornecedor       | Rascunho |
| estrategia/07-esquema-redesenhado.md     | Schema redesenhado completo         | Rascunho |
| estrategia/08-design-greenfield.md       | Design greenfield                   | Rascunho |
| estrategia/09-migracao-dados.md          | Migração de dados                   | Rascunho |
| estrategia/10-paridade-funcionalidades.md| Checklist paridade funcional        | Rascunho |
| estrategia/11-treinamento.md             | Treinamento e rollout               | Rascunho |

---

## Log de Progresso

### 2025-12-28 (Tarde)

- [x] Auditoria completa de organização dos 47 arquivos
- [x] Identificada numeração duplicada (17-validacao + 17-dependencias)
- [x] Renumerado 17-dependencias.md → 18-dependencias.md
- [x] Atualizado 00-indice.md com estrutura completa
- [x] Atualizado este rastreador com todos os documentos
- [x] Corrigido tecnico/modulos/_indice.md

### 2025-12-28 (Manhã)

- [x] Traduzidos nomes de pastas e arquivos para português
- [x] Criado documento comparativo consolidado (00-comparativo-legado-novo.md)
- [x] Atualizado módulo NFe: ACBrMonitorConsole, sped-nfe, removido SaaS
- [x] Adicionado módulo DFe (download de NFe) para servidor
- [x] Adicionada seção de tarefas agendadas em infraestrutura
- [x] Criado backlog de documentação (meta/backlog-documentacao.md)
- [x] Fechado ADR-004: Integração NFe (ACBrMonitorConsole ou sped-nfe)
- [x] Corrigidos links quebrados em 8 arquivos (nomes antigos em inglês)
- [x] Completado backlog de documentação (22 itens)
- [x] Criados documentos técnicos 05-18
- [x] Criados módulos vendas, estoque, financeiro, logistica, cadastros, relatorios
- [x] Criado negocios/05-regras-negocio.md
- [x] Criados documentos de estratégia 09-11

### 2025-12-27

- [x] Criada documentação inicial (série 00-12)
- [x] Auditoria de cobertura completa - 17/17 fluxos documentados
- [x] Reorganizado em estrutura de pastas categóricas
- [x] Criado índice principal (00-indice.md)
- [x] Extraídos conceitos de infraestrutura para tecnico/04-infraestrutura.md

---

## Resumo

| Categoria | Arquivos | Status |
|-----------|----------|--------|
| Negócios | 5 | 100% documentado |
| Técnico | 18 + 9 módulos | Rascunho completo |
| Estratégia | 12 | Rascunho completo |
| Meta | 3 | Atualizado |
| **Total** | **47** | **Estrutura completa** |

# Dicionário de Dados

> Status: **Aprovado**
> Última atualização: 2025-12-28

---

## Visão Geral

Este documento define o glossário de termos de negócio, convenções de nomenclatura e significado dos campos utilizados no ERP Staccato.

---

## Glossário de Termos de Negócio

### Termos Gerais

| Termo | Significado | Contexto |
|-------|-------------|----------|
| **ERP** | Enterprise Resource Planning | Sistema de gestão empresarial |
| **Staccato** | Nome da empresa/sistema | Comércio de materiais de construção |
| **Loja** | Unidade de negócio física | Multi-loja (3 filiais) |

### Vendas e Comercial

| Termo | Significado | Detalhes |
|-------|-------------|----------|
| **Orçamento** | Proposta comercial | Precede a venda, tem validade |
| **Venda** | Transação confirmada | Orçamento convertido em pedido |
| **RT** | Representante Técnico | Profissional que indica clientes (arquiteto, designer) |
| **Comissão RT** | Comissão do representante | Percentual sobre venda indicada |
| **Desconto L1** | Desconto nível 1 | Desconto padrão do vendedor |
| **Desconto L2** | Desconto nível 2 | Desconto adicional (gerente) |
| **Desconto L3** | Desconto nível 3 | Desconto máximo (direção) |
| **Preço Cheio** | Preço sem desconto | Preço de tabela |
| **Preço Líquido** | Preço final | Após todos os descontos |
| **Frete FOB** | Free On Board | Cliente retira ou paga frete |
| **Frete CIF** | Cost, Insurance, Freight | Frete incluso no preço |

### Estoque e Logística

| Termo | Significado | Detalhes |
|-------|-------------|----------|
| **Galpão** | Armazém/depósito | Local de armazenamento de mercadorias |
| **Bloco** | Subdivisão do galpão | Área física identificada (A1, B2, etc.) |
| **Lote** | Unidade de estoque | Conjunto de produtos da mesma compra |
| **FIFO** | First In, First Out | Método de consumo de estoque |
| **Posição de Estoque** | Quantidade disponível | Por produto/loja/fornecedor |
| **Reserva** | Estoque comprometido | Reservado para venda pendente |
| **Consumo** | Baixa de estoque | Efetivação da venda |
| **Pallet** | Unidade de carga | Plataforma de transporte |
| **Coleta** | Retirada de mercadoria | Busca no fornecedor |
| **Entrega** | Envio ao cliente | Logística de última milha |

### Financeiro

| Termo | Significado | Detalhes |
|-------|-------------|----------|
| **CNAB** | Centro Nacional de Automação Bancária | Arquivo de troca bancária |
| **CNAB 240** | Versão do layout | 240 posições por linha |
| **Remessa** | Arquivo de envio | Boletos para cobrança |
| **Retorno** | Arquivo de resposta | Baixas e rejeições |
| **Conta a Receber** | Crédito a receber | Parcela de venda |
| **Conta a Pagar** | Débito a pagar | Parcela de compra |
| **Crédito Cliente** | Saldo positivo | Valor disponível para uso |
| **Baixa** | Quitação | Pagamento/recebimento efetivado |
| **Conciliação** | Verificação | Comparação banco vs sistema |

### Fiscal

| Termo | Significado | Detalhes |
|-------|-------------|----------|
| **NFe** | Nota Fiscal Eletrônica | Documento fiscal de saída |
| **NF-e** | Mesmo que NFe | Grafias alternativas |
| **DANFE** | Documento Auxiliar da NFe | Versão impressa da NFe |
| **CC-e** | Carta de Correção Eletrônica | Correção de NFe emitida |
| **MDF-e** | Manifesto de Documentos Fiscais | Transporte de cargas |
| **SEFAZ** | Secretaria da Fazenda | Órgão autorizador |
| **NCM** | Nomenclatura Comum do Mercosul | Classificação de produtos |
| **CFOP** | Código Fiscal de Operações | Natureza da operação |
| **CST** | Código de Situação Tributária | Regime tributário |
| **ICMS** | Imposto sobre Circulação | Imposto estadual |
| **ICMS-ST** | Substituição Tributária | ICMS antecipado |
| **IPI** | Imposto sobre Produtos Industrializados | Imposto federal |
| **PIS** | Programa de Integração Social | Contribuição federal |
| **COFINS** | Contribuição para Financiamento | Contribuição federal |
| **IBPT** | Instituto Brasileiro de Planejamento Tributário | Carga tributária |

### Cadastros

| Termo | Significado | Detalhes |
|-------|-------------|----------|
| **Razão Social** | Nome legal | Registro CNPJ/CPF |
| **Nome Fantasia** | Nome comercial | Nome de divulgação |
| **CPF** | Cadastro de Pessoa Física | Documento pessoa física |
| **CNPJ** | Cadastro Nacional de Pessoa Jurídica | Documento pessoa jurídica |
| **IE** | Inscrição Estadual | Registro estadual |
| **IM** | Inscrição Municipal | Registro municipal |
| **Transportadora** | Empresa de transporte | Entrega de mercadorias |

---

## Enumerações (Enums)

### Status de Orçamento

| Valor | Label | Descrição |
|-------|-------|-----------|
| `ATIVO` | Ativo | Em andamento, aguardando decisão |
| `FECHADO` | Fechado | Convertido em venda |
| `EXPIRADO` | Expirado | Passou da validade |
| `CANCELADO` | Cancelado | Cancelado manualmente |

### Status de Venda

| Valor | Label | Descrição |
|-------|-------|-----------|
| `PENDENTE` | Pendente | Aguardando pagamento |
| `EM_ANDAMENTO` | Em Andamento | Pagamento parcial ou processando |
| `FINALIZADA` | Finalizada | Paga e entregue |
| `CANCELADA` | Cancelada | Venda cancelada |
| `DEVOLVIDA` | Devolvida | Devolução total |

### Status de NFe

| Valor | Label | Descrição |
|-------|-------|-----------|
| `PENDENTE` | Pendente | Aguardando emissão |
| `PROCESSANDO` | Processando | Enviada para SEFAZ |
| `AUTORIZADA` | Autorizada | Aprovada pela SEFAZ |
| `CANCELADA` | Cancelada | Cancelada na SEFAZ |
| `DENEGADA` | Denegada | Rejeitada pela SEFAZ |
| `INUTILIZADA` | Inutilizada | Número inutilizado |

### Status de Parcela

| Valor | Label | Descrição |
|-------|-------|-----------|
| `PENDENTE` | Pendente | Aguardando pagamento |
| `PAGO` | Pago | Quitado |
| `VENCIDO` | Vencido | Não pago no prazo |
| `CANCELADO` | Cancelado | Parcela cancelada |

### Status de Entrega

| Valor | Label | Descrição |
|-------|-------|-----------|
| `AGENDADA` | Agendada | Data definida |
| `EM_ROTA` | Em Rota | Saiu para entrega |
| `ENTREGUE` | Entregue | Confirmada |
| `NAO_ENTREGUE` | Não Entregue | Tentativa frustrada |
| `CANCELADA` | Cancelada | Entrega cancelada |

### Tipo de Endereço

| Valor | Label | Descrição |
|-------|-------|-----------|
| `FISCAL` | Fiscal | Endereço de nota fiscal |
| `ENTREGA` | Entrega | Endereço de entrega |
| `COBRANCA` | Cobrança | Endereço de cobrança |

### Forma de Pagamento

| Valor | Label | Descrição |
|-------|-------|-----------|
| `DINHEIRO` | Dinheiro | Pagamento em espécie |
| `CARTAO_CREDITO` | Cartão de Crédito | Pagamento com cartão |
| `CARTAO_DEBITO` | Cartão de Débito | Pagamento com débito |
| `BOLETO` | Boleto Bancário | Pagamento via boleto |
| `PIX` | PIX | Transferência instantânea |
| `TRANSFERENCIA` | Transferência | TED/DOC |
| `CHEQUE` | Cheque | Pagamento com cheque |
| `CREDITO_CLIENTE` | Crédito do Cliente | Uso de saldo |

---

## Unidades de Medida

### Unidades Padrão

| Código | Nome | Uso |
|--------|------|-----|
| `UN` | Unidade | Peças individuais |
| `CX` | Caixa | Embalagem com múltiplos |
| `M2` | Metro Quadrado | Pisos, revestimentos |
| `M` | Metro Linear | Perfis, rodapés |
| `KG` | Quilograma | Peso |
| `L` | Litro | Volume |
| `PC` | Peça | Similar a unidade |
| `CJ` | Conjunto | Kit/combo |
| `RL` | Rolo | Materiais em rolo |

### Conversões

| De | Para | Fator | Exemplo |
|----|------|-------|---------|
| CX | M2 | Variável | 1 CX = 2.5 M2 (depende do produto) |
| CX | UN | Variável | 1 CX = 12 UN (depende do produto) |

---

## Convenções de Nomenclatura

### Banco de Dados (PostgreSQL)

| Elemento | Convenção | Exemplo |
|----------|-----------|---------|
| Tabelas | snake_case, plural | `vendas`, `venda_itens` |
| Colunas | snake_case | `data_emissao`, `valor_total` |
| Primary Keys | `id` | `id` |
| Foreign Keys | `{tabela}_id` | `cliente_id`, `venda_id` |
| Timestamps | `created_at`, `updated_at` | Laravel padrão |
| Soft Delete | `deleted_at` | Laravel padrão |
| Booleanos | `is_*` ou `has_*` | `is_active`, `has_discount` |

### Código PHP (Laravel)

| Elemento | Convenção | Exemplo |
|----------|-----------|---------|
| Models | PascalCase, singular | `Venda`, `VendaItem` |
| Controllers | PascalCase + Controller | `VendaController` |
| Requests | PascalCase + Request | `StoreVendaRequest` |
| Resources | PascalCase + Resource | `VendaResource` |
| Enums | PascalCase | `StatusVenda` |
| Traits | PascalCase | `HasOptimisticLocking` |

### Código TypeScript/Vue

| Elemento | Convenção | Exemplo |
|----------|-----------|---------|
| Componentes | PascalCase | `VendaForm.vue` |
| Composables | camelCase com `use` | `useVendas.ts` |
| Stores | camelCase | `vendaStore.ts` |
| Types/Interfaces | PascalCase | `Venda`, `VendaFormData` |
| Props | camelCase | `vendaId`, `isLoading` |

---

## Tabelas Principais

### clientes

| Coluna | Tipo | Descrição |
|--------|------|-----------|
| `id` | SERIAL | Identificador único |
| `tipo_pessoa` | CHAR(1) | 'F' = Física, 'J' = Jurídica |
| `cpf_cnpj` | VARCHAR(14) | CPF ou CNPJ sem máscara |
| `razao_social` | VARCHAR(200) | Nome/Razão Social |
| `nome_fantasia` | VARCHAR(200) | Nome Fantasia (PJ) |
| `ie` | VARCHAR(20) | Inscrição Estadual |
| `email` | VARCHAR(100) | Email principal |
| `telefone` | VARCHAR(20) | Telefone principal |
| `credito` | DECIMAL(15,2) | Saldo de crédito |
| `limite_credito` | DECIMAL(15,2) | Limite de crédito |
| `created_at` | TIMESTAMP | Data de cadastro |
| `updated_at` | TIMESTAMP | Última atualização |
| `deleted_at` | TIMESTAMP | Soft delete |

### produtos

| Coluna | Tipo | Descrição |
|--------|------|-----------|
| `id` | SERIAL | Identificador único |
| `codigo` | VARCHAR(50) | Código interno |
| `cod_comercial` | VARCHAR(50) | Código do fornecedor |
| `descricao` | VARCHAR(500) | Descrição completa |
| `unidade` | VARCHAR(10) | Unidade de medida |
| `ncm` | VARCHAR(8) | Código NCM |
| `fornecedor_id` | INTEGER | FK para fornecedores |
| `preco_custo` | DECIMAL(15,4) | Preço de custo |
| `preco_venda` | DECIMAL(15,4) | Preço de venda |
| `margem` | DECIMAL(5,2) | Margem de lucro % |
| `peso` | DECIMAL(10,4) | Peso em KG |
| `m2_caixa` | DECIMAL(10,4) | M2 por caixa |
| `pecas_caixa` | INTEGER | Peças por caixa |
| `ativo` | BOOLEAN | Produto ativo |
| `search_vector` | TSVECTOR | Índice de busca |

### vendas

| Coluna | Tipo | Descrição |
|--------|------|-----------|
| `id` | SERIAL | Identificador único |
| `orcamento_id` | INTEGER | FK para orçamento origem |
| `cliente_id` | INTEGER | FK para cliente |
| `vendedor_id` | INTEGER | FK para usuário vendedor |
| `loja_id` | INTEGER | FK para loja |
| `profissional_id` | INTEGER | FK para profissional RT |
| `endereco_entrega_id` | INTEGER | FK para endereço |
| `status` | VARCHAR(20) | Enum de status |
| `subtotal` | DECIMAL(15,2) | Soma dos itens |
| `desconto` | DECIMAL(15,2) | Desconto total |
| `frete` | DECIMAL(15,2) | Valor do frete |
| `total` | DECIMAL(15,2) | Valor final |
| `observacao` | TEXT | Observações |
| `created_at` | TIMESTAMP | Data da venda |
| `updated_at` | TIMESTAMP | Última atualização |

### estoques

| Coluna | Tipo | Descrição |
|--------|------|-----------|
| `id` | SERIAL | Identificador único |
| `produto_id` | INTEGER | FK para produto |
| `fornecedor_id` | INTEGER | FK para fornecedor |
| `loja_id` | INTEGER | FK para loja |
| `nfe_entrada_id` | INTEGER | FK para NFe de entrada |
| `quantidade_original` | DECIMAL(15,4) | Quantidade inicial |
| `quantidade_disponivel` | DECIMAL(15,4) | Quantidade atual |
| `custo_unitario` | DECIMAL(15,4) | Custo unitário |
| `data_entrada` | TIMESTAMP | Data de entrada |
| `lote` | VARCHAR(50) | Número do lote |
| `bloco_id` | INTEGER | FK para bloco do galpão |
| `localizacao` | VARCHAR(50) | Posição no bloco |

### nfe

| Coluna | Tipo | Descrição |
|--------|------|-----------|
| `id` | SERIAL | Identificador único |
| `venda_id` | INTEGER | FK para venda (saída) |
| `tipo` | CHAR(1) | '0' = Entrada, '1' = Saída |
| `numero` | INTEGER | Número da nota |
| `serie` | VARCHAR(3) | Série da nota |
| `chave_acesso` | CHAR(44) | Chave de acesso NFe |
| `status` | VARCHAR(20) | Enum de status |
| `xml` | TEXT | XML completo |
| `protocolo` | VARCHAR(50) | Protocolo SEFAZ |
| `data_emissao` | TIMESTAMP | Data de emissão |
| `data_autorizacao` | TIMESTAMP | Data de autorização |
| `valor_total` | DECIMAL(15,2) | Valor total da nota |

---

## Campos Calculados vs Armazenados

### Campos Armazenados

| Campo | Tabela | Motivo |
|-------|--------|--------|
| `total` | vendas | Performance, histórico |
| `subtotal` | vendas | Performance |
| `custo_medio` | produtos | Atualizado em cada compra |
| `quantidade_disponivel` | estoques | Atualizado em consumo |

### Campos Calculados (On-the-fly)

| Campo | Cálculo | Uso |
|-------|---------|-----|
| `idade` | NOW() - data_nascimento | Exibição |
| `dias_vencido` | NOW() - data_vencimento | Relatórios |
| `markup` | (preco_venda - preco_custo) / preco_custo | Análise |

### Views Materializadas

| View | Dados | Refresh |
|------|-------|---------|
| `immv_produto_estoque` | Estoque por produto | Automático (pg_ivm) |
| `immv_vendas_dashboard` | Totais de vendas | Automático (pg_ivm) |
| `mv_ranking_produtos` | Ranking de vendas | Diário |

---

## Campos Legados vs Novos

### Campos Renomeados

| Legado (MySQL) | Novo (PostgreSQL) | Motivo |
|----------------|-------------------|--------|
| `idCliente` | `cliente_id` | Convenção Laravel |
| `idVenda` | `venda_id` | Convenção Laravel |
| `dataCriacao` | `created_at` | Padrão Laravel |
| `dataAlteracao` | `updated_at` | Padrão Laravel |
| `fornecedor` | `fornecedor_id` | Normalização (era VARCHAR) |

### Campos Removidos

| Campo | Tabela | Motivo |
|-------|--------|--------|
| `ui` | vendas | Flag temporária de UI |
| `temp_*` | várias | Campos temporários |
| `_bak` | várias | Colunas de backup |

### Campos Adicionados

| Campo | Tabela | Propósito |
|-------|--------|-----------|
| `deleted_at` | todas | Soft delete |
| `uuid` | públicas | Identificador externo |
| `search_vector` | produtos, clientes | Full-text search |
| `updated_at` | todas | Optimistic locking |

---

## Relacionamentos Principais

```
clientes
├── enderecos (1:N)
├── profissionais (N:M via cliente_profissional)
└── vendas (1:N)

vendas
├── cliente (N:1)
├── vendedor (N:1)
├── loja (N:1)
├── profissional (N:1, opcional)
├── itens (1:N)
├── parcelas (1:N)
├── entregas (1:N)
└── nfe (1:N)

produtos
├── fornecedor (N:1)
├── ncm (N:1)
├── estoques (1:N)
└── precos (1:N, por loja)

estoques
├── produto (N:1)
├── fornecedor (N:1)
├── loja (N:1)
├── nfe_entrada (N:1)
└── bloco (N:1)
```

---

## Documentos Relacionados

- [02-banco-dados.md](./02-banco-dados.md) - Schema completo do banco
- [04-infraestrutura.md](./04-infraestrutura.md) - Views materializadas
- [../estrategia/09-migracao-dados.md](../estrategia/09-migracao-dados.md) - Mapeamento de migração

# Checklist de Paridade de Funcionalidades

> Status: **Rascunho**
> Última atualização: 2025-12-28
> Prioridade: Alta

---

## Visão Geral

Este documento mapeia todas as funcionalidades do sistema C++ atual para o novo sistema Laravel, garantindo que nenhuma funcionalidade crítica seja perdida durante a migração.

### Legenda de Status

| Status | Descrição |
|--------|-----------|
| ⬜ | Não iniciado |
| 🔄 | Em progresso |
| ✅ | Completo |
| ❌ | Descartado (com justificativa) |
| 🆕 | Novo (não existe no C++) |

### Resumo por Módulo

| Módulo | Total | ⬜ | 🔄 | ✅ | ❌ |
|--------|-------|-----|-----|-----|-----|
| Orçamentos | 14 | 14 | 0 | 0 | 0 |
| Vendas | 16 | 16 | 0 | 0 | 0 |
| Compras | 18 | 18 | 0 | 0 | 0 |
| Estoque | 8 | 8 | 0 | 0 | 0 |
| Galpão | 10 | 10 | 0 | 0 | 0 |
| Financeiro | 20 | 20 | 0 | 0 | 0 |
| NFe | 16 | 16 | 0 | 0 | 0 |
| Logística | 22 | 22 | 0 | 0 | 0 |
| Relatórios | 6 | 6 | 0 | 0 | 0 |
| Cadastros | 12 | 12 | 0 | 0 | 0 |
| Utilitários | 10 | 10 | 0 | 0 | 0 |
| **Total** | **152** | **152** | **0** | **0** | **0** |

---

## 1. Orçamentos (Quotations)

**Arquivos C++**: `widgetorcamento.cpp`, `orcamento.cpp`

| # | Funcionalidade | Status | Arquivo Laravel | Observações |
|---|----------------|--------|-----------------|-------------|
| 1.1 | Criar novo orçamento | ⬜ | `OrcamentoController@store` | |
| 1.2 | Adicionar itens ao orçamento | ⬜ | `OrcamentoItemController` | |
| 1.3 | Calcular preços com frete automático | ⬜ | `FreteService` | Regras complexas |
| 1.4 | Desconto global e por item | ⬜ | `DescontoService` | 3 níveis de desconto |
| 1.5 | Calcular peso e quantidade de caixas | ⬜ | `CalculoPesoService` | |
| 1.6 | Replicar orçamento | ⬜ | `OrcamentoService@replicar` | |
| 1.7 | Exportar para Excel | ⬜ | `OrcamentoExportController` | |
| 1.8 | Exportar para PDF | ⬜ | `OrcamentoExportController` | |
| 1.9 | Frete manual (exceções) | ⬜ | `FreteService` | |
| 1.10 | Gestão de follow-up | ⬜ | `FollowupController` | |
| 1.11 | Converter orçamento em venda | ⬜ | `OrcamentoService@converter` | Fluxo crítico |
| 1.12 | Verificar disponibilidade em tempo real | ⬜ | `EstoqueService@disponibilidade` | |
| 1.13 | Atribuir consultor profissional | ⬜ | `OrcamentoController` | Campo profissional_id |
| 1.14 | Verificar serviços especiais do produto | ⬜ | `ProdutoService` | |

---

## 2. Vendas (Sales)

**Arquivos C++**: `widgetvenda.cpp`, `venda.cpp`, `widgetdevolucao.cpp`

| # | Funcionalidade | Status | Arquivo Laravel | Observações |
|---|----------------|--------|-----------------|-------------|
| 2.1 | Criar venda a partir de orçamento | ⬜ | `VendaService@criarDeOrcamento` | |
| 2.2 | Gerenciar fluxo de pagamento | ⬜ | `PagamentoController` | |
| 2.3 | Definir método e parcelas de pagamento | ⬜ | `ParcelamentoService` | |
| 2.4 | Gerar comissões de profissionais | ⬜ | `ComissaoService` | Cálculo RT |
| 2.5 | Criar registros de consumo | ⬜ | `ConsumoService` | FIFO |
| 2.6 | Gerenciar endereços de entrega | ⬜ | `EnderecoController` | |
| 2.7 | Cancelar venda com rollback | ⬜ | `VendaService@cancelar` | Estorna estoque/financeiro |
| 2.8 | Processar devoluções | ⬜ | `DevolucaoController` | Fluxo complexo |
| 2.9 | Gerar PDF da venda | ⬜ | `VendaExportController` | |
| 2.10 | Imprimir comprovante de entrega | ⬜ | `ComprovantePdfController` | |
| 2.11 | Visualizar comprovantes | ⬜ | `ComprovanteController@show` | |
| 2.12 | Rastrear status da venda | ⬜ | `VendaStatusService` | Enum de status |
| 2.13 | Integração com financeiro | ⬜ | `ContasReceberService` | Auto-gerar parcelas |
| 2.14 | Marcar RT (Responsável Técnico) | ⬜ | `VendaController@marcarRT` | |
| 2.15 | Gerenciar pontuação/recompensas | ⬜ | `PontuacaoService` | |
| 2.16 | Dividir venda (devolução parcial) | ⬜ | `DevolucaoService@dividir` | |

---

## 3. Compras (Purchases)

**Arquivos C++**: `tabcompras.cpp`, `widgetcompra*.cpp`, `compraavulsa.cpp`

| # | Funcionalidade | Status | Arquivo Laravel | Observações |
|---|----------------|--------|-----------------|-------------|
| 3.1 | Visualizar itens pendentes por fornecedor | ⬜ | `CompraPendenteController` | |
| 3.2 | Filtrar por status e período | ⬜ | `CompraPendenteController@index` | Query params |
| 3.3 | Criar compras avulsas | ⬜ | `CompraAvulsaController` | |
| 3.4 | Gerar relatórios de follow-up | ⬜ | `CompraReportController` | |
| 3.5 | Exportar para Excel/PDF | ⬜ | `CompraExportController` | |
| 3.6 | Gerar pedidos de compra automaticamente | ⬜ | `CompraService@gerar` | |
| 3.7 | Confirmar pedidos de compra | ⬜ | `CompraService@confirmar` | |
| 3.8 | Agendar data de confirmação | ⬜ | `CompraController@agendar` | |
| 3.9 | Cancelar compras | ⬜ | `CompraService@cancelar` | |
| 3.10 | Faturar compras | ⬜ | `CompraService@faturar` | |
| 3.11 | Faturamento de representação | ⬜ | `RepresentacaoService` | |
| 3.12 | Reagendar faturamento | ⬜ | `CompraController@reagendar` | |
| 3.13 | Rastrear consumos de produtos | ⬜ | `ConsumoController` | |
| 3.14 | Desfazer consumos | ⬜ | `ConsumoService@desfazer` | |
| 3.15 | Gerenciar devoluções de compra | ⬜ | `CompraDevolucaoController` | |
| 3.16 | Visualizar histórico de compras | ⬜ | `CompraHistoricoController` | |
| 3.17 | Resumo de estados de compra | ⬜ | `CompraResumoController` | Dashboard |
| 3.18 | Associar NFe a compra | ⬜ | `CompraService@associarNfe` | |

---

## 4. Estoque (Inventory)

**Arquivos C++**: `tabestoque.cpp`, `widgetestoques.cpp`, `widgetestoqueproduto.cpp`

| # | Funcionalidade | Status | Arquivo Laravel | Observações |
|---|----------------|--------|-----------------|-------------|
| 4.1 | Consultar níveis de estoque por localização | ⬜ | `EstoqueController@index` | |
| 4.2 | Filtrar itens ativos/descontinuados | ⬜ | `EstoqueController@index` | Query params |
| 4.3 | Rastrear movimentações de estoque | ⬜ | `MovimentacaoController` | |
| 4.4 | Consultar disponibilidade de produto | ⬜ | `ProdutoController@disponibilidade` | |
| 4.5 | Filtrar por estoque/StaccatoOFF | ⬜ | `EstoqueController` | |
| 4.6 | Rastrear custo de produtos | ⬜ | `CustoProdutoService` | |
| 4.7 | Gerenciar consumo FIFO | ⬜ | `FifoService` | Correção crítica |
| 4.8 | Histórico de movimentações | ⬜ | `MovimentacaoController@historico` | |

---

## 5. Galpão (Warehouse)

**Arquivos C++**: `widgetgalpao.cpp`, `widgetgalpaopeso.cpp`, `palletitem.cpp`, `viewgalpao.cpp`

| # | Funcionalidade | Status | Arquivo Laravel | Observações |
|---|----------------|--------|-----------------|-------------|
| 5.1 | Visualização gráfica de pallets | ⬜ | `GalpaoController@mapa` | Vue + Canvas/SVG |
| 5.2 | Criar pallets com nomes customizados | ⬜ | `PalletController@store` | |
| 5.3 | Mover produtos entre pallets | ⬜ | `PalletService@mover` | |
| 5.4 | Visualizar capacidade de pallets | ⬜ | `PalletController@capacidade` | |
| 5.5 | Agendar chegadas de transporte | ⬜ | `TransporteController@agendar` | |
| 5.6 | Imprimir mapa de pallets | ⬜ | `GalpaoPdfController` | |
| 5.7 | Editar atribuições de pallets | ⬜ | `PalletController@update` | |
| 5.8 | Remover pallets | ⬜ | `PalletController@destroy` | |
| 5.9 | Buscar conteúdo de pallets | ⬜ | `PalletController@search` | |
| 5.10 | Rastrear transportes agendados | ⬜ | `TransporteController@index` | |

---

## 6. Financeiro (Financial)

**Arquivos C++**: `tabfinanceiro.cpp`, `widgetfinanceiro*.cpp`, `widgetgare.cpp`, `cnab.cpp`

| # | Funcionalidade | Status | Arquivo Laravel | Observações |
|---|----------------|--------|-----------------|-------------|
| 6.1 | Visualizar fluxo de caixa | ⬜ | `FluxoCaixaController` | |
| 6.2 | Análise multi-período | ⬜ | `FluxoCaixaController@analise` | |
| 6.3 | Criar contas a pagar | ⬜ | `ContasPagarController@store` | |
| 6.4 | Rastrear vencimentos (pagar) | ⬜ | `ContasPagarController@vencimentos` | |
| 6.5 | Processar pagamentos | ⬜ | `PagamentoService@pagar` | |
| 6.6 | Estornar pagamentos | ⬜ | `PagamentoService@estornar` | |
| 6.7 | Gerar remessa CNAB 240 (pagar) | ⬜ | `CnabService@gerarRemessa` | Itaú |
| 6.8 | Importar folha de pagamento | ⬜ | `FolhaPagamentoController` | |
| 6.9 | Criar contas a receber | ⬜ | `ContasReceberController@store` | |
| 6.10 | Rastrear vencimentos (receber) | ⬜ | `ContasReceberController@vencimentos` | |
| 6.11 | Antecipar recebíveis | ⬜ | `AntecipacaoService` | Cálculo de juros |
| 6.12 | Gerar remessa CNAB 240 (receber) | ⬜ | `CnabService@gerarRemessa` | Boletos |
| 6.13 | Visualizar valores em atraso | ⬜ | `ContasReceberController@atraso` | |
| 6.14 | Processar retorno CNAB | ⬜ | `CnabService@processarRetorno` | Baixa automática |
| 6.15 | Gerenciar GARE (impostos) | ⬜ | `GareController` | |
| 6.16 | Gerar arquivo CNAB para GARE | ⬜ | `GareService@gerarCnab` | Itaú 240 |
| 6.17 | Inserir lançamento manual | ⬜ | `LancamentoController@store` | |
| 6.18 | Transferência entre contas | ⬜ | `TransferenciaController` | |
| 6.19 | Exportar para Excel | ⬜ | `FinanceiroExportController` | |
| 6.20 | Conciliação bancária | ⬜ | `ConciliacaoController` | |

---

## 7. NFe (Electronic Invoicing)

**Arquivos C++**: `tabnfe.cpp`, `widgetnfe*.cpp`, `cadastrarnfe.cpp`, `importarxml.cpp`

| # | Funcionalidade | Status | Arquivo Laravel | Observações |
|---|----------------|--------|-----------------|-------------|
| 7.1 | Receber notas de entrada | ⬜ | `NfeEntradaController` | |
| 7.2 | Marcar como utilizada/não utilizada | ⬜ | `NfeEntradaController@marcar` | |
| 7.3 | Inutilizar NFe | ⬜ | `NfeService@inutilizar` | SEFAZ |
| 7.4 | Emitir NFe de saída | ⬜ | `NfeService@emitir` | ACBr/sped-nfe |
| 7.5 | Cancelar NFe emitida | ⬜ | `NfeService@cancelar` | SEFAZ |
| 7.6 | Consultar status na SEFAZ | ⬜ | `NfeService@consultar` | |
| 7.7 | Exportar NFes | ⬜ | `NfeExportController` | |
| 7.8 | Imprimir DANFE | ⬜ | `DanfePdfController` | |
| 7.9 | Download automático SEFAZ (distribuição) | ⬜ | `NfeDistribuicaoService` | NSU |
| 7.10 | Manifestar eventos (confirmação, ciência) | ⬜ | `NfeEventoService` | |
| 7.11 | Processar notas recebidas | ⬜ | `NfeDistribuicaoController` | |
| 7.12 | Importar XML de NFe | ⬜ | `ImportarXmlController` | |
| 7.13 | Matching de produtos no XML | ⬜ | `ImportarXmlService@match` | |
| 7.14 | Tratar divergências (preço, qtd) | ⬜ | `ImportarXmlService` | |
| 7.15 | Criar consumo automático | ⬜ | `ImportarXmlService@consumir` | |
| 7.16 | Criar pagamento (GARE se aplicável) | ⬜ | `ImportarXmlService@criarPagamento` | |

---

## 8. Logística (Logistics)

**Arquivos C++**: `tablogistica.cpp`, `widgetlogistica*.cpp`

### 8.1 Coleta (Pickup)

| # | Funcionalidade | Status | Arquivo Laravel | Observações |
|---|----------------|--------|-----------------|-------------|
| 8.1.1 | Agendar coleta de fornecedor | ⬜ | `ColetaController@agendar` | |
| 8.1.2 | Atribuir veículo de coleta | ⬜ | `ColetaController@atribuirVeiculo` | |
| 8.1.3 | Calcular peso de transporte | ⬜ | `PesoService` | |
| 8.1.4 | Adicionar/remover produtos da coleta | ⬜ | `ColetaItemController` | |
| 8.1.5 | Reagendar coleta | ⬜ | `ColetaController@reagendar` | |
| 8.1.6 | Confirmar recebimento | ⬜ | `ColetaController@confirmar` | |
| 8.1.7 | Rastrear coletas recebidas | ⬜ | `ColetaController@recebidas` | |

### 8.2 Entrega (Delivery)

| # | Funcionalidade | Status | Arquivo Laravel | Observações |
|---|----------------|--------|-----------------|-------------|
| 8.2.1 | Agendar entregas de vendas | ⬜ | `EntregaController@agendar` | |
| 8.2.2 | Atribuir veículos | ⬜ | `EntregaController@atribuirVeiculo` | |
| 8.2.3 | Gerenciar locais e datas de entrega | ⬜ | `EntregaController` | |
| 8.2.4 | Calcular quantidades disponíveis | ⬜ | `EntregaService@disponibilidade` | |
| 8.2.5 | Entregas parciais | ⬜ | `EntregaService@parcial` | |
| 8.2.6 | Gerar NFe futuras | ⬜ | `EntregaService@gerarNfeFutura` | |
| 8.2.7 | Importar NFe existente no agendamento | ⬜ | `EntregaController@importarNfe` | |
| 8.2.8 | Visualização de mapa | ⬜ | `EntregaController@mapa` | Integração maps |
| 8.2.9 | Reagendar entregas | ⬜ | `EntregaController@reagendar` | |
| 8.2.10 | Calendário de entregas | ⬜ | `CalendarioController` | Vue Calendar |
| 8.2.11 | Confirmar entrega com comprovante | ⬜ | `EntregaController@confirmar` | Upload foto |
| 8.2.12 | Imprimir protocolo de entrega | ⬜ | `EntregaPdfController@protocolo` | |
| 8.2.13 | Imprimir checklist de entrega | ⬜ | `EntregaPdfController@checklist` | |
| 8.2.14 | Cancelar entregas | ⬜ | `EntregaController@cancelar` | |
| 8.2.15 | Visualizar entregas concluídas | ⬜ | `EntregaController@concluidas` | |

### 8.3 Veículos e Representação

| # | Funcionalidade | Status | Arquivo Laravel | Observações |
|---|----------------|--------|-----------------|-------------|
| 8.3.1 | Gerenciar frota de caminhões | ⬜ | `VeiculoController` | |
| 8.3.2 | Rastrear atribuições de veículos | ⬜ | `VeiculoController@atribuicoes` | |
| 8.3.3 | Ativar/desativar veículos | ⬜ | `VeiculoController@toggleAtivo` | |
| 8.3.4 | Entregas de representação (terceiros) | ⬜ | `RepresentacaoEntregaController` | |

---

## 9. Relatórios (Reports)

**Arquivos C++**: `widgetrelatorio.cpp`, modelos LimeReport

| # | Funcionalidade | Status | Arquivo Laravel | Observações |
|---|----------------|--------|-----------------|-------------|
| 9.1 | Gerar relatórios de vendas | ⬜ | `RelatorioVendaController` | |
| 9.2 | Filtrar por vendedor e loja | ⬜ | `RelatorioController` | Query params |
| 9.3 | Calcular totais por loja | ⬜ | `RelatorioService` | |
| 9.4 | Calcular totais por vendedor | ⬜ | `RelatorioService` | |
| 9.5 | Exportar para Excel | ⬜ | `RelatorioExportController` | Laravel Excel |
| 9.6 | Relatórios de comissão | ⬜ | `ComissaoReportController` | |

---

## 10. Cadastros (Master Data)

**Arquivos C++**: `cadastro*.cpp`

| # | Funcionalidade | Status | Arquivo Laravel | Observações |
|---|----------------|--------|-----------------|-------------|
| 10.1 | CRUD de Clientes | ⬜ | `ClienteController` | |
| 10.2 | Gerenciar endereços de cliente | ⬜ | `ClienteEnderecoController` | |
| 10.3 | CRUD de Fornecedores | ⬜ | `FornecedorController` | |
| 10.4 | CRUD de Produtos | ⬜ | `ProdutoController` | |
| 10.5 | Associar NCM a produtos | ⬜ | `ProdutoController` | |
| 10.6 | Gerenciar preços e promoções | ⬜ | `PromocaoController` | |
| 10.7 | CRUD de Profissionais/Vendedores | ⬜ | `ProfissionalController` | |
| 10.8 | CRUD de Transportadoras | ⬜ | `TransportadoraController` | |
| 10.9 | CRUD de Funcionários | ⬜ | `FuncionarioController` | |
| 10.10 | CRUD de Usuários | ⬜ | `UsuarioController` | |
| 10.11 | CRUD de Lojas | ⬜ | `LojaController` | |
| 10.12 | CRUD de NCMs | ⬜ | `NcmController` | |

---

## 11. Utilitários e Sistema

**Arquivos C++**: `mainwindow.cpp`, `application.cpp`, `user.cpp`, diversos

| # | Funcionalidade | Status | Arquivo Laravel | Observações |
|---|----------------|--------|-----------------|-------------|
| 11.1 | Login e autenticação | ⬜ | `AuthController` | Sanctum + Fortify |
| 11.2 | Gerenciamento de permissões | ⬜ | `PermissaoController` | Spatie Permission |
| 11.3 | Importar produtos de Excel | ⬜ | `ImportarProdutoController` | |
| 11.4 | Importar tabela IBPT | ⬜ | `ImportarIbptController` | |
| 11.5 | Calculadora de frete | ⬜ | `CalculoFreteController` | |
| 11.6 | Verificação de consistência de dados | ⬜ | `ConsistenciaController` | Admin only |
| 11.7 | Configurações do sistema | ⬜ | `ConfiguracaoController` | |
| 11.8 | Gráficos de performance | ⬜ | `GraficoController` | Chart.js |
| 11.9 | Gerenciar dados bancários | ⬜ | `DadosBancarioController` | |
| 11.10 | Tema claro/escuro | ⬜ | Frontend only | Tailwind dark mode |

---

## 12. Funcionalidades Novas (não existem no C++)

| # | Funcionalidade | Justificativa | Arquivo Laravel |
|---|----------------|---------------|-----------------|
| 🆕 12.1 | API REST para integrações | Permitir integrações futuras | `Api\*Controller` |
| 🆕 12.2 | Notificações em tempo real | Melhor UX que polling | Laravel Reverb |
| 🆕 12.3 | Auditoria completa (activity log) | LGPD e compliance | spatie/activitylog |
| 🆕 12.4 | 2FA (autenticação dois fatores) | Segurança | Laravel Fortify |
| 🆕 12.5 | Exportação de dados LGPD | Compliance | `LgpdController` |
| 🆕 12.6 | Dashboard responsivo | Acesso mobile/tablet | Vue + Tailwind |
| 🆕 12.7 | Busca global unificada | Melhor UX | `BuscaController` |
| 🆕 12.8 | Webhooks para eventos | Integrações | `WebhookController` |

---

## 13. Funcionalidades Descartadas

| # | Funcionalidade | Justificativa |
|---|----------------|---------------|
| ❌ 13.1 | *Nenhuma até o momento* | |

---

## Critérios de Aceitação por Módulo

### Orçamentos

- [ ] Usuário pode criar orçamento com todos os campos do C++
- [ ] Cálculo de frete automático produz mesmos valores
- [ ] Descontos calculados corretamente nos 3 níveis
- [ ] Conversão para venda mantém todos os dados
- [ ] Exportação PDF tem mesmo layout

### Vendas

- [ ] Fluxo completo orçamento → venda → entrega funciona
- [ ] Cancelamento estorna estoque e financeiro
- [ ] Devoluções parciais funcionam
- [ ] Comissões calculadas corretamente

### Compras

- [ ] Geração automática de pedidos funciona
- [ ] Fluxo pendente → confirmado → faturado → recebido
- [ ] Associação com NFe de entrada
- [ ] Consumos rastreados corretamente

### Estoque

- [ ] FIFO funciona corretamente (correção do bug)
- [ ] Níveis de estoque em tempo real
- [ ] Movimentações auditadas

### Financeiro

- [ ] CNAB 240 Itaú gera arquivo válido
- [ ] Retorno CNAB processa baixas automaticamente
- [ ] Fluxo de caixa mostra projeção correta
- [ ] GARE processa impostos corretamente

### NFe

- [ ] Emissão via ACBr/sped-nfe funciona
- [ ] Cancelamento dentro do prazo
- [ ] Importação de XML com matching de produtos
- [ ] DANFE imprime corretamente

### Logística

- [ ] Agendamento de coletas e entregas
- [ ] Calendário visual funciona
- [ ] Confirmação com comprovante
- [ ] Entregas parciais

---

## Processo de Validação

### Por Funcionalidade

1. Implementar no Laravel
2. Testar com dados reais (cópia do banco de produção)
3. Comparar resultado com sistema C++
4. Validar com usuário final
5. Marcar como ✅

### Testes de Regressão

- Executar suite de testes automatizados
- Comparar relatórios gerados
- Validar cálculos financeiros
- Verificar integridade de dados

---

## Documentos Relacionados

- [01-plano-migracao.md](./01-plano-migracao.md) - Plano de fases
- [09-migracao-dados.md](./09-migracao-dados.md) - Scripts de migração
- [../tecnico/modulos/](../tecnico/modulos/) - Specs de cada módulo

#include "widgetfinanceirocontas.h"
#include "ui_widgetfinanceirocontas.h"

#include "acbrlib.h"
#include "anteciparrecebimento.h"
#include "application.h"
#include "contas.h"
#include "inserirlancamento.h"
#include "inserirtransferencia.h"
#include "permissao.h"
#include "reaisdelegate.h"
#include "sql.h"
#include "sqlquery.h"
#include "user.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QScrollBar>
#include <QSet>
#include <QSqlError>
#include <QSqlRecord>

WidgetFinanceiroContas::WidgetFinanceiroContas(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetFinanceiroContas) {
  ui->setupUi(this);
  ui->pushButtonImportarFolhaPag->hide();
}

WidgetFinanceiroContas::~WidgetFinanceiroContas() { delete ui; }

void WidgetFinanceiroContas::setupTables() {
  if (tipo == Tipo::Nulo) { throw RuntimeException("Erro Tipo::Nulo!", this); }

  if (tipo == Tipo::Receber) { modelVencidos.setQuery(Sql::view_a_receber_vencidos()); }
  if (tipo == Tipo::Pagar) { modelVencidos.setQuery(Sql::view_a_pagar_vencidos()); }

  modelVencidos.sort("`Data`");

  ui->tableVencidos->setModel(&modelVencidos);

  ui->tableVencidos->setItemDelegate(new ReaisDelegate(this));

  // -------------------------------------------------------------------------

  if (tipo == Tipo::Receber) { modelVencer.setQuery(Sql::view_a_receber_vencer()); }
  if (tipo == Tipo::Pagar) { modelVencer.setQuery(Sql::view_a_pagar_vencer()); }

  modelVencer.sort("`Data`");

  ui->tableVencer->setModel(&modelVencer);

  ui->tableVencer->setItemDelegate(new ReaisDelegate(this));
}

void WidgetFinanceiroContas::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->dateEditRealizadoAte, &QDateEdit::dateChanged, this, &WidgetFinanceiroContas::montaFiltro, connectionType);
  connect(ui->dateEditRealizadoDe, &QDateEdit::dateChanged, this, &WidgetFinanceiroContas::on_dateEditRealizadoDe_dateChanged, connectionType);
  connect(ui->dateEditVencimentoAte, &QDateEdit::dateChanged, this, &WidgetFinanceiroContas::montaFiltro, connectionType);
  connect(ui->dateEditVencimentoDe, &QDateEdit::dateChanged, this, &WidgetFinanceiroContas::on_dateEditVencimentoDe_dateChanged, connectionType);
  connect(ui->doubleSpinBoxAte, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &WidgetFinanceiroContas::montaFiltro, connectionType);
  connect(ui->doubleSpinBoxDe, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &WidgetFinanceiroContas::on_doubleSpinBoxDe_valueChanged, connectionType);
  connect(ui->groupBoxLojas, &QGroupBox::toggled, this, &WidgetFinanceiroContas::montaFiltro, connectionType);
  connect(ui->groupBoxRealizado, &QGroupBox::toggled, this, &WidgetFinanceiroContas::montaFiltro, connectionType);
  connect(ui->groupBoxRealizado, &QGroupBox::toggled, this, &WidgetFinanceiroContas::on_groupBoxRealizado_toggled, connectionType);
  connect(ui->groupBoxVencimento, &QGroupBox::toggled, this, &WidgetFinanceiroContas::montaFiltro, connectionType);
  connect(ui->groupBoxVencimento, &QGroupBox::toggled, this, &WidgetFinanceiroContas::on_groupBoxVencimento_toggled, connectionType);
  connect(ui->itemBoxLojas, &ItemBox::textChanged, this, &WidgetFinanceiroContas::montaFiltro, connectionType);
  connect(ui->lineEditBusca, &LineEdit::delayedTextChanged, this, &WidgetFinanceiroContas::montaFiltro, connectionType);
  connect(ui->pushButtonAbrirDANFE, &QPushButton::clicked, this, &WidgetFinanceiroContas::on_pushButtonAbrirDANFE_clicked, connectionType);
  connect(ui->pushButtonAdiantarRecebimento, &QPushButton::clicked, this, &WidgetFinanceiroContas::on_pushButtonAdiantarRecebimento_clicked, connectionType);
  connect(ui->pushButtonExcluirLancamento, &QPushButton::clicked, this, &WidgetFinanceiroContas::on_pushButtonExcluirLancamento_clicked, connectionType);
  connect(ui->pushButtonImportarFolhaPag, &QPushButton::clicked, this, &WidgetFinanceiroContas::on_pushButtonImportarFolhaPag_clicked, connectionType);
  connect(ui->pushButtonInserirLancamento, &QPushButton::clicked, this, &WidgetFinanceiroContas::on_pushButtonInserirLancamento_clicked, connectionType);
  connect(ui->pushButtonInserirTransferencia, &QPushButton::clicked, this, &WidgetFinanceiroContas::on_pushButtonInserirTransferencia_clicked, connectionType);
  connect(ui->pushButtonRemessaItau, &QPushButton::clicked, this, &WidgetFinanceiroContas::on_pushButtonRemessaItau_clicked, connectionType);
  connect(ui->pushButtonReverterPagamento, &QPushButton::clicked, this, &WidgetFinanceiroContas::on_pushButtonReverterPagamento_clicked, connectionType);
  connect(ui->radioButtonAgendado, &QRadioButton::clicked, this, &WidgetFinanceiroContas::montaFiltro, connectionType);
  connect(ui->radioButtonCancelado, &QRadioButton::clicked, this, &WidgetFinanceiroContas::montaFiltro, connectionType);
  connect(ui->radioButtonConferido, &QRadioButton::clicked, this, &WidgetFinanceiroContas::montaFiltro, connectionType);
  connect(ui->radioButtonPago, &QRadioButton::clicked, this, &WidgetFinanceiroContas::montaFiltro, connectionType);
  connect(ui->radioButtonPendente, &QRadioButton::clicked, this, &WidgetFinanceiroContas::montaFiltro, connectionType);
  connect(ui->radioButtonRecebido, &QRadioButton::clicked, this, &WidgetFinanceiroContas::montaFiltro, connectionType);
  connect(ui->radioButtonTodos, &QRadioButton::clicked, this, &WidgetFinanceiroContas::montaFiltro, connectionType);
  connect(ui->table, &TableView::activated, this, &WidgetFinanceiroContas::on_table_activated, connectionType);
  connect(ui->table->verticalScrollBar(), &QScrollBar::valueChanged, this, &WidgetFinanceiroContas::onTableScrolled, connectionType);
  connect(ui->tableVencer, &TableView::doubleClicked, this, &WidgetFinanceiroContas::on_tableVencer_doubleClicked, connectionType);
  connect(ui->tableVencidos, &TableView::doubleClicked, this, &WidgetFinanceiroContas::on_tableVencidos_doubleClicked, connectionType);
  // ui->table->selectionModel() so existe depois do 1o ui->table->setModel(&model), que so acontece
  // dentro de montaFiltro() (chamado depois de setConnections()) - conectar aqui seria conectar a
  // um sender nulo. Feito logo apos o setModel, em montaFiltro().
}

void WidgetFinanceiroContas::onTableScrolled(const int value) {
  if (carregandoPagina) { return; }

  carregandoPagina = true;

  try {
    auto *scrollBar = ui->table->verticalScrollBar();
    const int threshold = ui->table->verticalHeader()->defaultSectionSize() * 5;

    // O QTableView guarda o indice da 1a linha visivel como valor do scrollbar e nao o reancora
    // quando linhas entram/saem ACIMA do viewport. Sem somar o deslocamento, carregar uma pagina faz
    // o conteudo sob o cursor pular ~1000 linhas - e, rolando pra cima, dispara em cascata (o valor
    // continua dentro do threshold, entao o proximo evento carrega outra pagina).
    int deslocamento = 0;

    if (value >= scrollBar->maximum() - threshold) { deslocamento += model.tryLoadNext(); }
    if (value <= threshold) { deslocamento += model.tryLoadPrevious(); }

    if (deslocamento != 0) {
      // O QTableView adia o layout apos inserir/remover linhas (rowCountChanged -> doDelayedItemsLayout),
      // entao a faixa do scrollbar ainda seria a antiga aqui e o setValue poderia ser recortado nela
      ui->table->executarLayoutPendente();
      scrollBar->setValue(scrollBar->value() + deslocamento);
    }
  } catch (...) {
    carregandoPagina = false;
    throw;
  }

  carregandoPagina = false;
}

void WidgetFinanceiroContas::updateTables() {
  if (not isSet) {
    ui->lineEditBusca->setDelayed();

    ui->radioButtonPendente->setChecked(true);

    ui->dateEditRealizadoAte->setDate(qApp->serverDate());
    ui->dateEditRealizadoDe->setDate(qApp->serverDate());
    ui->dateEditVencimentoAte->setDate(qApp->serverDate());
    ui->dateEditVencimentoDe->setDate(qApp->serverDate());

    ui->itemBoxLojas->setSearchDialog(SearchDialog::loja(this));

    setupTables();
    setConnections();
    // Prefixo injetado pelo pai: esta classe serve mais de uma tela (Contas a Pagar e a
    // Receber, Compras e Financeiro), entao a chave nao pode ser hardcoded aqui.
    Permissao::aplicarTela(this, property("prefixoPermissao").toString().isEmpty()
                                     ? QString("financeiro.contasPagar")
                                     : property("prefixoPermissao").toString());
    isSet = true;
  }

  // model e SqlPaginatedModel: nao ha um select()/re-executar-a-ultima-query - montaFiltro()
  // reconstroi os filtros (idempotente, mesmo estado da UI) e recarrega a 1a pagina.
  montaFiltro();

  modelVencidos.select();
  modelVencer.select();

  somarSelecao();
}

void WidgetFinanceiroContas::resetTables() {
  montaFiltro();
  setupTables();
}

void WidgetFinanceiroContas::on_table_activated(const QModelIndex &index) {
  if (tipo == Tipo::Nulo) { throw RuntimeException("Erro Tipo::Nulo!", this); }

  const QString header = model.headerData(index.column(), Qt::Horizontal).toString();

  if (tipo == Tipo::Receber) {
    auto *contas = new Contas(Contas::Tipo::Receber, this);
    contas->setAttribute(Qt::WA_DeleteOnClose);

    if (header == "Id") { contas->viewContaReceberPgt(model.data(index.row(), "idPagamento").toString()); }
    else if (header == "Contraparte") { contas->viewContaReceberContraparte(model.data(index.row(), "contraParte").toString()); }
    else { contas->viewContaReceber(model.data(index.row(), "idPagamento").toString(), model.data(index.row(), "contraParte").toString()); }
  }

  if (tipo == Tipo::Pagar) {
    const QString ordemCompra = model.data(index.row(), "ordemCompra").toString();

    if (header == "O.C." and ordemCompra.isEmpty()) { throw RuntimeError("Sem O.C.!"); }

    auto *contas = new Contas(Contas::Tipo::Pagar, this);
    contas->setAttribute(Qt::WA_DeleteOnClose);

    if (header == "Id") { contas->viewContaPagarPgt(model.data(index.row(), "idPagamento").toString()); }
    else if (header == "Contraparte") { contas->viewContaPagarContraparte(model.data(index.row(), "contraParte").toString()); }
    else if (header == "O.C.") { contas->viewContaPagarOrdemCompra(ordemCompra); }
    else { contas->viewContaPagarData(model.data(index.row(), "dataPagamento").toString(), filtrosContasPagar()); }
  }
}

QString WidgetFinanceiroContas::filtrosContasPagar() const {
  QStringList filtros;

  //-------------------------------------

  QString status;

  const auto children = ui->groupBoxStatus->findChildren<QRadioButton *>(QRegularExpression("radioButton"));

  for (const auto &child : children) {
    if (child->isChecked()) {
      if (child->text() == "Todos") { break; }

      status = child->text();
      break;
    }
  }

  if (not status.isEmpty()) { filtros << "status = '" + status + "'"; }

  //-------------------------------------

  if (not qFuzzyIsNull(ui->doubleSpinBoxDe->value()) or not qFuzzyIsNull(ui->doubleSpinBoxAte->value())) {
    filtros << "valor BETWEEN " + QString::number(ui->doubleSpinBoxDe->value() - 1) + " AND " + QString::number(ui->doubleSpinBoxAte->value() + 1);
  }

  //-------------------------------------

  if (ui->groupBoxRealizado->isChecked()) {
    filtros << "dataRealizado BETWEEN '" + ui->dateEditRealizadoDe->date().toString("yyyy-MM-dd") + "' AND '" + ui->dateEditRealizadoAte->date().toString("yyyy-MM-dd") + "'";
  }

  //-------------------------------------

  if (ui->groupBoxLojas->isChecked() and not ui->itemBoxLojas->text().isEmpty()) { filtros << "idLoja = " + ui->itemBoxLojas->getId().toString(); }

  //-------------------------------------

  return filtros.join(" AND ");
}

void WidgetFinanceiroContas::montaFiltro() {
  if (tipo == Tipo::Nulo) { throw RuntimeException("Erro Tipo::Nulo!", this); }

  if (tipo == Tipo::Pagar) {
    QStringList filtros; // tocam só cp.* — entram na subquery de corte (paginação), antes do JOIN
    QString status;

    const auto children = ui->groupBoxStatus->findChildren<QRadioButton *>(QRegularExpression("radioButton"));

    for (const auto &child : children) {
      if (child->isChecked()) {
        if (child->text() == "Todos") { break; }

        status = child->text();
        break;
      }
    }

    if (not status.isEmpty()) { filtros << "cp.status = '" + status + "'"; }

    //-------------------------------------

    const QString valor = (not qFuzzyIsNull(ui->doubleSpinBoxDe->value()) or not qFuzzyIsNull(ui->doubleSpinBoxAte->value()))
                              ? "cp.valor BETWEEN " + QString::number(ui->doubleSpinBoxDe->value() - 1) + " AND " + QString::number(ui->doubleSpinBoxAte->value() + 1)
                              : "";
    if (not valor.isEmpty()) { filtros << valor; }

    //-------------------------------------

    const QString dataPagamento = ui->groupBoxVencimento->isChecked() ? "cp.dataPagamento BETWEEN '" + ui->dateEditVencimentoDe->date().toString("yyyy-MM-dd") + "' AND '" +
                                                                            ui->dateEditVencimentoAte->date().toString("yyyy-MM-dd") + "'"
                                                                      : "";
    if (not dataPagamento.isEmpty()) { filtros << dataPagamento; }

    //-------------------------------------

    const QString dataRealizado = ui->groupBoxRealizado->isChecked() ? "cp.dataRealizado BETWEEN '" + ui->dateEditRealizadoDe->date().toString("yyyy-MM-dd") + "' AND '" +
                                                                           ui->dateEditRealizadoAte->date().toString("yyyy-MM-dd") + "'"
                                                                     : "";
    if (not dataRealizado.isEmpty()) { filtros << dataRealizado; }

    //-------------------------------------

    const QString loja = (ui->groupBoxLojas->isChecked() and not ui->itemBoxLojas->text().isEmpty()) ? "cp.idLoja = " + ui->itemBoxLojas->getId().toString() : "";
    if (not loja.isEmpty()) { filtros << loja; }

    //------------------------------------- busca: entra na própria subquery de corte
    // As colunas O.C./NF-e/Venda(pf2)/Cód. Forn. só existem via o fan-out cp2->pf2->ehc->e->n, mas
    // não é preciso rodar o fan-out para filtrar: basta perguntar às tabelas filhas quais idPagamento
    // casam, com "IN (subquery)" NÃO correlacionado — o MySQL materializa cada conjunto uma única vez
    // em vez de consultá-lo por linha de cp. Medido no banco real (pior caso: "Todos", sem filtro de
    // Data, termo sem match): ~25 s varrendo janela a janela contra ~1,3 s assim. As mesmas 7 colunas
    // de antes continuam pesquisáveis.

    const QString text = qApp->escaparBusca(ui->lineEditBusca->text());

    if (not text.isEmpty()) {
      // Um ÚNICO "IN" no nível de cp: com dois IN irmãos o MySQL deixa de materializar o segundo e
      // passa a reavaliá-lo por linha (medido 4,9 s contra 1,8 s aninhando o ramo de NF-e dentro do
      // mesmo IN). O termo de numeroNFe entra como mais um OR do pf2, não como um IN separado.
      filtros << "(cp.contraParte LIKE '%" + text + "%' OR cp.idVenda LIKE '%" + text + "%' OR cp.observacao LIKE '%" + text +
                     "%'"
                     " OR cp.idPagamento IN (SELECT cp2.idPagamento FROM conta_a_pagar_has_idcompra cp2"
                     " WHERE cp2.idCompra IN (SELECT pf2.idCompra FROM pedido_fornecedor_has_produto2 pf2"
                     " WHERE pf2.ordemCompra LIKE '%" +
                     text + "%' OR pf2.idVenda LIKE '%" + text + "%' OR pf2.codFornecedor LIKE '%" + text +
                     "%'"
                     " OR pf2.idPedido2 IN (SELECT ehc.idPedido2 FROM estoque_has_compra ehc"
                     " WHERE ehc.idEstoque IN (SELECT e.idEstoque FROM estoque e"
                     " WHERE e.idNFe IN (SELECT n.idNFe FROM nfe n WHERE n.numeroNFe LIKE '%" +
                     text + "%'))))))";
    }

    //------------------------------------- colunas, expressao SQL de cada uma (pra ORDER BY/keyset da paginacao)

    static const QStringList fieldNames = {"idPagamento", "idLoja",       "contraparte", "dataEmissao",      "dataPagamento", "dataRealizado", "idVenda",
                                            "ordemCompra", "numeroNFe",    "idNFe",       "status",           "valor",         "valorReal",     "tipo",
                                            "parcela",     "observacao",   "grupo",       "statusFinanceiro", "pf2_idVenda",   "codFornecedor"};

    static const QHash<QString, QString> exprPorCampo = {
        {"idPagamento", "cp.idPagamento"},     {"idLoja", "cp.idLoja"},               {"contraparte", "cp.contraParte"},   {"dataEmissao", "cp.dataEmissao"},
        {"dataPagamento", "cp.dataPagamento"}, {"dataRealizado", "cp.dataRealizado"}, {"idVenda", "cp.idVenda"},            {"status", "cp.status"},
        {"valor", "cp.valor"},                 {"valorReal", "cp.valorReal"},         {"tipo", "cp.tipo"},                 {"parcela", "cp.parcela"},
        {"observacao", "cp.observacao"},       {"grupo", "cp.grupo"},
        {"ordemCompra", "GROUP_CONCAT(DISTINCT pf2.ordemCompra SEPARATOR ',')"},
        {"numeroNFe", "GROUP_CONCAT(DISTINCT n.numeroNFe SEPARATOR ', ')"},
        {"idNFe", "GROUP_CONCAT(DISTINCT n.idNFe SEPARATOR ', ')"},
        {"statusFinanceiro", "GROUP_CONCAT(DISTINCT pf2.statusFinanceiro SEPARATOR ',')"},
        {"pf2_idVenda", "GROUP_CONCAT(DISTINCT pf2.idVenda SEPARATOR ', ')"},
        {"codFornecedor", "GROUP_CONCAT(DISTINCT pf2.codFornecedor SEPARATOR ', ')"},
    };

    // colunas que só existem via o fan-out cp2->pf2->ehc->e->n: ordenar por elas exige repetir esse
    // join (e agrupar) já na subquery de corte — mais lento, aceitável só nesse clique explícito de
    // cabeçalho (mesma lógica do Cliente/CPF-CNPJ em widgetnfesaida.cpp)
    static const QSet<QString> colunasComFanOut = {"ordemCompra", "numeroNFe", "idNFe", "statusFinanceiro", "pf2_idVenda", "codFornecedor"};

    const QStringList filtrosCopia = filtros;

    const SqlPaginatedModel::QueryBuilderFactory factory = [filtrosCopia](const QString &sortColumn, const Qt::SortOrder order) -> SqlPaginatedModel::PageQueryBuilder {
      const QString sortExpr = exprPorCampo.value(sortColumn, "cp.dataPagamento");
      // nome (sem qualificador de tabela) do mesmo campo, pro ORDER BY de exibicao - que roda sobre o
      // wrapper "SELECT * FROM (...) x" abaixo, onde "cp" ja saiu de escopo (so os apelidos de x valem)
      const QString sortFieldExibicao = exprPorCampo.contains(sortColumn) ? sortColumn : "dataPagamento";
      const bool precisaFanOut = colunasComFanOut.contains(sortColumn);

      return [filtrosCopia, sortExpr, sortFieldExibicao, order, precisaFanOut](const SqlPaginatedModel::PageRequest &request) -> QString {
        const bool forward = request.direction != SqlPaginatedModel::Direction::Previous;

        const QVector<SqlPaginatedModel::KeyExpr> keys = {{sortExpr, order}, {"cp.idPagamento", order}};

        QStringList capFiltros = filtrosCopia;
        QString capHaving;

        if (request.direction != SqlPaginatedModel::Direction::First) {
          const QString keysetCond = SqlPaginatedModel::buildKeysetWhere(keys, request.cursorValues, forward);
          if (precisaFanOut) { capHaving = keysetCond; } // keyset compara uma expressao agregada - so vale em HAVING
          else { capFiltros << keysetCond; }
        }

        const QString capOrderBy = SqlPaginatedModel::buildOrderBy(keys, forward);

        const QString capJoins = precisaFanOut ? " LEFT JOIN conta_a_pagar_has_idcompra cp2 ON cp.idPagamento = cp2.idPagamento"
                                                  " LEFT JOIN pedido_fornecedor_has_produto2 pf2 ON cp2.idCompra = pf2.idCompra"
                                                  " LEFT JOIN estoque_has_compra ehc ON ehc.idPedido2 = pf2.idPedido2"
                                                  " LEFT JOIN estoque e ON ehc.idEstoque = e.idEstoque"
                                                  " LEFT JOIN nfe n ON n.idNFe = e.idNFe"
                                                : "";
        // FORCE INDEX: mesmo risco medido em NFe de o otimizador preferir um indice so de status sem
        // filtro de data (ver db/add_index_nfe_tipo_status_utilizada_data.sql) - so aplica quando a
        // subquery de corte nao precisa do fan-out (que ja força um plano de JOIN proprio)
        const QString forceIndex = precisaFanOut ? "" : " FORCE INDEX (idx_conta_pagar_status_date_valor)";
        const QString capGroupBy = precisaFanOut ? " GROUP BY cp.idPagamento" : "";
        const QString capHavingClause = capHaving.isEmpty() ? "" : " HAVING " + capHaving;

        const QString capSql = "SELECT cp.idPagamento AS idPagamento FROM conta_a_pagar_has_pagamento cp" + forceIndex + capJoins + " WHERE " +
                               (capFiltros.isEmpty() ? "1" : capFiltros.join(" AND ")) + capGroupBy + capHavingClause + " ORDER BY " + capOrderBy + " LIMIT " + QString::number(1000);

        // keys usa "cp." (valido dentro do capSql acima) - o ORDER BY de exibicao roda fora do wrapper
        // "x" (a query de exibicao abaixo), onde so os apelidos sem qualificador existem
        const QVector<SqlPaginatedModel::KeyExpr> keysExibicao = {{sortFieldExibicao, order}, {"idPagamento", order}};
        const QString exibicaoOrderBy = SqlPaginatedModel::buildOrderBy(keysExibicao, true); // exibicao sempre na ordem normal

        return "SELECT * FROM ("
               "SELECT `cp`.`idPagamento` AS `idPagamento`, `cp`.`idLoja` AS `idLoja`, `cp`.`contraParte` AS `contraparte`, `cp`.`dataEmissao` AS `dataEmissao`, "
               "`cp`.`dataPagamento` AS `dataPagamento`, `cp`.`dataRealizado` AS `dataRealizado`, `cp`.`idVenda` AS `idVenda`, "
               "GROUP_CONCAT(DISTINCT `pf2`.`ordemCompra` SEPARATOR ',') AS `ordemCompra`, "
               "GROUP_CONCAT(DISTINCT `n`.`numeroNFe` SEPARATOR ', ') AS `numeroNFe`, "
               "GROUP_CONCAT(DISTINCT `n`.`idNFe` SEPARATOR ', ') AS `idNFe`, "
               "`cp`.`status` AS `status`, `cp`.`valor` AS `valor`, `cp`.`valorReal` AS `valorReal`, `cp`.`tipo` AS `tipo`, `cp`.`parcela` AS `parcela`, "
               "`cp`.`observacao` AS `observacao`, `cp`.`grupo` AS `grupo`, "
               "GROUP_CONCAT(DISTINCT `pf2`.`statusFinanceiro` SEPARATOR ',') AS `statusFinanceiro`, "
               "GROUP_CONCAT(DISTINCT `pf2`.`idVenda` SEPARATOR ', ') AS `pf2_idVenda`, "
               "GROUP_CONCAT(DISTINCT `pf2`.`codFornecedor` SEPARATOR ', ') AS `codFornecedor` "
               "FROM (" +
               capSql +
               ") lim "
               "JOIN conta_a_pagar_has_pagamento cp ON cp.idPagamento = lim.idPagamento "
               "LEFT JOIN conta_a_pagar_has_idcompra cp2 ON cp.idPagamento = cp2.idPagamento "
               "LEFT JOIN pedido_fornecedor_has_produto2 pf2 ON cp2.idCompra = pf2.idCompra "
               "LEFT JOIN estoque_has_compra ehc ON ehc.idPedido2 = pf2.idPedido2 "
               "LEFT JOIN estoque e ON ehc.idEstoque = e.idEstoque "
               // FORCE INDEX: só precisamos de numeroNFe/idNFe, e nfe_tipo_index (idNFe, tipo, numeroNFe)
               // cobre isso. Sem ele o MySQL lê a linha inteira de `nfe` — tabela de ~2,9 GB por causa da
               // coluna xml — medido 450 ms contra 119 ms por janela de 1000 linhas.
               "LEFT JOIN nfe n FORCE INDEX (nfe_tipo_index) ON n.idNFe = e.idNFe "
               "GROUP BY cp.idPagamento"
               ") x ORDER BY " +
               exibicaoOrderBy;
      };
    };

    // Preserva a ordenacao escolhida por clique de cabecalho: montaFiltro() roda a cada digitacao na
    // busca/data/status/loja, e passar a coluna fixa aqui jogava fora a escolha do usuario (a seta do
    // cabecalho continuava nela, mas os dados voltavam pra Vencimento). So volta ao padrao quando o
    // PROPRIO padrao muda - o que distingue "trocou o radio" de "mexeu em outro filtro".
    const QString sortPadrao = ui->radioButtonPago->isChecked() ? "dataRealizado" : "dataPagamento";
    const bool usarPadrao = (sortPadrao != sortPadraoAtual) or model.sortColumn().isEmpty();

    sortPadraoAtual = sortPadrao;

    model.reset(fieldNames, "idPagamento", usarPadrao ? sortPadrao : model.sortColumn(), usarPadrao ? Qt::AscendingOrder : model.sortOrder(), {}, factory);
  }

  if (tipo == Tipo::Receber) {
    QStringList filtros; // tocam só cr.* — entram na subquery de corte (paginação), antes do JOIN
    QString status;

    const auto children = ui->groupBoxFiltros->findChildren<QRadioButton *>(QRegularExpression("radioButton"));

    for (const auto &child : children) {
      if (child->isChecked()) {
        if (child->text() == "Todos") { break; }

        status = child->text();
        break;
      }
    }

    if (not status.isEmpty()) { filtros << "cr.status = '" + status + "'"; }

    //-------------------------------------

    const QString valor = (not qFuzzyIsNull(ui->doubleSpinBoxDe->value()) or not qFuzzyIsNull(ui->doubleSpinBoxAte->value()))
                              ? "cr.valor BETWEEN " + QString::number(ui->doubleSpinBoxDe->value() - 1) + " AND " + QString::number(ui->doubleSpinBoxAte->value() + 1)
                              : "";
    if (not valor.isEmpty()) { filtros << valor; }

    //-------------------------------------

    const QString dataPagamento = ui->groupBoxVencimento->isChecked() ? "cr.dataPagamento BETWEEN '" + ui->dateEditVencimentoDe->date().toString("yyyy-MM-dd") + "' AND '" +
                                                                            ui->dateEditVencimentoAte->date().toString("yyyy-MM-dd") + "'"
                                                                      : "";
    if (not dataPagamento.isEmpty()) { filtros << dataPagamento; }

    //-------------------------------------

    const QString dataRealizado = ui->groupBoxRealizado->isChecked() ? "cr.dataRealizado BETWEEN '" + ui->dateEditRealizadoDe->date().toString("yyyy-MM-dd") + "' AND '" +
                                                                           ui->dateEditRealizadoAte->date().toString("yyyy-MM-dd") + "'"
                                                                     : "";
    if (not dataRealizado.isEmpty()) { filtros << dataRealizado; }

    //-------------------------------------

    const QString loja = (ui->groupBoxLojas->isChecked() and not ui->itemBoxLojas->text().isEmpty()) ? "cr.idLoja = " + ui->itemBoxLojas->getId().toString() : "";
    if (not loja.isEmpty()) { filtros << loja; }

    //-------------------------------------

    filtros << "cr.representacao = FALSE";

    //------------------------------------- busca: entra na própria subquery de corte
    // "ordemRepresentacao" só existe via o fan-out venda->pf2 (1:N), mas não é preciso rodar o
    // fan-out para filtrar: "IN (subquery)" NÃO correlacionado resolve o conjunto de idVenda uma
    // única vez (mesma lógica do Pagar acima). Medido ~0,32 s no pior caso.

    const QString text = qApp->escaparBusca(ui->lineEditBusca->text());

    if (not text.isEmpty()) {
      filtros << "(cr.idVenda LIKE '%" + text + "%' OR cr.contraParte LIKE '%" + text + "%' OR cr.observacao LIKE '%" + text +
                     "%'"
                     " OR cr.idVenda IN (SELECT pf2.idVenda FROM pedido_fornecedor_has_produto2 pf2 WHERE pf2.ordemRepresentacao LIKE '%" +
                     text + "%'))";
    }

    //------------------------------------- colunas, expressao SQL de cada uma (pra ORDER BY/keyset da paginacao)

    static const QStringList fieldNames = {"idPagamento",   "idLoja",  "representacao", "contraparte", "dataEmissao",      "dataPagamento",
                                            "dataRealizado", "idVenda", "ordemRepresentacao", "status", "valor",            "valorReal",
                                            "tipo",          "parcela", "observacao",    "statusFinanceiro"};

    static const QHash<QString, QString> exprPorCampo = {
        {"idPagamento", "cr.idPagamento"},     {"idLoja", "cr.idLoja"},               {"representacao", "cr.representacao"}, {"contraparte", "cr.contraParte"},
        {"dataEmissao", "cr.dataEmissao"},     {"dataPagamento", "cr.dataPagamento"}, {"dataRealizado", "cr.dataRealizado"}, {"idVenda", "cr.idVenda"},
        {"status", "cr.status"},               {"valor", "cr.valor"},                 {"valorReal", "cr.valorReal"},        {"tipo", "cr.tipo"},
        {"parcela", "cr.parcela"},             {"observacao", "cr.observacao"},
        {"ordemRepresentacao", "GROUP_CONCAT(DISTINCT pf2.ordemRepresentacao)"},
        {"statusFinanceiro", "v.statusFinanceiro"},
    };

    // "ordemRepresentacao" só existe via o fan-out venda->pf2 (1:N) — ordenar por ela exige repetir
    // esse join e agrupar já na subquery de corte (mesma lógica do Pagar acima)
    static const QSet<QString> colunasComFanOut = {"ordemRepresentacao"};
    // "statusFinanceiro" só precisa do join simples até venda (1:1 via idVenda, sem fan-out) — mais
    // lento que o caminho rápido mas sem precisar de GROUP BY/HAVING (mesma lógica do Cliente em Saída)
    static const QSet<QString> colunasComJoinVenda = {"statusFinanceiro"};

    // desempate fixo da ordem padrão da tela (idVenda/tipo/parcela) — preserva a ordem hoje observada
    // (so `parcela` de fato desce, ver nota no início do arquivo de plano), independente da coluna
    // escolhida como principal (inclusive por clique de cabeçalho)
    static const QVector<SqlPaginatedModel::KeyExpr> extraKeysFixas = {{"cr.idVenda", Qt::AscendingOrder}, {"cr.tipo", Qt::AscendingOrder}, {"cr.parcela", Qt::DescendingOrder}};
    // mesmo desempate, sem qualificador de tabela - pro ORDER BY de exibicao (roda fora do wrapper "x"
    // abaixo, onde "cr" ja saiu de escopo)
    static const QVector<SqlPaginatedModel::KeyExpr> extraKeysFixasExibicao = {{"idVenda", Qt::AscendingOrder}, {"tipo", Qt::AscendingOrder}, {"parcela", Qt::DescendingOrder}};

    const QStringList filtrosCopia = filtros;
    const SqlPaginatedModel::QueryBuilderFactory factory = [filtrosCopia](const QString &sortColumn, const Qt::SortOrder order) -> SqlPaginatedModel::PageQueryBuilder {
      const QString sortExpr = exprPorCampo.value(sortColumn, "cr.dataPagamento");
      // nome (sem qualificador de tabela) do mesmo campo, pro ORDER BY de exibicao - que roda sobre o
      // wrapper "SELECT * FROM (...) x" abaixo, onde "cr" ja saiu de escopo (so os apelidos de x valem)
      const QString sortFieldExibicao = exprPorCampo.contains(sortColumn) ? sortColumn : "dataPagamento";
      const bool precisaFanOut = colunasComFanOut.contains(sortColumn);
      const bool precisaJoinVenda = precisaFanOut or colunasComJoinVenda.contains(sortColumn);

      return [filtrosCopia, sortExpr, sortFieldExibicao, order, precisaFanOut, precisaJoinVenda](const SqlPaginatedModel::PageRequest &request) -> QString {
        const bool forward = request.direction != SqlPaginatedModel::Direction::Previous;

        QVector<SqlPaginatedModel::KeyExpr> keys;
        keys << SqlPaginatedModel::KeyExpr{sortExpr, order};
        keys += extraKeysFixas;
        keys << SqlPaginatedModel::KeyExpr{"cr.idPagamento", order};

        // mesmo desempate de "keys", sem qualificador de tabela - pro ORDER BY de exibicao (ver
        // nota equivalente no Pagar acima)
        QVector<SqlPaginatedModel::KeyExpr> keysExibicao;
        keysExibicao << SqlPaginatedModel::KeyExpr{sortFieldExibicao, order};
        keysExibicao += extraKeysFixasExibicao;
        keysExibicao << SqlPaginatedModel::KeyExpr{"idPagamento", order};

        QStringList capFiltros = filtrosCopia;
        QString capHaving;

        if (request.direction != SqlPaginatedModel::Direction::First) {
          const QString keysetCond = SqlPaginatedModel::buildKeysetWhere(keys, request.cursorValues, forward);
          if (precisaFanOut) { capHaving = keysetCond; } // keyset compara uma expressao agregada - so vale em HAVING
          else { capFiltros << keysetCond; }
        }

        const QString capOrderBy = SqlPaginatedModel::buildOrderBy(keys, forward);

        const QString capJoinVenda = precisaJoinVenda ? " LEFT JOIN venda v ON cr.idVenda = v.idVenda" : "";
        const QString capJoinPf2 = precisaFanOut ? " LEFT JOIN pedido_fornecedor_has_produto2 pf2 ON v.idVenda = pf2.idVenda" : "";
        // FORCE INDEX: mesmo risco medido em NFe (ver db/add_index_nfe_tipo_status_utilizada_data.sql)
        // - so aplica quando a subquery de corte nao precisa de nenhum join extra
        const QString forceIndex = precisaJoinVenda ? "" : " FORCE INDEX (idx_conta_receber_status_date_rep)";
        const QString capGroupBy = precisaFanOut ? " GROUP BY cr.idPagamento" : "";
        const QString capHavingClause = capHaving.isEmpty() ? "" : " HAVING " + capHaving;

        const QString capSql = "SELECT cr.idPagamento AS idPagamento FROM conta_a_receber_has_pagamento cr" +
                               forceIndex + capJoinVenda + capJoinPf2 + " WHERE " + (capFiltros.isEmpty() ? "1" : capFiltros.join(" AND ")) + capGroupBy + capHavingClause + " ORDER BY " + capOrderBy +
                               " LIMIT " + QString::number(1000);

        const QString exibicaoOrderBy = SqlPaginatedModel::buildOrderBy(keysExibicao, true); // exibicao sempre na ordem normal

        // Envolto em "SELECT * FROM (...) x": a busca referencia nomes crus (idVenda, ordemRepresentacao)
        // que tambem existem como coluna real de pf2 (pf2.idVenda, pf2.ordemRepresentacao) - direto num
        // HAVING dessa mesma query isso e ambiguo pro MySQL; via a tabela derivada x (que so expoe os
        // apelidos já resolvidos) a referencia deixa de ser ambigua. Mesmo mecanismo usado no Pagar acima.
        return "SELECT * FROM ("
               "SELECT `cr`.`idPagamento` AS `idPagamento`, `cr`.`idLoja` AS `idLoja`, `cr`.`representacao` AS `representacao`, `cr`.`contraParte` AS `contraparte`, "
               "`cr`.`dataEmissao` AS `dataEmissao`, `cr`.`dataPagamento` AS `dataPagamento`, `cr`.`dataRealizado` AS `dataRealizado`, `cr`.`idVenda` AS `idVenda`, "
               "GROUP_CONCAT(DISTINCT `pf2`.`ordemRepresentacao`) AS `ordemRepresentacao`, "
               "`cr`.`status` AS `status`, `cr`.`valor` AS `valor`, `cr`.`valorReal` AS `valorReal`, `cr`.`tipo` AS `tipo`, `cr`.`parcela` AS `parcela`, "
               "`cr`.`observacao` AS `observacao`, `v`.`statusFinanceiro` AS `statusFinanceiro` "
               "FROM (" +
               capSql +
               ") lim "
               "JOIN conta_a_receber_has_pagamento cr ON cr.idPagamento = lim.idPagamento "
               "LEFT JOIN venda v ON cr.idVenda = v.idVenda "
               "LEFT JOIN pedido_fornecedor_has_produto2 pf2 ON v.idVenda = pf2.idVenda "
               "GROUP BY cr.idPagamento"
               ") x " +
               "ORDER BY " + exibicaoOrderBy;
      };
    };

    // mesma logica do Pagar acima
    const QString sortPadrao = ui->radioButtonRecebido->isChecked() ? "dataRealizado" : "dataPagamento";
    const bool usarPadrao = (sortPadrao != sortPadraoAtual) or model.sortColumn().isEmpty();

    sortPadraoAtual = sortPadrao;

    model.reset(fieldNames, "idPagamento", usarPadrao ? sortPadrao : model.sortColumn(), usarPadrao ? Qt::AscendingOrder : model.sortOrder(), {"idVenda", "tipo", "parcela"}, factory);
  }

  if (tipo == Tipo::Receber) {
    model.setHeaderLabel("idVenda", "Venda");
    model.setHeaderLabel("ordemRepresentacao", "O.C. Rep.");
  }

  if (tipo == Tipo::Pagar) {
    model.setHeaderLabel("idVenda", "Venda");
    model.setHeaderLabel("pf2_idVenda", "Venda");
    model.setHeaderLabel("ordemCompra", "O.C.");
    model.setHeaderLabel("numeroNFe", "NF-e");
    model.setHeaderLabel("codFornecedor", "Cód. Forn.");
  }

  model.setHeaderLabel("idPagamento", "Id");
  model.setHeaderLabel("contraparte", "Contraparte"); // fieldNames usa "contraparte" (minusculo, casado com o alias SQL) - setHeaderLabel é case-sensitive
  model.setHeaderLabel("dataEmissao", "Emissão");
  model.setHeaderLabel("dataPagamento", "Vencimento");
  model.setHeaderLabel("dataRealizado", "Realizado");
  model.setHeaderLabel("valor", "R$");
  model.setHeaderLabel("valorReal", "R$ Real");
  model.setHeaderLabel("tipo", "Tipo");
  model.setHeaderLabel("parcela", "Parcela");
  model.setHeaderLabel("observacao", "Obs.");
  model.setHeaderLabel("status", "Status");
  model.setHeaderLabel("statusFinanceiro", "Status Financeiro");

  ui->table->setModel(&model);
  // Qt::UniqueConnection: montaFiltro() roda a cada mudanca de filtro/ordenacao - setModel(&model) com
  // o mesmo ponteiro so recria o selectionModel na 1a vez, entao isso reconecta sem duplicar
  connect(ui->table->selectionModel(), &QItemSelectionModel::selectionChanged, this, &WidgetFinanceiroContas::somarSelecao, Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));

  // setStoredSelection() ficou de fora de proposito: ele guarda/restaura a selecao pelo INDICE da
  // linha, e num model paginado o indice N vira outro registro depois de um reset (a janela volta
  // pra origem 0). Era o que deixava uma linha destacada que o usuario nunca escolheu - e fazia
  // "Reverter Pagamento"/"Excluir Lançamento" agirem nela.

  // Mantem a seta do cabecalho coerente com a ordenacao que o model realmente aplicou. O QHeaderView
  // nasce apontando pra secao 0 (DESC) e so muda por clique do usuario, entao sem isso ele mente ja
  // na 1a abertura. Nao dispara recarga: SqlPaginatedModel::sort() sai cedo quando a ordem ja e essa.
  const int secaoOrdenada = model.fieldIndex(model.sortColumn(), true);

  if (secaoOrdenada != -1) { ui->table->horizontalHeader()->setSortIndicator(secaoOrdenada, model.sortOrder()); }

  // resto da configuracao da tabela depende so das colunas do model (fixas): basta uma vez
  if (tabelaConfigurada) { return; }

  tabelaConfigurada = true;

  // "R$"/"R$ Real" (não "valor"/"valorReal"): o model paginado não é QSqlQueryModel, então TableView
  // resolve a coluna via headerData() (o rótulo renomeado acima), não via record() (nome cru da coluna SQL)
  ui->table->setItemDelegateForColumn("R$", new ReaisDelegate(this));
  ui->table->setItemDelegateForColumn("R$ Real", new ReaisDelegate(this));

  if (tipo == Tipo::Receber) { ui->table->hideColumn("representacao"); }

  if (tipo == Tipo::Pagar) {
    ui->table->hideColumn("idNFe");
    ui->table->hideColumn("grupo");
  }

  ui->table->hideColumn("idLoja");
}

void WidgetFinanceiroContas::on_pushButtonInserirLancamento_clicked() {
  if (tipo == Tipo::Nulo) { throw RuntimeException("Erro Tipo::Nulo!", this); }

  auto *lancamento = new InserirLancamento((tipo == Tipo::Receber) ? InserirLancamento::Tipo::Receber : InserirLancamento::Tipo::Pagar, this);
  lancamento->setAttribute(Qt::WA_DeleteOnClose);
  lancamento->show();
}

void WidgetFinanceiroContas::on_pushButtonAdiantarRecebimento_clicked() {
  auto *adiantar = new AnteciparRecebimento(this);
  adiantar->setAttribute(Qt::WA_DeleteOnClose);
  adiantar->show();
}

void WidgetFinanceiroContas::on_doubleSpinBoxDe_valueChanged(const double value) { ui->doubleSpinBoxAte->setValue(value); }

void WidgetFinanceiroContas::on_dateEditVencimentoDe_dateChanged(const QDate date) { ui->dateEditVencimentoAte->setDate(date); }

void WidgetFinanceiroContas::on_dateEditRealizadoDe_dateChanged(const QDate date) { ui->dateEditRealizadoAte->setDate(date); }

void WidgetFinanceiroContas::setTipo(const Tipo novoTipo) {
  if (novoTipo == Tipo::Nulo) { throw RuntimeException("Erro Tipo::Nulo!", this); }

  tipo = novoTipo;

  if (tipo == Tipo::Pagar) {
    ui->pushButtonAdiantarRecebimento->hide();
    ui->radioButtonRecebido->hide();
    ui->lineEditBusca->setPlaceholderText("O.C./Contraparte/NF-e/Venda/Obs./Cód. Forn.");
  }

  if (tipo == Tipo::Receber) {
    ui->pushButtonImportarFolhaPag->hide();
    ui->radioButtonPago->hide();
    ui->radioButtonAgendado->hide();
    // ambos leem colunas que so existem no model de Pagar (idNFe / codFornecedor / pf2_idVenda):
    // em Receber lancavam "não encontrado no model paginado!" ao serem clicados
    ui->pushButtonAbrirDANFE->hide();
    ui->pushButtonRemessaItau->hide();
    ui->lineEditBusca->setPlaceholderText("Venda/O.C. Rep./Contraparte/Obs.");
  }
}

void WidgetFinanceiroContas::on_groupBoxVencimento_toggled(const bool enabled) {
  const auto children = ui->groupBoxVencimento->findChildren<QDateEdit *>(QRegularExpression("dateEdit"));

  for (const auto &child : children) { child->setEnabled(enabled); }
}

void WidgetFinanceiroContas::on_groupBoxRealizado_toggled(const bool enabled) {
  const auto children = ui->groupBoxRealizado->findChildren<QDateEdit *>(QRegularExpression("dateEdit"));

  for (const auto &child : children) { child->setEnabled(enabled); }
}

void WidgetFinanceiroContas::filtrarPorResumo(const QModelIndex &index, const SqlQueryModel &resumo) {
  if (not index.isValid()) { return; }

  // Pagar tem uma coluna por status; Receber guarda o status na linha (colunas são por tipo de pagamento)
  const QString status = (tipo == Tipo::Pagar) ? resumo.headerData(index.column(), Qt::Horizontal).toString() : resumo.record(index.row()).value("Status").toString();

  // setChecked não emite 'clicked', então o montaFiltro() explícito no fim cobre o caso de reclicar a mesma data
  if (status == "PENDENTE") { ui->radioButtonPendente->setChecked(true); }
  else if (status == "CONFERIDO") { ui->radioButtonConferido->setChecked(true); }
  else if (status == "AGENDADO") { ui->radioButtonAgendado->setChecked(true); }
  else { ui->radioButtonTodos->setChecked(true); }

  const QDate data = resumo.record(index.row()).value("Data").toDate();

  ui->dateEditVencimentoDe->setDate(data);
  ui->dateEditVencimentoAte->setDate(data);

  ui->groupBoxVencimento->setChecked(true);

  montaFiltro();
}

void WidgetFinanceiroContas::on_tableVencidos_doubleClicked(const QModelIndex &index) {
  filtrarPorResumo(index, modelVencidos);

  ui->tableVencer->clearSelection();
}

void WidgetFinanceiroContas::on_tableVencer_doubleClicked(const QModelIndex &index) {
  filtrarPorResumo(index, modelVencer);

  ui->tableVencidos->clearSelection();
}

void WidgetFinanceiroContas::on_pushButtonInserirTransferencia_clicked() {
  auto *transferencia = new InserirTransferencia(this);
  transferencia->setAttribute(Qt::WA_DeleteOnClose);

  transferencia->show();
}

// Resolve os idPagamento da selecao ANTES de qualquer dialogo. O model e paginado (janela
// deslizante), entao index.row() so identifica uma linha enquanto a janela nao for recarregada - e
// qualquer QMessageBox::exec()/QInputDialog roda um event loop aninhado onde um timer (busca com
// atraso do LineEdit, reconexao do ping do banco) pode disparar montaFiltro() e trocar as linhas
// debaixo dos indices ja capturados.
// Chamado quando um UPDATE reporta 0 linhas afetadas. O MySQL conta linhas ALTERADAS, nao
// encontradas (o app nao liga CLIENT_FOUND_ROWS - ver Application::setConnectOptions), entao 0 tanto
// pode ser "o id nao existe" - o caso que os UPDATEs querem pegar, porque um indice fora da janela
// paginada vira bind NULL - quanto "a linha ja estava nesse status", que e normal e nao e erro.
void WidgetFinanceiroContas::verificarPagamentoExiste(const QVariant &idPagamento) {
  SqlQuery query;
  query.prepare("SELECT 1 FROM " + QString((tipo == Tipo::Pagar) ? "conta_a_pagar_has_pagamento" : "conta_a_receber_has_pagamento") + " WHERE idPagamento = :idPagamento");
  query.bindValue(":idPagamento", idPagamento);

  if (not query.exec()) { throw RuntimeException("Erro verificando lançamento: " + query.lastError().text(), this); }

  if (not query.first()) { throw RuntimeException("Nenhum lançamento encontrado com o id '" + idPagamento.toString() + "'!", this); }
}

QVariantList WidgetFinanceiroContas::idsPagamentoSelecionados() const {
  const auto selection = ui->table->selectionModel()->selectedRows();

  QVariantList ids;

  for (const auto &index : selection) { ids << model.data(index.row(), "idPagamento"); }

  return ids;
}

void WidgetFinanceiroContas::on_pushButtonExcluirLancamento_clicked() {
  if (tipo == Tipo::Nulo) { throw RuntimeException("Erro Tipo::Nulo!", this); }

  // TODO: se o grupo for 'Transferencia' procurar a outra metade e cancelar tambem
  // usar 'grupo', 'data', 'valor'

  const QVariantList ids = idsPagamentoSelecionados();

  if (ids.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!", this); }

  // a tabela e MultiSelection: antes so a primeira linha era cancelada, sem aviso
  QMessageBox msgBox(QMessageBox::Question, "Atenção!", "Tem certeza que deseja excluir " + QString::number(ids.size()) + " lançamento(s)?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.button(QMessageBox::Yes)->setText("Excluir");
  msgBox.button(QMessageBox::No)->setText("Voltar");

  if (msgBox.exec() != QMessageBox::Yes) { return; }

  qApp->startTransaction("WidgetFinanceiroContas::on_pushButtonExcluirLancamento_clicked");

  SqlQuery query;
  query.prepare("UPDATE " + QString((tipo == Tipo::Pagar) ? "conta_a_pagar_has_pagamento" : "conta_a_receber_has_pagamento") + " SET status = 'CANCELADO' WHERE idPagamento = :idPagamento");

  for (const auto &id : ids) {
    query.bindValue(":idPagamento", id);

    if (not query.exec()) { throw RuntimeException("Erro excluindo lançamento: " + query.lastError().text(), this); }
    if (query.numRowsAffected() == 0) { verificarPagamentoExiste(id); } // 0 = id inexistente OU ja cancelado
  }

  qApp->endTransaction();

  montaFiltro();

  qApp->enqueueInformation("Lançamento(s) excluído(s) com sucesso!", this);
}

void WidgetFinanceiroContas::on_pushButtonReverterPagamento_clicked() {
  if (tipo == Tipo::Nulo) { throw RuntimeException("Erro Tipo::Nulo!", this); }

  // TODO: bloquear se o pagamento já estiver PENDENTE

  // TODO: verificar se precisa limpar os campos que foram preenchidos

  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!", this); }

  if (selection.size() != 1) { throw RuntimeError("Deve selecionar apenas uma linha!", this); }

  // id e data resolvidos ANTES do diálogo (ver idsPagamentoSelecionados): sem isso as validações
  // abaixo rodariam numa linha e o UPDATE em outra, contornando as próprias guardas
  const QVariant idPagamento = model.data(selection.first().row(), "idPagamento");
  const QDate realizado = model.data(selection.first().row(), "dataRealizado").toDate();

  SqlQuery queryPagamento;
  queryPagamento.prepare("SELECT grupo FROM " + QString((tipo == Tipo::Pagar) ? "conta_a_pagar_has_pagamento" : "conta_a_receber_has_pagamento") + " WHERE idPagamento = :idPagamento");
  queryPagamento.bindValue(":idPagamento", idPagamento);

  if (not queryPagamento.exec()) { throw RuntimeException("Erro buscando pagamento: " + queryPagamento.lastError().text(), this); }

  if (not queryPagamento.first()) { throw RuntimeException("Dados do pagamento não encontrado para o pagamento com id: '" + idPagamento.toString() + "'"); }

  if (queryPagamento.value("grupo").toString() == "TRANSFERÊNCIA") { throw RuntimeError("Não pode reverter transferência!", this); }

  // ---------------------------------------------------------------

  const bool mais30dias = realizado < qApp->serverDate().addDays(-30);

  if (not User::isAdmin() and mais30dias) { throw RuntimeError("O pagamento foi realizado a mais de 30 dias!", this); }

  QMessageBox msgBox(QMessageBox::Question, "Atenção!", "Tem certeza que deseja reverter?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.button(QMessageBox::Yes)->setText("Reverter");
  msgBox.button(QMessageBox::No)->setText("Voltar");

  if (msgBox.exec() != QMessageBox::Yes) { return; }

  qApp->startTransaction("WidgetFinanceiroContas::on_pushButtonReverterPagamento_clicked");

  SqlQuery query;
  query.prepare("UPDATE " + QString((tipo == Tipo::Pagar) ? "conta_a_pagar_has_pagamento" : "conta_a_receber_has_pagamento") + " SET status = 'PENDENTE' WHERE idPagamento = :idPagamento");
  query.bindValue(":idPagamento", idPagamento);

  if (not query.exec()) { throw RuntimeException("Erro revertendo lançamento: " + query.lastError().text(), this); }
  if (query.numRowsAffected() == 0) { verificarPagamentoExiste(idPagamento); } // 0 = id inexistente OU ja pendente

  qApp->endTransaction();

  updateTables();

  qApp->enqueueInformation("Lançamento revertido com sucesso!", this);
}

void WidgetFinanceiroContas::verificaCabecalho(QXlsx::Document &xlsx) {
  if (xlsx.readValue(1, 1).toString() != "Data Emissão") { throw RuntimeError("Cabeçalho errado na coluna 1!"); }
  if (xlsx.readValue(1, 2).toString() != "Centro de Custo") { throw RuntimeError("Cabeçalho errado na coluna 2!"); }
  if (xlsx.readValue(1, 3).toString() != "Contraparte") { throw RuntimeError("Cabeçalho errado na coluna 3!"); }
  if (xlsx.readValue(1, 4).toString() != "") { throw RuntimeError("Cabeçalho errado na coluna 4!"); }
  if (xlsx.readValue(1, 5).toString() != "Tipo") { throw RuntimeError("Cabeçalho errado na coluna 5!"); }
  if (xlsx.readValue(1, 6).toString() != "Vencimento") { throw RuntimeError("Cabeçalho errado na coluna 6!"); }
  if (xlsx.readValue(1, 7).toString() != "Banco") { throw RuntimeError("Cabeçalho errado na coluna 7!"); }
  if (xlsx.readValue(1, 8).toString() != "Obs") { throw RuntimeError("Cabeçalho errado na coluna 8!"); }
  if (xlsx.readValue(1, 9).toString() != "Grupo") { throw RuntimeError("Cabeçalho errado na coluna 9!"); }
}

void WidgetFinanceiroContas::on_pushButtonImportarFolhaPag_clicked() {
  const QString file = QFileDialog::getOpenFileName(this, "Importar arquivo do Excel", "", "Excel (*.xlsx)");

  if (file.isEmpty()) { return; }

  SqlTableModel modelImportar;
  modelImportar.setTable("conta_a_pagar_has_pagamento");

  QXlsx::Document xlsx(file, this);

  if (not xlsx.selectSheet("Planilha1")) { throw RuntimeException("Não encontrou 'Planilha1' na tabela!", this); }

  verificaCabecalho(xlsx);

  const int rows = xlsx.dimension().rowCount();

  qApp->startTransaction("WidgetFinanceiroContas::pushButtonImportarFolhaPag");

  for (int rowExcel = 2; rowExcel <= rows; ++rowExcel) {
    if (xlsx.readValue(rowExcel, 1).toString().isEmpty()) { continue; }

    SqlQuery queryLoja;

    if (not queryLoja.exec("SELECT idLoja FROM loja WHERE nomeFantasia = '" + xlsx.readValue(rowExcel, 2).toString() + "'")) {
      throw RuntimeException("Erro buscando idLoja: " + queryLoja.lastError().text());
    }

    if (not queryLoja.first()) { throw RuntimeError("Loja não encontrada no banco de dados: '" + xlsx.readValue(rowExcel, 2).toString() + "'"); }

    SqlQuery queryConta;

    if (not queryConta.exec("SELECT idConta FROM loja_has_conta WHERE banco = '" + xlsx.readValue(rowExcel, 7).toString() + "'")) {
      throw RuntimeException("Erro buscando idConta: " + queryConta.lastError().text());
    }

    if (not queryConta.first()) { throw RuntimeError("Conta não encontrada no banco de dados: '" + xlsx.readValue(rowExcel, 7).toString() + "'"); }

    const int rowModel = modelImportar.insertRowAtEnd();

    modelImportar.setData(rowModel, "dataEmissao", xlsx.readValue(rowExcel, 1));
    modelImportar.setData(rowModel, "idLoja", queryLoja.value("idLoja"));
    modelImportar.setData(rowModel, "contraParte", xlsx.readValue(rowExcel, 3));
    modelImportar.setData(rowModel, "valor", xlsx.readValue(rowExcel, 4));
    modelImportar.setData(rowModel, "tipo", xlsx.readValue(rowExcel, 5));
    modelImportar.setData(rowModel, "dataPagamento", xlsx.readValue(rowExcel, 6));
    modelImportar.setData(rowModel, "observacao", xlsx.readValue(rowExcel, 8));
    modelImportar.setData(rowModel, "idConta", queryConta.value("idConta"));
    modelImportar.setData(rowModel, "centroCusto", queryLoja.value("idLoja"));
    modelImportar.setData(rowModel, "grupo", xlsx.readValue(rowExcel, 9));
  }

  modelImportar.submitAll();

  qApp->endTransaction();

  qApp->enqueueInformation("Tabela importada com sucesso!", this);
}

void WidgetFinanceiroContas::on_pushButtonRemessaItau_clicked() {
  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!", this); }

  for (const auto index : selection) {
    if (model.data(index.row(), "status").toString() == "PAGO") { throw RuntimeError("Linha selecionada já paga!", this); }
    if (not model.data(index.row(), "tipo").toString().contains("TRANSF. ITAÚ")) { throw RuntimeError("Pagamento selecionado não é transferência ITAÚ!", this); }
  }

  // ids resolvidos ANTES da remessa (ver idsPagamentoSelecionados): remessaPagamentoItau240() abre
  // diálogo de arquivo, e no event loop aninhado o model pode ser recarregado — os índices deixariam
  // de apontar para os mesmos pagamentos que acabaram de entrar no arquivo CNAB
  QStringList ids;

  for (const auto &id : idsPagamentoSelecionados()) { ids << id.toString(); }

  CNAB cnab(this);
  const QString idCnab = cnab.remessaPagamentoItau240(montarPagamento(selection));

  SqlQuery query;

  if (not query.exec("UPDATE conta_a_pagar_has_pagamento SET status = 'AGENDADO', idCnab = " + idCnab + " WHERE idPagamento IN (" + ids.join(",") + ")")) {
    throw RuntimeException("Erro alterando pagamento: " + query.lastError().text(), this);
  }

  updateTables();
}

QVector<CNAB::Pagamento> WidgetFinanceiroContas::montarPagamento(const QModelIndexList &selection) {
  QVector<CNAB::Pagamento> pagamentos;

  for (const auto index : selection) {
    const QString grupo = model.data(index.row(), "grupo").toString();
    const QString contraParte = model.data(index.row(), "contraParte").toString();
    const QString observacao = model.data(index.row(), "observacao").toString();
    const QString data = model.data(index.row(), "dataPagamento").toDate().toString("ddMMyyyy");

    CNAB::Pagamento pagamento;

    if (grupo == "RH - SALÁRIOS") {
      const QString idLoja = QString::number(model.data(index.row(), "idLoja").toInt());

      SqlQuery queryFuncionario;

      if (not queryFuncionario.exec("SELECT banco, agencia, cc, nomeBanco, cpfBanco FROM usuario WHERE nomeBanco = '" + contraParte + "' AND idLoja = " + idLoja +
                                    " ORDER BY desativado ASC, (banco IS NULL OR banco = '' OR agencia IS NULL OR agencia = '' OR cc IS NULL OR cc = '') ASC, idUsuario DESC LIMIT 1")) {
        throw RuntimeException("Erro buscando dados báncarios do funcionário: " + queryFuncionario.lastError().text());
      }

      if (not queryFuncionario.first()) { throw RuntimeException("Não encontrou o funcionário: '" + contraParte + "'"); }

      const int codBanco = queryFuncionario.value("banco").toString().left(3).toInt();
      const QString cpfDest = queryFuncionario.value("cpfBanco").toString().remove(".").remove("/").remove("-");
      const QString agencia = queryFuncionario.value("agencia").toString().remove("-");
      const QStringList contaDac = queryFuncionario.value("cc").toString().split("-");
      const QString nome = queryFuncionario.value("nomeBanco").toString();

      if (codBanco == 0 or cpfDest.isEmpty() or agencia.isEmpty() or nome.isEmpty()) { throw RuntimeError("Dados bancários incompletos do funcionário: " + contraParte); }

      if (contaDac.size() != 2) { throw RuntimeError("Conta corrente formatada errada! Deve seguir o formato XXXXX-X!\nFuncionário: " + contraParte); }

      const QString &conta = contaDac.at(0);
      const QString &dac = contaDac.at(1);

      pagamento.tipo = CNAB::Pagamento::Tipo::Salario;
      pagamento.codBanco = codBanco;
      pagamento.valor = QString::number(model.data(index.row(), "valor").toDouble(), 'f', 2).remove('.').toULong();
      pagamento.observacao = observacao;
      pagamento.data = data;
      pagamento.cpfDest = cpfDest;
      pagamento.agencia = agencia.toULong();
      pagamento.conta = conta.toULong();
      pagamento.dac = dac.toULong();
      pagamento.nome = nome;

      pagamentos << pagamento;

    } else if (grupo == "PRODUTOS - VENDA") {
      SqlQuery queryFornecedor;

      if (not queryFornecedor.exec("SELECT banco, agencia, cc, nomeBanco, cnpjBanco FROM fornecedor WHERE razaoSocial = '" + contraParte + "'")) {
        throw RuntimeException("Erro buscando dados báncarios do fornecedor: " + queryFornecedor.lastError().text());
      }

      if (not queryFornecedor.first()) { throw RuntimeException("Não encontrou o fornecedor: '" + contraParte + "'"); }

      const int codBanco = queryFornecedor.value("banco").toString().left(3).toInt();
      const QString cnpjDest = queryFornecedor.value("cnpjBanco").toString().remove(".").remove("/").remove("-");
      const QString agencia = queryFornecedor.value("agencia").toString().remove("-");
      const QStringList contaDac = queryFornecedor.value("cc").toString().split("-");
      const QString nome = queryFornecedor.value("nomeBanco").toString();

      if (codBanco == 0 or cnpjDest.isEmpty() or agencia.isEmpty() or nome.isEmpty()) { throw RuntimeError("Dados bancários incompletos do fornecedor: " + contraParte); }

      if (contaDac.size() != 2) { throw RuntimeError("Conta corrente formatada errada! Deve seguir o formato XXXXX-X!\nFornecedor: " + contraParte); }

      const QString &conta = contaDac.at(0);
      const QString &dac = contaDac.at(1);

      pagamento.tipo = CNAB::Pagamento::Tipo::Fornecedor;
      pagamento.codBanco = codBanco;
      pagamento.valor = QString::number(model.data(index.row(), "valor").toDouble(), 'f', 2).remove('.').toULong();
      pagamento.observacao = observacao;
      pagamento.data = data;
      pagamento.cnpjDest = cnpjDest;
      pagamento.agencia = agencia.toULong();
      pagamento.conta = conta.toULong();
      pagamento.dac = dac.toULong();
      pagamento.nome = nome;
      pagamento.codFornecedor = model.data(index.row(), "codFornecedor").toString() + " " + model.data(index.row(), "pf2_idVenda").toString();

      pagamentos << pagamento;

    } else {
      throw RuntimeError("Grupo não permitido: " + grupo);
    }
  }

  return pagamentos;
}

void WidgetFinanceiroContas::on_pushButtonAbrirDANFE_clicked()
{
  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!", this); }

  QStringList idNFe;

  for (auto index : selection) {
    idNFe << model.data(index.row(), "idNFe").toString().split(", ");
  }

  idNFe.removeDuplicates();
  idNFe.removeAll({});

  for (auto id : idNFe) {
    ACBrLib::gerarDanfe(id.toInt());
  }
}

void WidgetFinanceiroContas::somarSelecao() {
  const auto selection = ui->table->selectionModel()->selectedRows();

  double soma = 0.;
  double somaReal = 0.;

  for (auto index : selection) {
    soma += model.data(index.row(), "valor").toDouble();
    somaReal += model.data(index.row(), "valorReal").toDouble();
  }

  QString somaStr = QLocale(QLocale::Portuguese).toString(soma, 'f', 2);
  QString somaRealStr = QLocale(QLocale::Portuguese).toString(somaReal, 'f', 2);
  QString linhas = QLocale(QLocale::Portuguese).toString(selection.size());
  ui->lineEditSomaSelecao->setText(QString("R$ %1 - R$ %2 Real - %3 linha(s)").arg(somaStr, somaRealStr, linhas));

  // QString text = ui->lineEditSomaSelecao->text();
  // int pixels = ui->lineEditSomaSelecao->fontMetrics().horizontalAdvance(text);

  // ui->lineEditSomaSelecao->setFixedWidth(pixels + 10);
}

// TODO: [Verificar com Midi] contareceber.status e venda.statusFinanceiro deveriam ser o mesmo porem em diversas linhas eles tem valores diferentes
// TODO: essa tela mostra apenas o valor previsto, verificar se deve mostrar tambem o valorReal pago/recebido

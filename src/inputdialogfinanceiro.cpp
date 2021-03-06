#include "inputdialogfinanceiro.h"
#include "ui_inputdialogfinanceiro.h"

#include "application.h"
#include "comboboxdelegate.h"
#include "doubledelegate.h"
#include "editdelegate.h"
#include "noeditdelegate.h"
#include "porcentagemdelegate.h"
#include "reaisdelegate.h"
#include "sortfilterproxymodel.h"
#include "sqlquery.h"

#include <QDebug>
#include <QLineEdit>
#include <QSqlError>

InputDialogFinanceiro::InputDialogFinanceiro(const Tipo &tipo, QWidget *parent) : QDialog(parent), tipo(tipo), ui(new Ui::InputDialogFinanceiro) {
  ui->setupUi(this);

  setWindowFlags(Qt::Window);

  setupTables();

  ui->widgetPgts->setTipo(WidgetPagamentos::Tipo::Compra);

  ui->widgetPgts->hide();

  ui->frameData->hide();
  ui->frameDataPreco->hide();
  ui->groupBoxFinanceiro->hide();

  ui->labelAliquota->hide();
  ui->doubleSpinBoxAliquota->hide();
  ui->labelSt->hide();
  ui->doubleSpinBoxSt->hide();

  ui->dateEditEvento->setDate(qApp->serverDate());
  ui->dateEditProximo->setDate(qApp->serverDate());
  ui->dateEditPgtSt->setDate(qApp->serverDate());
  ui->dateEditFrete->setDate(qApp->serverDate());

  if (tipo == Tipo::ConfirmarCompra) {
    ui->frameData->show();
    ui->frameDataPreco->show();
    ui->groupBoxFrete->show();

    ui->labelEvento->setText("Data confirmação:");
    ui->labelProximoEvento->setText("Data prevista faturamento:");

    ui->widgetPgts->show();

    ui->treeView->hide();
  }

  if (tipo == Tipo::Financeiro) {
    ui->frameDataPreco->show();
    ui->groupBoxFinanceiro->show();

    ui->frameAdicionais->hide();

    ui->treeView->hide();
  }

  setConnections();

  connect(ui->widgetPgts, &WidgetPagamentos::montarFluxoCaixa, [=]() { montarFluxoCaixa(true); });

  showMaximized();
}

InputDialogFinanceiro::~InputDialogFinanceiro() { delete ui; }

void InputDialogFinanceiro::setConnections() {
  if (not blockingSignals.isEmpty()) { blockingSignals.pop(); } // avoid crashing on first setConnections

  if (not blockingSignals.isEmpty()) { return; } // delay setting connections until last unset/set block

  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->checkBoxDataFrete, &QCheckBox::toggled, this, &InputDialogFinanceiro::on_checkBoxDataFrete_toggled, connectionType);
  connect(ui->checkBoxMarcarTodos, &QCheckBox::toggled, this, &InputDialogFinanceiro::on_checkBoxMarcarTodos_toggled, connectionType);
  connect(ui->checkBoxParcelarSt, &QCheckBox::toggled, this, &InputDialogFinanceiro::on_checkBoxParcelarSt_toggled, connectionType);
  connect(ui->comboBoxST, &QComboBox::currentTextChanged, this, &InputDialogFinanceiro::on_comboBoxST_currentTextChanged, connectionType);
  connect(ui->dateEditEvento, &QDateEdit::dateChanged, this, &InputDialogFinanceiro::on_dateEditEvento_dateChanged, connectionType);
  connect(ui->dateEditFrete, &QDateEdit::dateChanged, this, &InputDialogFinanceiro::on_dateEditFrete_dateChanged, connectionType);
  connect(ui->dateEditPgtSt, &QDateEdit::dateChanged, this, &InputDialogFinanceiro::on_dateEditPgtSt_dateChanged, connectionType);
  connect(ui->doubleSpinBoxAliquota, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &InputDialogFinanceiro::on_doubleSpinBoxAliquota_valueChanged, connectionType);
  connect(ui->doubleSpinBoxFrete, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &InputDialogFinanceiro::on_doubleSpinBoxFrete_valueChanged, connectionType);
  connect(ui->doubleSpinBoxSt, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &InputDialogFinanceiro::on_doubleSpinBoxSt_valueChanged, connectionType);
  connect(ui->lineEditCodFornecedor, &QLineEdit::textChanged, this, &InputDialogFinanceiro::on_lineEditCodFornecedor_textChanged, connectionType);
  connect(ui->pushButtonCorrigirFluxo, &QPushButton::clicked, this, &InputDialogFinanceiro::on_pushButtonCorrigirFluxo_clicked, connectionType);
  connect(ui->pushButtonSalvar, &QPushButton::clicked, this, &InputDialogFinanceiro::on_pushButtonSalvar_clicked, connectionType);
  connect(ui->table->model(), &QAbstractItemModel::dataChanged, this, &InputDialogFinanceiro::updateTableData, connectionType);
}

void InputDialogFinanceiro::unsetConnections() {
  blockingSignals.push(0);

  disconnect(ui->checkBoxDataFrete, &QCheckBox::toggled, this, &InputDialogFinanceiro::on_checkBoxDataFrete_toggled);
  disconnect(ui->checkBoxMarcarTodos, &QCheckBox::toggled, this, &InputDialogFinanceiro::on_checkBoxMarcarTodos_toggled);
  disconnect(ui->checkBoxParcelarSt, &QCheckBox::toggled, this, &InputDialogFinanceiro::on_checkBoxParcelarSt_toggled);
  disconnect(ui->comboBoxST, &QComboBox::currentTextChanged, this, &InputDialogFinanceiro::on_comboBoxST_currentTextChanged);
  disconnect(ui->dateEditEvento, &QDateEdit::dateChanged, this, &InputDialogFinanceiro::on_dateEditEvento_dateChanged);
  disconnect(ui->dateEditFrete, &QDateEdit::dateChanged, this, &InputDialogFinanceiro::on_dateEditFrete_dateChanged);
  disconnect(ui->dateEditPgtSt, &QDateEdit::dateChanged, this, &InputDialogFinanceiro::on_dateEditPgtSt_dateChanged);
  disconnect(ui->doubleSpinBoxAliquota, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &InputDialogFinanceiro::on_doubleSpinBoxAliquota_valueChanged);
  disconnect(ui->doubleSpinBoxFrete, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &InputDialogFinanceiro::on_doubleSpinBoxFrete_valueChanged);
  disconnect(ui->doubleSpinBoxSt, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &InputDialogFinanceiro::on_doubleSpinBoxSt_valueChanged);
  disconnect(ui->lineEditCodFornecedor, &QLineEdit::textChanged, this, &InputDialogFinanceiro::on_lineEditCodFornecedor_textChanged);
  disconnect(ui->pushButtonCorrigirFluxo, &QPushButton::clicked, this, &InputDialogFinanceiro::on_pushButtonCorrigirFluxo_clicked);
  disconnect(ui->pushButtonSalvar, &QPushButton::clicked, this, &InputDialogFinanceiro::on_pushButtonSalvar_clicked);
  disconnect(ui->table->model(), &QAbstractItemModel::dataChanged, this, &InputDialogFinanceiro::updateTableData);
}

void InputDialogFinanceiro::on_doubleSpinBoxAliquota_valueChanged(const double aliquota) {
  // FIXME: porcentagem esta limitado a 20% mas valor nao

  unsetConnections();

  try {
    [&] {
      double total = 0;

      const auto list = ui->table->selectionModel()->selectedRows();

      for (const auto &index : list) {
        const int row = index.row();

        modelPedidoFornecedor2.setData(row, "aliquotaSt", aliquota);

        total += modelPedidoFornecedor2.data(row, "preco").toDouble();
      }

      const double valueSt = total * (aliquota / 100);

      ui->doubleSpinBoxSt->setValue(valueSt);

      // TODO: adicionar frete/adicionais
      ui->doubleSpinBoxTotal->setValue(total);
    }();
  } catch (std::exception &) {}

  setConnections();

  montarFluxoCaixa();
}

QDate InputDialogFinanceiro::getDate() const { return ui->dateEditEvento->date(); }

QDate InputDialogFinanceiro::getNextDate() const { return ui->dateEditProximo->date(); }

void InputDialogFinanceiro::setupTables() {
  // TODO: montar TreeView pf/pf2 (pelo menos no historico)

  modelPedidoFornecedor.setTable("pedido_fornecedor_has_produto");

  //--------------------------------------------------

  modelPedidoFornecedor2.setTable("pedido_fornecedor_has_produto2");

  modelPedidoFornecedor2.setHeaderData("aliquotaSt", "Alíquota ST");
  modelPedidoFornecedor2.setHeaderData("st", "ST");
  modelPedidoFornecedor2.setHeaderData("status", "Status");
  modelPedidoFornecedor2.setHeaderData("ordemRepresentacao", "Cód. Rep.");
  modelPedidoFornecedor2.setHeaderData("codFornecedor", "Cód. Forn.");
  modelPedidoFornecedor2.setHeaderData("idVenda", "Venda");
  modelPedidoFornecedor2.setHeaderData("fornecedor", "Fornecedor");
  modelPedidoFornecedor2.setHeaderData("descricao", "Produto");
  modelPedidoFornecedor2.setHeaderData("obs", "Obs.");
  modelPedidoFornecedor2.setHeaderData("colecao", "Coleção");
  modelPedidoFornecedor2.setHeaderData("codComercial", "Cód. Com.");
  modelPedidoFornecedor2.setHeaderData("quant", "Quant.");
  modelPedidoFornecedor2.setHeaderData("un", "Un.");
  modelPedidoFornecedor2.setHeaderData("caixas", "Caixas");
  modelPedidoFornecedor2.setHeaderData("prcUnitario", "R$ Unit.");
  modelPedidoFornecedor2.setHeaderData("preco", "Total");

  modelPedidoFornecedor2.proxyModel = new SortFilterProxyModel(&modelPedidoFornecedor2, this);

  ui->table->setModel(&modelPedidoFornecedor2);

  ui->table->hideColumn("idRelacionado");
  ui->table->hideColumn("idPedido2");
  ui->table->hideColumn("idPedidoFK");
  ui->table->hideColumn("selecionado");
  ui->table->hideColumn("statusFinanceiro");
  ui->table->hideColumn("ordemCompra");
  ui->table->hideColumn("idVendaProduto1");
  ui->table->hideColumn("idVendaProduto2");
  ui->table->hideColumn("idCompra");
  ui->table->hideColumn("idProduto");
  ui->table->hideColumn("quantUpd");
  ui->table->hideColumn("codBarras");
  ui->table->hideColumn("dataPrevCompra");
  ui->table->hideColumn("dataRealCompra");
  ui->table->hideColumn("dataPrevConf");
  ui->table->hideColumn("dataRealConf");
  ui->table->hideColumn("dataPrevFat");
  ui->table->hideColumn("dataRealFat");
  ui->table->hideColumn("dataPrevColeta");
  ui->table->hideColumn("dataRealColeta");
  ui->table->hideColumn("dataPrevReceb");
  ui->table->hideColumn("dataRealReceb");
  ui->table->hideColumn("dataPrevEnt");
  ui->table->hideColumn("dataRealEnt");
  ui->table->hideColumn("un2");
  ui->table->hideColumn("kgcx");
  ui->table->hideColumn("formComercial");

  if (tipo == Tipo::ConfirmarCompra) {
    ui->table->hideColumn("status");
    ui->table->hideColumn("ordemRepresentacao");
    ui->table->hideColumn("codFornecedor");
  }

  ui->table->setItemDelegate(new NoEditDelegate(this));

  ui->table->setItemDelegateForColumn("obs", new EditDelegate(this));
  ui->table->setItemDelegateForColumn("aliquotaSt", new PorcentagemDelegate(false, this));
  ui->table->setItemDelegateForColumn("st", new ComboBoxDelegate(ComboBoxDelegate::Tipo::ST, this));
  ui->table->setItemDelegateForColumn("prcUnitario", new ReaisDelegate(this));
  ui->table->setItemDelegateForColumn("preco", new ReaisDelegate(this));

  connect(ui->table->selectionModel(), &QItemSelectionModel::selectionChanged, this, &InputDialogFinanceiro::calcularTotal);

  //--------------------------------------------------

  modelFluxoCaixa.setTable("conta_a_pagar_has_pagamento");

  modelFluxoCaixa.setHeaderData("tipo", "Tipo");
  modelFluxoCaixa.setHeaderData("parcela", "Parcela");
  modelFluxoCaixa.setHeaderData("valor", "R$");
  modelFluxoCaixa.setHeaderData("dataPagamento", "Data");
  modelFluxoCaixa.setHeaderData("observacao", "Obs.");
  modelFluxoCaixa.setHeaderData("status", "Status");

  ui->tableFluxoCaixa->setModel(&modelFluxoCaixa);

  ui->tableFluxoCaixa->hideColumn("idVenda");
  ui->tableFluxoCaixa->hideColumn("idCnab");
  ui->tableFluxoCaixa->hideColumn("idNFe");
  ui->tableFluxoCaixa->hideColumn("nfe");
  ui->tableFluxoCaixa->hideColumn("contraParte");
  ui->tableFluxoCaixa->hideColumn("idCompra");
  ui->tableFluxoCaixa->hideColumn("idLoja");
  ui->tableFluxoCaixa->hideColumn("idPagamento");
  ui->tableFluxoCaixa->hideColumn("dataEmissao");
  ui->tableFluxoCaixa->hideColumn("dataRealizado");
  ui->tableFluxoCaixa->hideColumn("valorReal");
  ui->tableFluxoCaixa->hideColumn("tipoReal");
  ui->tableFluxoCaixa->hideColumn("parcelaReal");
  ui->tableFluxoCaixa->hideColumn("idConta");
  ui->tableFluxoCaixa->hideColumn("tipoDet");
  ui->tableFluxoCaixa->hideColumn("centroCusto");
  ui->tableFluxoCaixa->hideColumn("grupo");
  ui->tableFluxoCaixa->hideColumn("subGrupo");
  ui->tableFluxoCaixa->hideColumn("desativado");

  ui->tableFluxoCaixa->setItemDelegate(new DoubleDelegate(this));

  ui->tableFluxoCaixa->setItemDelegateForColumn("valor", new ReaisDelegate(this));
}

void InputDialogFinanceiro::montarFluxoCaixa(const bool updateDate) {
  qDebug() << "montarFluxoCaixa";

  unsetConnections();

  try {
    [=] {
      if (representacao) { return; }

      modelFluxoCaixa.revertAll();

      if (tipo == Tipo::Financeiro) {
        const auto selection = ui->tableFluxoCaixa->selectionModel()->selectedRows();

        for (const auto &index : selection) { modelFluxoCaixa.setData(index.row(), "status", "SUBSTITUIDO"); }
      }

      for (int pagamento = 0; pagamento < ui->widgetPgts->pagamentos; ++pagamento) {
        if (ui->widgetPgts->listTipoPgt.at(pagamento)->currentText() == "ESCOLHA UMA OPÇÃO!") { continue; }

        const QString tipoPgt = ui->widgetPgts->listTipoPgt.at(pagamento)->currentText();
        const int parcelas = ui->widgetPgts->listParcela.at(pagamento)->currentIndex() + 1;

        SqlQuery query2;
        query2.prepare("SELECT fp.idConta, fp.pula1Mes, fp.ajustaDiaUtil, fp.dMaisUm, fp.centavoSobressalente, fpt.taxa FROM forma_pagamento fp LEFT JOIN forma_pagamento_has_taxa fpt ON "
                       "fp.idPagamento = fpt.idPagamento WHERE fp.pagamento = :pagamento AND fpt.parcela = :parcela");
        query2.bindValue(":pagamento", tipoPgt);
        query2.bindValue(":parcela", parcelas);

        if (not query2.exec() or not query2.first()) { throw RuntimeException("Erro buscando taxa: " + query2.lastError().text(), this); }

        const int idConta = query2.value("idConta").toInt();
        const bool centavoSobressalente = query2.value("centavoSobressalente").toBool();

        //-----------------------------------------------------------------

        const QString observacaoPgt = ui->widgetPgts->listObservacao.at(pagamento)->text();

        const double valor = ui->widgetPgts->listValorPgt.at(pagamento)->value();
        const double valorParcela = qApp->roundDouble(valor / parcelas, 2);
        const double restoParcela = qApp->roundDouble(valor - (valorParcela * parcelas), 2);

        for (int parcela = 0; parcela < parcelas; ++parcela) {
          const int row = modelFluxoCaixa.insertRowAtEnd();

          modelFluxoCaixa.setData(row, "contraParte", modelPedidoFornecedor2.data(0, "fornecedor"));
          modelFluxoCaixa.setData(row, "dataEmissao", ui->dateEditEvento->date());
          modelFluxoCaixa.setData(row, "idCompra", modelPedidoFornecedor2.data(0, "idCompra"));
          modelFluxoCaixa.setData(row, "idLoja", 1); // Geral

          QString tipoData = ui->widgetPgts->listTipoData.at(pagamento)->currentText();
          QDate dataPgt = ui->widgetPgts->listDataPgt.at(pagamento)->date();

          if (tipoData == "DATA + 1 MÊS" or tipoData == "DATA MÊS") {
            dataPgt = dataPgt.addMonths(parcela);
          } else {
            dataPgt = dataPgt.addDays(tipoData.toInt() * parcela);
          }

          dataPgt = qApp->ajustarDiaUtil(dataPgt);

          modelFluxoCaixa.setData(row, "dataPagamento", dataPgt);

          double val;

          if (centavoSobressalente and parcela == 0) {
            val = valorParcela + restoParcela;
          } else if (not centavoSobressalente and parcela == parcelas - 1) {
            val = valorParcela + restoParcela;
          } else {
            val = valorParcela;
          }

          modelFluxoCaixa.setData(row, "valor", val);
          modelFluxoCaixa.setData(row, "tipo", QString::number(pagamento + 1) + ". " + tipoPgt);
          modelFluxoCaixa.setData(row, "parcela", parcela + 1);
          modelFluxoCaixa.setData(row, "observacao", observacaoPgt);
          modelFluxoCaixa.setData(row, "idConta", idConta);
          modelFluxoCaixa.setData(row, "grupo", "PRODUTOS - VENDA");
        }
      }

      if (ui->doubleSpinBoxFrete->value() > 0) {
        const int row = modelFluxoCaixa.insertRowAtEnd();

        modelFluxoCaixa.setData(row, "contraParte", modelPedidoFornecedor2.data(0, "fornecedor"));
        modelFluxoCaixa.setData(row, "dataEmissao", ui->dateEditEvento->date());
        modelFluxoCaixa.setData(row, "idCompra", modelPedidoFornecedor2.data(0, "idCompra"));
        modelFluxoCaixa.setData(row, "idLoja", 1); // Geral
        QDate dataFrete = ui->checkBoxDataFrete->isChecked() ? qApp->ajustarDiaUtil(ui->dateEditFrete->date()) : modelFluxoCaixa.data(0, "dataPagamento").toDate();
        modelFluxoCaixa.setData(row, "dataPagamento", dataFrete);
        modelFluxoCaixa.setData(row, "valor", ui->doubleSpinBoxFrete->value());
        modelFluxoCaixa.setData(row, "tipo", "Frete");
        modelFluxoCaixa.setData(row, "parcela", 1);
        modelFluxoCaixa.setData(row, "observacao", "");
        modelFluxoCaixa.setData(row, "idConta", modelFluxoCaixa.data(0, "idConta"));
        modelFluxoCaixa.setData(row, "grupo", "LOGÍSTICA - FRETES");
      }

      // set st date
      if (updateDate) {
        if (ui->widgetPgts->pagamentos > 0) { ui->dateEditPgtSt->setDate(ui->widgetPgts->listDataPgt.at(0)->date()); }
      }

      //----------------------------------------------

      if (ui->widgetPgts->pagamentos > 0) {
        double stForn = 0;
        double stLoja = 0;

        const auto list = ui->table->selectionModel()->selectedRows();

        for (const auto &index : list) {
          const int row = index.row();

          const QString tipoSt = modelPedidoFornecedor2.data(row, "st").toString();

          if (tipoSt == "SEM ST") { continue; }

          const double aliquotaSt = modelPedidoFornecedor2.data(row, "aliquotaSt").toDouble();
          const double preco = modelPedidoFornecedor2.data(row, "preco").toDouble();
          const double valorSt = preco * (aliquotaSt / 100);

          if (tipoSt == "ST FORNECEDOR") { stForn += valorSt; }
          if (tipoSt == "ST LOJA") { stLoja += valorSt; }
        }

        // 'ST Loja' tem a data do faturamento, 'ST Fornecedor' segue as datas dos pagamentos

        const int parcelas = ui->widgetPgts->listParcela.at(0)->currentIndex() + 1;

        if (stForn > 0) {
          if (ui->checkBoxParcelarSt->isChecked()) {
            // TODO: dividir antes o valor para achar o resto e colocar na primeira/ultima parcela

            for (int parcela = 0; parcela < parcelas; ++parcela) {
              const int row = modelFluxoCaixa.insertRowAtEnd();

              modelFluxoCaixa.setData(row, "contraParte", modelPedidoFornecedor2.data(0, "fornecedor"));
              modelFluxoCaixa.setData(row, "dataEmissao", ui->dateEditEvento->date());
              modelFluxoCaixa.setData(row, "idCompra", modelPedidoFornecedor2.data(0, "idCompra"));
              modelFluxoCaixa.setData(row, "idLoja", 1); // Geral

              const QString tipoData = ui->widgetPgts->listTipoData.at(0)->currentText();
              QDate dataPgt = ui->widgetPgts->listDataPgt.at(0)->date();

              if (tipoData == "DATA + 1 MÊS" or tipoData == "DATA MÊS") {
                dataPgt = dataPgt.addMonths(parcela);
              } else {
                dataPgt = dataPgt.addDays(tipoData.toInt() * parcela);
              }

              dataPgt = qApp->ajustarDiaUtil(dataPgt);

              modelFluxoCaixa.setData(row, "dataPagamento", dataPgt);

              modelFluxoCaixa.setData(row, "valor", stForn / parcelas);
              modelFluxoCaixa.setData(row, "tipo", "ST Fornecedor");
              modelFluxoCaixa.setData(row, "parcela", parcela + 1);
              modelFluxoCaixa.setData(row, "observacao", "");
              modelFluxoCaixa.setData(row, "idConta", modelFluxoCaixa.data(0, "idConta"));
              modelFluxoCaixa.setData(row, "grupo", "IMPOSTOS - ICMS;ST;ISS");
            }
          } else {
            const int row = modelFluxoCaixa.insertRowAtEnd();

            modelFluxoCaixa.setData(row, "contraParte", modelPedidoFornecedor2.data(0, "fornecedor"));
            modelFluxoCaixa.setData(row, "dataEmissao", ui->dateEditEvento->date());
            modelFluxoCaixa.setData(row, "idCompra", modelPedidoFornecedor2.data(0, "idCompra"));
            modelFluxoCaixa.setData(row, "idLoja", 1); // Geral
            modelFluxoCaixa.setData(row, "dataPagamento", qApp->ajustarDiaUtil(ui->dateEditPgtSt->date()));
            modelFluxoCaixa.setData(row, "valor", stForn);
            modelFluxoCaixa.setData(row, "tipo", "ST Fornecedor");
            modelFluxoCaixa.setData(row, "parcela", 1);
            modelFluxoCaixa.setData(row, "observacao", "");
            modelFluxoCaixa.setData(row, "idConta", modelFluxoCaixa.data(0, "idConta"));
            modelFluxoCaixa.setData(row, "grupo", "IMPOSTOS - ICMS;ST;ISS");
          }
        }

        if (stLoja > 0) {
          const int row = modelFluxoCaixa.insertRowAtEnd();

          modelFluxoCaixa.setData(row, "contraParte", modelPedidoFornecedor2.data(0, "fornecedor"));
          modelFluxoCaixa.setData(row, "dataEmissao", ui->dateEditEvento->date());
          modelFluxoCaixa.setData(row, "idCompra", modelPedidoFornecedor2.data(0, "idCompra"));
          modelFluxoCaixa.setData(row, "idLoja", 1); // Geral
          modelFluxoCaixa.setData(row, "dataPagamento", qApp->ajustarDiaUtil(ui->dateEditPgtSt->date()));
          modelFluxoCaixa.setData(row, "valor", stLoja);
          modelFluxoCaixa.setData(row, "tipo", "ST Loja");
          modelFluxoCaixa.setData(row, "parcela", 1);
          modelFluxoCaixa.setData(row, "observacao", "");
          modelFluxoCaixa.setData(row, "idConta", modelFluxoCaixa.data(0, "idConta"));
          modelFluxoCaixa.setData(row, "grupo", "IMPOSTOS - ICMS;ST;ISS");
        }
      }
    }();
  } catch (std::exception &) {}

  setConnections();
}

void InputDialogFinanceiro::calcularTotal() {
  unsetConnections();

  try {
    [&] {
      double total = 0;

      const auto list = ui->table->selectionModel()->selectedRows();

      for (const auto &index : list) { total += modelPedidoFornecedor2.data(index.row(), "preco").toDouble(); }

      ui->doubleSpinBoxTotal->setValue(total);
      ui->widgetPgts->setTotal(total);
    }();
  } catch (std::exception &) {}

  setConnections();
}

void InputDialogFinanceiro::updateTableData(const QModelIndex &topLeft) {
  unsetConnections();

  try {
    [&] {
      const QString header = modelPedidoFornecedor2.headerData(topLeft.column(), Qt::Horizontal).toString();
      const int row = topLeft.row();

      const double quant = modelPedidoFornecedor2.data(row, "quant").toDouble();

      const auto match = modelPedidoFornecedor.match("idPedido1", modelPedidoFornecedor2.data(row, "idPedidoFK"), 1, Qt::MatchExactly);

      if (match.isEmpty()) { throw RuntimeException("Erro atualizando valores na linha mãe!", this); }

      const int rowMae = match.first().row();

      if (header == "Quant." or header == "R$ Unit.") {
        const double prcUnitario = modelPedidoFornecedor2.data(row, "prcUnitario").toDouble();
        const double preco = quant * prcUnitario;

        modelPedidoFornecedor2.setData(row, "preco", preco);

        modelPedidoFornecedor.setData(rowMae, "prcUnitario", prcUnitario);
        modelPedidoFornecedor.setData(rowMae, "preco", preco);
      }

      if (header == "Total") {
        const double preco = modelPedidoFornecedor2.data(row, "preco").toDouble();
        const double prcUnitario = preco / quant;

        modelPedidoFornecedor2.setData(row, "prcUnitario", prcUnitario);

        modelPedidoFornecedor.setData(rowMae, "prcUnitario", prcUnitario);
        modelPedidoFornecedor.setData(rowMae, "preco", preco);
      }
    }();
  } catch (std::exception &) {}

  setConnections();

  calcularTotal();
  ui->widgetPgts->resetarPagamentos();
}

void InputDialogFinanceiro::setTreeView() {
  modelTree.appendModel(&modelPedidoFornecedor);
  modelTree.appendModel(&modelPedidoFornecedor2);

  modelTree.updateData();

  modelTree.setHeaderData("aliquotaSt", "Alíquota ST");
  modelTree.setHeaderData("st", "ST");
  modelTree.setHeaderData("status", "Status");
  modelTree.setHeaderData("ordemRepresentacao", "Cód. Rep.");
  modelTree.setHeaderData("idVenda", "Código");
  modelTree.setHeaderData("fornecedor", "Fornecedor");
  modelTree.setHeaderData("descricao", "Produto");
  modelTree.setHeaderData("obs", "Obs.");
  modelTree.setHeaderData("colecao", "Coleção");
  modelTree.setHeaderData("codComercial", "Cód. Com.");
  modelTree.setHeaderData("quant", "Quant.");
  modelTree.setHeaderData("un", "Un.");
  modelTree.setHeaderData("un2", "Un.2");
  modelTree.setHeaderData("caixas", "Caixas");
  modelTree.setHeaderData("prcUnitario", "R$ Unit.");
  modelTree.setHeaderData("preco", "Total");
  modelTree.setHeaderData("kgcx", "Kg./Cx.");
  modelTree.setHeaderData("formComercial", "Formato");

  ui->treeView->setModel(&modelTree);

  ui->treeView->hideColumn("idRelacionado");
  ui->treeView->hideColumn("idPedido2");
  ui->treeView->hideColumn("idPedidoFK");
  ui->treeView->hideColumn("selecionado");
  ui->treeView->hideColumn("statusFinanceiro");
  ui->treeView->hideColumn("ordemCompra");
  ui->treeView->hideColumn("idVendaProduto1");
  ui->treeView->hideColumn("idVendaProduto2");
  ui->treeView->hideColumn("idCompra");
  ui->treeView->hideColumn("idProduto");
  ui->treeView->hideColumn("quantUpd");
  ui->treeView->hideColumn("codBarras");
  ui->treeView->hideColumn("dataPrevCompra");
  ui->treeView->hideColumn("dataRealCompra");
  ui->treeView->hideColumn("dataPrevConf");
  ui->treeView->hideColumn("dataRealConf");
  ui->treeView->hideColumn("dataPrevFat");
  ui->treeView->hideColumn("dataRealFat");
  ui->treeView->hideColumn("dataPrevColeta");
  ui->treeView->hideColumn("dataRealColeta");
  ui->treeView->hideColumn("dataPrevReceb");
  ui->treeView->hideColumn("dataRealReceb");
  ui->treeView->hideColumn("dataPrevEnt");
  ui->treeView->hideColumn("dataRealEnt");
  ui->treeView->hideColumn("created");
  ui->treeView->hideColumn("lastUpdated");

  ui->treeView->setItemDelegate(new NoEditDelegate(this));

  ui->treeView->setItemDelegateForColumn("aliquotaSt", new PorcentagemDelegate(false, this));
  ui->treeView->setItemDelegateForColumn("st", new ComboBoxDelegate(ComboBoxDelegate::Tipo::ST, this));
  ui->treeView->setItemDelegateForColumn("prcUnitario", new ReaisDelegate(this));
  ui->treeView->setItemDelegateForColumn("preco", new ReaisDelegate(this));
  ui->treeView->setItemDelegateForColumn("quant", new EditDelegate(this));
}

void InputDialogFinanceiro::setFilter(const QString &idCompra) {
  if (idCompra.isEmpty()) { throw RuntimeException("IdCompra vazio!"); }

  QString filtro = "idCompra IN (" + idCompra + ")";

  if (tipo == Tipo::ConfirmarCompra) { filtro += " AND status = 'EM COMPRA'"; }
  if (tipo == Tipo::Financeiro) { filtro += " AND status != 'CANCELADO'"; }

  modelPedidoFornecedor.setFilter(filtro);

  modelPedidoFornecedor.select();

  modelPedidoFornecedor2.setFilter(filtro);

  modelPedidoFornecedor2.select();

  //  setTreeView();

  if (tipo == Tipo::ConfirmarCompra or tipo == Tipo::Financeiro) {
    modelFluxoCaixa.setFilter((tipo == Tipo::ConfirmarCompra) ? "0" : "idCompra IN (" + idCompra + ") AND status NOT IN ('CANCELADO', 'SUBSTITUIDO') AND desativado = FALSE");

    modelFluxoCaixa.select();

    ui->checkBoxMarcarTodos->setChecked(true);
  }

  if (tipo == Tipo::Financeiro) {
    ui->comboBoxFinanceiro->setCurrentText(modelPedidoFornecedor.data(0, "statusFinanceiro").toString());
    ui->dateEditEvento->setDate(modelFluxoCaixa.data(0, "dataEmissao").toDate());
  }

  SqlQuery query;
  query.prepare("SELECT v.representacao FROM pedido_fornecedor_has_produto pf LEFT JOIN venda v ON pf.idVenda = v.idVenda WHERE pf.idCompra = :idCompra");
  query.bindValue(":idCompra", idCompra);

  if (not query.exec() or not query.first()) { throw RuntimeException("Erro buscando se é representacao: " + query.lastError().text()); }

  representacao = query.value("representacao").toBool();

  if (representacao and tipo == Tipo::ConfirmarCompra) {
    ui->framePagamentos->hide();
    ui->frameAdicionais->hide();
  }

  if (representacao and tipo == Tipo::Financeiro) {
    ui->framePagamentos->hide();
    ui->pushButtonSalvar->hide();
  }

  if (representacao) { ui->lineEditCodFornecedor->hide(); }

  ui->widgetPgts->setRepresentacao(representacao);

  setWindowTitle("OC: " + modelPedidoFornecedor.data(0, "ordemCompra").toString());

  // -------------------------------------------------------------------------

  calcularTotal();
}

void InputDialogFinanceiro::on_pushButtonSalvar_clicked() {
  unsetConnections();

  try {
    [=] {
      verifyFields();

      qApp->startTransaction("InputDialogFinanceiro::on_pushButtonSalvar");

      cadastrar();

      qApp->endTransaction();

      qApp->enqueueInformation("Dados salvos com sucesso!", this);

      QDialog::accept();
      close();
    }();
  } catch (std::exception &) {}

  setConnections();
}

void InputDialogFinanceiro::verifyFields() {
  // TODO: implementar outras verificacoes necessarias

  if (ui->widgetPgts->isHidden()) { return; }

  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhum item selecionado!"); }

  if (not representacao) {
    if (tipo == Tipo::ConfirmarCompra) {
      for (auto &index : selection) {
        if (modelPedidoFornecedor2.data(index.row(), "codFornecedor").toString().isEmpty()) { throw RuntimeError("Não preencheu código do fornecedor!"); }
      }
    }

    if (not qFuzzyCompare(ui->doubleSpinBoxTotal->value(), ui->widgetPgts->getTotalPag())) { throw RuntimeError("Soma dos pagamentos difere do total! Favor verificar!"); }

    ui->widgetPgts->verifyFields();

    if (modelFluxoCaixa.rowCount() == 0) {
      QMessageBox msgBox(QMessageBox::Question, "Atenção!", "Sem pagamentos cadastrados, deseja continuar mesmo assim?", QMessageBox::Yes | QMessageBox::No, this);
      msgBox.setButtonText(QMessageBox::Yes, "Continuar");
      msgBox.setButtonText(QMessageBox::No, "Voltar");

      if (msgBox.exec() == QMessageBox::No) { throw std::exception(); }
    }
  }
}

void InputDialogFinanceiro::cadastrar() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (tipo == Tipo::ConfirmarCompra) {
    for (const auto &index : list) { modelPedidoFornecedor2.setData(index.row(), "selecionado", true); }
  }

  if (tipo == Tipo::Financeiro) {
    for (const auto &index : list) { modelPedidoFornecedor2.setData(index.row(), "statusFinanceiro", ui->comboBoxFinanceiro->currentText()); }
  }

  modelPedidoFornecedor.submitAll();

  modelPedidoFornecedor2.submitAll();

  modelFluxoCaixa.submitAll();
}

void InputDialogFinanceiro::on_dateEditEvento_dateChanged(const QDate &date) {
  if (ui->dateEditProximo->date() < date) { ui->dateEditProximo->setDate(date); }
}

void InputDialogFinanceiro::on_checkBoxMarcarTodos_toggled(const bool checked) { checked ? ui->table->selectAll() : ui->table->clearSelection(); }

void InputDialogFinanceiro::on_pushButtonCorrigirFluxo_clicked() {
  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Selecione os produtos que terão o fluxo corrigido!", this); }

  const auto selection2 = ui->tableFluxoCaixa->selectionModel()->selectedRows();

  if (selection2.isEmpty()) { throw RuntimeError("Selecione os pagamentos que serão substituídos!", this); }

  //--------------------------------------------------

  ui->frameAdicionais->show();
  //  ui->framePgtTotal->show();
  //  ui->pushButtonAdicionarPagamento->show();
  ui->widgetPgts->show();

  // TODO: 5alterar para que apenas na tela do financeiro compra a opcao de corrigir fluxo percorra todas as linhas
  // (enquanto na confirmacao de pagamento percorre apenas as linhas selecionadas)

  calcularTotal();
  ui->widgetPgts->resetarPagamentos();
}

void InputDialogFinanceiro::on_doubleSpinBoxFrete_valueChanged(double) {
  calcularTotal();
  ui->widgetPgts->resetarPagamentos();
}

void InputDialogFinanceiro::on_dateEditPgtSt_dateChanged(const QDate &) { montarFluxoCaixa(false); }

void InputDialogFinanceiro::on_doubleSpinBoxSt_valueChanged(const double valueSt) {
  unsetConnections();

  try {
    [=] {
      double total = 0;

      const auto list = ui->table->selectionModel()->selectedRows();

      for (const auto &index : list) { total += modelPedidoFornecedor2.data(index.row(), "preco").toDouble(); }

      const double aliquota = valueSt * 100 / total;

      ui->doubleSpinBoxAliquota->setValue(aliquota);

      for (const auto &index : list) { modelPedidoFornecedor2.setData(index.row(), "aliquotaSt", aliquota); }

      ui->doubleSpinBoxTotal->setValue(total);
    }();
  } catch (std::exception &) {}

  setConnections();

  montarFluxoCaixa();
}

void InputDialogFinanceiro::on_comboBoxST_currentTextChanged(const QString &text) {
  unsetConnections();

  try {
    [=] {
      if (text == "Sem ST") {
        ui->doubleSpinBoxSt->setValue(0);
        ui->doubleSpinBoxAliquota->setValue(0);

        ui->labelAliquota->hide();
        ui->doubleSpinBoxAliquota->hide();
        ui->labelSt->hide();
        ui->doubleSpinBoxSt->hide();
      }

      if (text == "ST Fornecedor" or text == "ST Loja") {
        ui->doubleSpinBoxAliquota->setValue(4.68);

        ui->labelAliquota->show();
        ui->doubleSpinBoxAliquota->show();
        ui->labelSt->show();
        ui->doubleSpinBoxSt->show();
        ui->dateEditPgtSt->show();
      }

      const auto list = ui->table->selectionModel()->selectedRows();

      for (const auto &index : list) {
        modelPedidoFornecedor2.setData(index.row(), "st", text);
        modelPedidoFornecedor2.setData(index.row(), "aliquotaSt", ui->doubleSpinBoxAliquota->value());
      }
    }();
  } catch (std::exception &) {}

  setConnections();

  montarFluxoCaixa();
}

void InputDialogFinanceiro::on_checkBoxParcelarSt_toggled(bool) { montarFluxoCaixa(); }

void InputDialogFinanceiro::on_lineEditCodFornecedor_textChanged(const QString &text) {
  const auto selection = ui->table->selectionModel()->selectedRows();

  for (auto &index : selection) { modelPedidoFornecedor2.setData(index.row(), "codFornecedor", text); }
}

void InputDialogFinanceiro::on_dateEditFrete_dateChanged(const QDate &) {
  const auto match = modelFluxoCaixa.match("tipo", "FRETE", 1, Qt::MatchExactly);

  if (match.isEmpty()) { return; }

  modelFluxoCaixa.setData(match.first().row(), "dataPagamento", ui->dateEditFrete->date());
}

void InputDialogFinanceiro::on_checkBoxDataFrete_toggled(bool checked) { ui->dateEditFrete->setEnabled(checked); }

// TODO: [Conrado] copiar de venda as verificacoes/terminar o codigo dos pagamentos
// TODO: refatorar o frame pagamentos para um widget para nao duplicar codigo

// TODO: 1quando for confirmacao de representacao perguntar qual o id para colocar na observacao das comissoes (codigo que vem do fornecedor)
// TODO: 3quando for representacao mostrar fluxo de comissao
// TODO: 3colocar possibilidade de ajustar valor total para as compras (contabilizar quanto de ajuste foi feito)
// TODO: gerar lancamentos da st por produto
// TODO: colocar checkbox para dizer se a ST vai ser na data do primeiro pagamento ou vai ser dividida junto com as parcelas

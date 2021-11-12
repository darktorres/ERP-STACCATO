#include "widgetgalpao.h"
#include "ui_widgetgalpao.h"

#include "application.h"
#include "logindialog.h"
#include "palletitem.h"
#include "sql.h"
#include "sqlquery.h"
#include "user.h"

#include <QDebug>
#include <QDrag>
#include <QElapsedTimer>
#include <QGraphicsProxyWidget>
#include <QGraphicsRectItem>
#include <QSqlError>

WidgetGalpao::WidgetGalpao(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetGalpao) { ui->setupUi(this); }

WidgetGalpao::~WidgetGalpao() { delete ui; }

void WidgetGalpao::resetTables() { modelIsSet = false; }

void WidgetGalpao::setConnections() {
  if (not blockingSignals.isEmpty()) { blockingSignals.pop(); } // avoid crashing on first setConnections

  if (not blockingSignals.isEmpty()) { return; } // delay setting connections until last unset/set block

  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->checkBoxConteudo, &QCheckBox::toggled, this, &WidgetGalpao::on_checkBoxConteudo_toggled, connectionType);
  connect(ui->checkBoxCriarPallet, &QCheckBox::toggled, this, &WidgetGalpao::on_checkBoxCriarPallet_toggled, connectionType);
  connect(ui->checkBoxEdicao, &QCheckBox::toggled, this, &WidgetGalpao::on_checkBoxEdicao_toggled, connectionType);
  connect(ui->checkBoxMoverPallet, &QCheckBox::toggled, this, &WidgetGalpao::on_checkBoxMoverPallet_toggled, connectionType);
  connect(ui->comboBoxPalletAtual, &QComboBox::currentTextChanged, this, &WidgetGalpao::on_comboBoxPalletAtual_currentTextChanged, connectionType);
  connect(ui->dateTimeEdit, &QDateTimeEdit::dateChanged, this, &WidgetGalpao::on_dateTimeEdit_dateChanged, connectionType);
  connect(ui->itemBoxVeiculo, &ItemBox::textChanged, this, &WidgetGalpao::on_itemBoxVeiculo_textChanged, connectionType);
  connect(ui->lineEditBuscaPallet, &QLineEdit::returnPressed, this, &WidgetGalpao::on_pushButtonBuscar_clicked, connectionType);
  connect(ui->lineEditMoverParaPallet, &QLineEdit::textChanged, this, &WidgetGalpao::on_lineEditMoverParaPallet_textChanged, connectionType);
  connect(ui->lineEditNomePallet, &QLineEdit::textChanged, this, &WidgetGalpao::on_lineEditNomePallet_textChanged, connectionType);
  connect(ui->pushButtonBuscar, &QPushButton::clicked, this, &WidgetGalpao::on_pushButtonBuscar_clicked, connectionType);
  connect(ui->pushButtonMover, &QPushButton::clicked, this, &WidgetGalpao::on_pushButtonMover_clicked, connectionType);
  connect(ui->pushButtonRemoverPallet, &QPushButton::clicked, this, &WidgetGalpao::on_pushButtonRemoverPallet_clicked, connectionType);
  connect(ui->pushButtonSalvarPallets, &QPushButton::clicked, this, &WidgetGalpao::on_pushButtonSalvarPallets_clicked, connectionType);
}

void WidgetGalpao::unsetConnections() {
  blockingSignals.push(0);

  disconnect(ui->checkBoxConteudo, &QCheckBox::toggled, this, &WidgetGalpao::on_checkBoxConteudo_toggled);
  disconnect(ui->checkBoxCriarPallet, &QCheckBox::toggled, this, &WidgetGalpao::on_checkBoxCriarPallet_toggled);
  disconnect(ui->checkBoxEdicao, &QCheckBox::toggled, this, &WidgetGalpao::on_checkBoxEdicao_toggled);
  disconnect(ui->checkBoxMoverPallet, &QCheckBox::toggled, this, &WidgetGalpao::on_checkBoxMoverPallet_toggled);
  disconnect(ui->comboBoxPalletAtual, &QComboBox::currentTextChanged, this, &WidgetGalpao::on_comboBoxPalletAtual_currentTextChanged);
  disconnect(ui->dateTimeEdit, &QDateTimeEdit::dateChanged, this, &WidgetGalpao::on_dateTimeEdit_dateChanged);
  disconnect(ui->itemBoxVeiculo, &ItemBox::textChanged, this, &WidgetGalpao::on_itemBoxVeiculo_textChanged);
  disconnect(ui->lineEditBuscaPallet, &QLineEdit::returnPressed, this, &WidgetGalpao::on_pushButtonBuscar_clicked);
  disconnect(ui->lineEditMoverParaPallet, &QLineEdit::textChanged, this, &WidgetGalpao::on_lineEditMoverParaPallet_textChanged);
  disconnect(ui->lineEditNomePallet, &QLineEdit::textChanged, this, &WidgetGalpao::on_lineEditNomePallet_textChanged);
  disconnect(ui->pushButtonBuscar, &QPushButton::clicked, this, &WidgetGalpao::on_pushButtonBuscar_clicked);
  disconnect(ui->pushButtonMover, &QPushButton::clicked, this, &WidgetGalpao::on_pushButtonMover_clicked);
  disconnect(ui->pushButtonRemoverPallet, &QPushButton::clicked, this, &WidgetGalpao::on_pushButtonRemoverPallet_clicked);
  disconnect(ui->pushButtonSalvarPallets, &QPushButton::clicked, this, &WidgetGalpao::on_pushButtonSalvarPallets_clicked);
}

void WidgetGalpao::updateTables() {
  if (not isSet) {
    ui->groupBoxEdicao->hide();
    ui->checkBoxCriarPallet->hide();
    ui->checkBoxMoverPallet->hide();
    ui->pushButtonRemoverPallet->hide();
    ui->pushButtonSalvarPallets->hide();
    ui->labelNomePallet->hide();
    ui->lineEditNomePallet->hide();

    // TODO: usar 2 scenes para não misturar pallets com estoques?
    // uma outra opção seria usar uma TableView no lugar do graphicsPallet

    //---------------------------
    ui->graphicsPallet->hide();
    ui->checkBoxConteudo->hide();
    ui->lineEditMoverParaPallet->hide();
    ui->labelAltura->hide();
    ui->spinBoxAltura->hide();
    //---------------------------

    scene = new QGraphicsScene(this);
    scene->setBackgroundBrush(Qt::white);

    auto *pixmapBackground = new QGraphicsPixmapItem(QPixmap("://novo_galpao2.png"));
    scene->addItem(pixmapBackground);

    ui->graphicsGalpao->setResizable(true);

    ui->graphicsGalpao->setScene(scene);
    ui->graphicsPallet->setScene(scene);

    ui->itemBoxVeiculo->setSearchDialog(SearchDialog::veiculo(this));
    ui->dateTimeEdit->setDate(qApp->serverDate());

    setConnections();

    ui->graphicsPallet->setSceneRect(pixmapBackground->boundingRect().width() + 70, 0, 842, 99999);
    ui->graphicsGalpao->setSceneRect(pixmapBackground->boundingRect());

    ui->graphicsGalpao->widgetGalpao = this;

    isSet = true;
  }

  if (not modelIsSet) {
    setupTables();
    modelIsSet = true;
  }

  modelTranspAgend.select();
  modelPallet.select(); // this one is slow due to view_estoque_contabil
  carregarPallets();
}

void WidgetGalpao::setupTables() {
  modelTranspAgend.setTable("veiculo_has_produto");

  modelTranspAgend.setHeaderData("data", "Agendado");
  modelTranspAgend.setHeaderData("idVenda", "Venda");
  modelTranspAgend.setHeaderData("status", "Status");
  modelTranspAgend.setHeaderData("fornecedor", "Fornecedor");
  modelTranspAgend.setHeaderData("produto", "Produto");
  modelTranspAgend.setHeaderData("caixas", "Cx.");
  modelTranspAgend.setHeaderData("kg", "Kg.");
  modelTranspAgend.setHeaderData("quant", "Quant.");
  modelTranspAgend.setHeaderData("un", "Un.");
  modelTranspAgend.setHeaderData("quantCaixa", "Quant./Cx.");
  modelTranspAgend.setHeaderData("codComercial", "Cód. Com.");
  modelTranspAgend.setHeaderData("formComercial", "Form. Com.");

  ui->tableTranspAgend->setModel(&modelTranspAgend);

  ui->tableTranspAgend->hideColumn("idEstoque");
  ui->tableTranspAgend->hideColumn("fotoEntrega");
  ui->tableTranspAgend->hideColumn("idVendaProduto1");
  ui->tableTranspAgend->hideColumn("idVendaProduto2");
  ui->tableTranspAgend->hideColumn("id");
  ui->tableTranspAgend->hideColumn("idEvento");
  ui->tableTranspAgend->hideColumn("idVeiculo");
  ui->tableTranspAgend->hideColumn("idCompra");
  ui->tableTranspAgend->hideColumn("idNFeSaida");
  ui->tableTranspAgend->hideColumn("idLoja");
  ui->tableTranspAgend->hideColumn("idProduto");
  ui->tableTranspAgend->hideColumn("obs");

  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->tableTranspAgend->selectionModel(), &QItemSelectionModel::selectionChanged, this, &WidgetGalpao::on_table_selectionChanged, connectionType);
}

void WidgetGalpao::carregarPallets() {
  if (isDirty) { return; }

  const QString palletSelecionado = ui->comboBoxPalletAtual->currentText();
  const QString palletDestino = ui->comboBoxMoverParaPallet->currentText();

  unsetConnections();

  ui->checkBoxCriarPallet->setChecked(false);
  ui->checkBoxMoverPallet->setChecked(false);

  const auto items = scene->items();

  for (auto *item : items) {
    if (auto *pallet = dynamic_cast<PalletItem *>(item)) { delete pallet; }
  }

  // 1. ler pallets do banco de dados e inserir na scene

  SqlQuery queryBlocos;

  if (not queryBlocos.exec("SELECT * FROM galpao ORDER BY LENGTH(label) DESC, label ASC")) { throw RuntimeError("Erro buscando pallets: " + queryBlocos.lastError().text(), this); }

  ui->comboBoxPalletAtual->clear();
  ui->comboBoxMoverParaPallet->clear();

  while (queryBlocos.next()) {
    const QString idBloco = queryBlocos.value("idBloco").toString();
    const QString label = queryBlocos.value("label").toString();

    const QStringList posicaoList = queryBlocos.value("posicao").toString().split(",");
    const QPointF posicao = QPointF(posicaoList.at(0).toDouble(), posicaoList.at(1).toDouble());

    const QStringList tamanhoList = queryBlocos.value("tamanho").toString().split(",");
    const QRectF tamanho = QRectF(0, 0, tamanhoList.at(0).toDouble(), tamanhoList.at(1).toDouble());

    auto *pallet = new PalletItem(idBloco, label, posicao, tamanho, ui->graphicsGalpao->sceneRect().width());
    pallet->setFlag(QGraphicsItem::ItemIsSelectable);

    connect(pallet, &PalletItem::save, this, &WidgetGalpao::salvarPallets);
    connect(pallet, &PalletItem::unselectOthers, this, &WidgetGalpao::unselectOthers);
    connect(pallet, &PalletItem::selectBloco, this, &WidgetGalpao::carregarBloco);
    connect(pallet, &PalletItem::unselectBloco, this, &WidgetGalpao::unselectBloco);

    //    palletsHash.insert(pallet->getIdBloco(), pallet);
    scene->addItem(pallet);

    // --------------------------------

    // TODO: guardar o idBloco junto do label
    ui->comboBoxPalletAtual->addItem(label);
    ui->comboBoxMoverParaPallet->addItem(label);
  }

  ui->comboBoxPalletAtual->insertItem(0, "Selecionar pallet...");
  ui->comboBoxMoverParaPallet->insertItem(0, "Mover para pallet...");

  ui->comboBoxPalletAtual->setCurrentIndex(0);
  ui->comboBoxMoverParaPallet->setCurrentIndex(0);

  // 2. ler conteudo dos pallets e inserir nos pallets existentes na scene

  //  SqlQuery query;

  //  if (not query.exec("SELECT "
  //                     "  g.idBloco, "
  //                     "  v.idEstoque_idConsumo, "
  //                     "  v.tipo, "
  //                     "  CAST(v.caixas AS DECIMAL(15, 2)) AS caixas, "
  //                     "  REPLACE(v.descricao, '-', '') AS descricao, "
  //                     "  v.idVendaProduto2 "
  //                     "FROM "
  //                     "  galpao g "
  //                     "LEFT JOIN "
  //                     "  view_galpao v ON g.idBloco = v.idBloco "
  //                     "WHERE "
  //                     "  idEstoque_idConsumo IS NOT NULL "
  //                     "ORDER BY CAST(g.idBloco AS UNSIGNED) ASC, idEstoque_idConsumo ASC")) {
  //    throw RuntimeError("Erro buscando dados do galpão: " + query.lastError().text(), this);
  //  }

  //  QHash<QString, QString> blocos;

  //  while (query.next()) {
  //    const QString idBloco = query.value("idBloco").toString();
  //    const QString idEstoque_idConsumo = query.value("idEstoque_idConsumo").toString();
  //    const QString tipo = query.value("tipo").toString();
  //    const QString caixas = QString::number(query.value("caixas").toDouble());
  //    const QString descricao = query.value("descricao").toString();
  //    const QString idVendaProduto2 = query.value("idVendaProduto2").toString();

  //    const QString item = idEstoque_idConsumo + " - " + tipo + " - " + caixas + "cx - " + descricao + " - " + idVendaProduto2;

  //    palletsHash.value(idBloco)->addEstoque(item);
  //  }

  if (palletSelecionado != "Selecionar pallet...") { ui->comboBoxPalletAtual->setCurrentText(palletSelecionado); }
  if (palletDestino != "Mover para pallet...") { ui->comboBoxMoverParaPallet->setCurrentText(palletDestino); }

  setConnections();
}

void WidgetGalpao::salvarPallets() {
  unselectBloco();

  qApp->startTransaction("WidgetGalpao::salvarPallets");

  isDirty = true;

  const auto items = scene->items();

  for (auto *item : items) {
    if (auto *pallet = dynamic_cast<PalletItem *>(item)) {
      if (pallet->getLabel().isEmpty()) { throw RuntimeError("Pallet sem nome! Cadastre um nome antes de salvar!"); }

      pallet->getIdBloco().isEmpty() ? inserirPallet(pallet) : atualizarPallet(pallet);
    }
  }

  isDirty = false;

  qApp->endTransaction();

  qApp->enqueueInformation("Pallets salvos com sucesso!", this);

  carregarPallets();
}

void WidgetGalpao::inserirPallet(PalletItem *pallet) {
  SqlQuery query;

  if (not query.exec("INSERT INTO galpao (label, posicao, tamanho) VALUES ('" + pallet->getLabel() + "', '" + pallet->getPosicao() + "', '" + pallet->getTamanho() + "')")) {
    throw RuntimeError("Erro salvando dados do galpão: " + query.lastError().text(), this);
  }
}

void WidgetGalpao::atualizarPallet(PalletItem *pallet) {
  SqlQuery query;

  if (not query.exec("UPDATE galpao SET label = '" + pallet->getLabel() + "', posicao = '" + pallet->getPosicao() + "', tamanho = '" + pallet->getTamanho() +
                     "' WHERE idBloco = " + pallet->getIdBloco())) {
    throw RuntimeError("Erro salvando dados do galpão: " + query.lastError().text(), this);
  }
}

void WidgetGalpao::on_dateTimeEdit_dateChanged() {
  if (ui->itemBoxVeiculo->text().isEmpty()) { return; }

  setFilter();
}

void WidgetGalpao::on_itemBoxVeiculo_textChanged() { setFilter(); }

void WidgetGalpao::setFilter() {
  modelTranspAgend.setFilter("idVeiculo = " + ui->itemBoxVeiculo->getId().toString() + " AND status != 'FINALIZADO' AND DATE(data) = '" + ui->dateTimeEdit->date().toString("yyyy-MM-dd") + "'");

  modelTranspAgend.select();

  on_table_selectionChanged();
}

void WidgetGalpao::on_table_selectionChanged() {
  const auto items = scene->items();

  for (auto *item : items) {
    if (auto *pallet = dynamic_cast<PalletItem *>(item)) { pallet->setFlagHighlight(false); }
  }

  const auto list = ui->tableTranspAgend->selectionModel()->selectedRows();

  for (auto index : list) {
    const int idVendaProduto2 = modelTranspAgend.data(index.row(), "idVendaProduto2").toInt();

    for (auto *item : items) {
      if (auto *pallet = dynamic_cast<PalletItem *>(item)) {
        auto palletItems = pallet->childItems();

        for (auto *palletItem : palletItems) {
          if (auto *estoque = dynamic_cast<EstoqueItem *>(palletItem)) {
            if (idVendaProduto2 == estoque->getIdVendaProduto2()) { pallet->setFlagHighlight(true); }
          }
        }
      }
    }
  }

  scene->update();
}

void WidgetGalpao::unselectOthers() {
  const auto items = scene->items();

  for (auto *item : items) {
    if (auto *pallet = dynamic_cast<PalletItem *>(item)) { pallet->unselect(); }
  }
}

void WidgetGalpao::on_pushButtonRemoverPallet_clicked() {
  if (not selectedIdBloco) { throw RuntimeError("Nenhum pallet selecionado!"); }

  if (modelPallet.rowCount() > 0) { throw RuntimeError("Pallet possui produtos! Transfira os produtos para outros pallets antes de remover!"); }

  qApp->startTransaction("WidgetGalpao::on_pushButtonRemoverPallet_clicked");

  SqlQuery query;

  if (not query.exec("DELETE FROM galpao WHERE idBloco = " + selectedIdBloco->getIdBloco())) { throw RuntimeException("Erro removendo pallet: " + query.lastError().text()); }

  delete selectedIdBloco;
  selectedIdBloco = nullptr;

  qApp->endTransaction();

  qApp->enqueueInformation("Pallet removido com sucesso!");
}

void WidgetGalpao::resizeEvent(QResizeEvent *event) {
  //  qDebug() << "WidgetGalpao::resizeEvent: " << event;

  QWidget::resizeEvent(event);
}

void WidgetGalpao::on_checkBoxCriarPallet_toggled(bool checked) {
  unselectBloco();

  if (ui->checkBoxMoverPallet->isChecked()) {
    unsetConnections();

    ui->checkBoxMoverPallet->setChecked(false);

    const auto items = scene->items();

    for (auto *item : items) {
      if (auto *pallet = dynamic_cast<PalletItem *>(item)) {
        pallet->setFlag(QGraphicsItem::ItemIsMovable, false);
        pallet->setFlag(QGraphicsItem::ItemIsSelectable, false);
        pallet->setFlag(QGraphicsItem::ItemIsFocusable, false);
      }
    }

    setConnections();
  }

  ui->graphicsGalpao->setIsEditable(checked);

  if (not checked and not ui->checkBoxMoverPallet->isChecked()) {
    const auto items = scene->items();

    for (auto *item : items) {
      if (auto *pallet = dynamic_cast<PalletItem *>(item)) { pallet->setFlag(QGraphicsItem::ItemIsSelectable, true); }
    }
  }
}

void WidgetGalpao::on_checkBoxMoverPallet_toggled(bool checked) {
  unselectBloco();

  if (ui->checkBoxCriarPallet->isChecked()) {
    unsetConnections();

    ui->checkBoxCriarPallet->setChecked(false);

    ui->graphicsGalpao->setIsEditable(false);

    setConnections();
  }

  const auto items = scene->items();

  for (auto *item : items) {
    if (auto *pallet = dynamic_cast<PalletItem *>(item)) {
      pallet->setFlag(QGraphicsItem::ItemIsMovable, checked);
      pallet->setFlag(QGraphicsItem::ItemIsSelectable, not checked);
      pallet->setFlag(QGraphicsItem::ItemIsFocusable, checked);
    }
  }

  if (checked) { unselectOthers(); }

  if (not checked and not ui->checkBoxCriarPallet->isChecked()) {
    const auto items = scene->items();

    for (auto *item : items) {
      if (auto *pallet = dynamic_cast<PalletItem *>(item)) { pallet->setFlag(QGraphicsItem::ItemIsSelectable, true); }
    }
  }
}

void WidgetGalpao::on_pushButtonSalvarPallets_clicked() { salvarPallets(); }

void WidgetGalpao::on_checkBoxConteudo_toggled(bool checked) { ui->graphicsPallet->setVisible(checked); }

void WidgetGalpao::carregarBloco() {
  selectedIdBloco = dynamic_cast<PalletItem *>(sender());

  if (not selectedIdBloco) { return; }

  //------------------------------
  unsetConnections();

  ui->comboBoxPalletAtual->setCurrentText(selectedIdBloco->getLabel());
  ui->lineEditNomePallet->setText(selectedIdBloco->getLabel());
  ui->lineEditNomePallet->setEnabled(true);

  ui->lineEditBuscaPallet->clear();

  setConnections();
  //------------------------------

  if (ui->groupBoxEdicao->isChecked()) { return; }

  ui->tabWidget->setCurrentIndex(1);

  if (not selectedIdBloco->getIdBloco().isEmpty()) {
    modelPallet.setQuery(Sql::view_galpao(selectedIdBloco->getIdBloco()));

    modelPallet.select();

    //  modelPallet.sort("idEstoque_idConsumo");

    modelPallet.setHeaderData("tipo", "Tipo");
    modelPallet.setHeaderData("caixas", "Cx.");
    modelPallet.setHeaderData("descricao", "Produto");
    modelPallet.setHeaderData("codComercial", "Código");
    modelPallet.setHeaderData("numeroNFe", "NFe");
    modelPallet.setHeaderData("lote", "Lote");
    modelPallet.setHeaderData("idVenda", "Pedido");

    ui->tablePallet->setModel(&modelPallet);

    ui->tablePallet->hideColumn("idBloco");
    ui->tablePallet->hideColumn("label");
    ui->tablePallet->hideColumn("idEstoque_idConsumo");
    ui->tablePallet->hideColumn("idVendaProduto2");

    //  ui->tablePallet->sortByColumn("idEstoque_idConsumo");
  }
}

void WidgetGalpao::unselectBloco() {
  if (not selectedIdBloco) { return; }

  unsetConnections();
  //----------------------

  //  ui->tabWidget->setCurrentIndex(0);
  ui->comboBoxPalletAtual->setCurrentIndex(0);

  modelPallet.setQuery("");
  modelPallet.select();
  ui->lineEditNomePallet->clear();
  ui->lineEditNomePallet->setDisabled(true);

  //----------------------
  setConnections();

  selectedIdBloco->unselect();
  selectedIdBloco = nullptr;
}

void WidgetGalpao::on_pushButtonMover_clicked() {
  if (ui->comboBoxPalletAtual->currentText() == "EM RECEBIMENTO") { throw RuntimeError("Não pode mover produtos ainda não recebidos!"); }

  if (ui->comboBoxPalletAtual->currentText() == "Selecionar pallet...") { throw RuntimeError("Selecione um pallet de origem!"); }

  if (ui->comboBoxMoverParaPallet->currentText() == "Mover para pallet..." /*and ui->lineEditMoverParaPallet->text().isEmpty()*/) { throw RuntimeError("Selecione um pallet de destino!"); }

  const auto selection = ui->tablePallet->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhum item selecionado!"); }

  qApp->startTransaction("on_pushButtonSalvarMover_clicked");

  const QString label = ui->comboBoxMoverParaPallet->currentText();

  for (auto index : selection) {
    const QString tipo = modelPallet.data(index.row(), "tipo").toString();
    const QString id = modelPallet.data(index.row(), "idEstoque_idConsumo").toString();

    SqlQuery query;

    if (tipo == "EST. LOJA") {
      if (not query.exec("UPDATE estoque SET idBloco = (SELECT idBloco FROM galpao WHERE label = '" + label + "') WHERE idEstoque = " + id)) {
        throw RuntimeError("Erro movendo itens: " + query.lastError().text());
      }
    }

    if (tipo == "CLIENTE") {
      if (not query.exec("UPDATE estoque_has_consumo SET idBloco = (SELECT idBloco FROM galpao WHERE label = '" + label + "') WHERE idConsumo = " + id)) {
        throw RuntimeError("Erro movendo itens: " + query.lastError().text());
      }
    }
  }

  qApp->endTransaction();

  modelPallet.select();

  qApp->enqueueInformation("Itens movidos com sucesso!");
}

void WidgetGalpao::on_lineEditMoverParaPallet_textChanged(const QString &text) {
  Q_UNUSED(text)

  // TODO: pintar de vermelho caso o pallet digitado não exista; pintar de verde caso exista
}

void WidgetGalpao::on_lineEditNomePallet_textChanged(const QString &text) {
  // TODO: verificar se o nome já existe para não ficar 2 pallets com o mesmo nome

  if (selectedIdBloco) { selectedIdBloco->setLabel(text); }
}

void WidgetGalpao::on_pushButtonBuscar_clicked() {
  const QString text = ui->lineEditBuscaPallet->text();

  if (text.isEmpty()) { return; }

  const QString idBloco = (selectedIdBloco) ? selectedIdBloco->getIdBloco() : "";

  modelPallet.setQuery(Sql::view_galpao(idBloco, text));

  modelPallet.select();

  //  modelPallet.sort("idEstoque_idConsumo");

  modelPallet.setHeaderData("label", "Pallet");
  modelPallet.setHeaderData("tipo", "Tipo");
  modelPallet.setHeaderData("caixas", "Cx.");
  modelPallet.setHeaderData("descricao", "Produto");
  modelPallet.setHeaderData("codComercial", "Código");
  modelPallet.setHeaderData("numeroNFe", "NFe");
  modelPallet.setHeaderData("lote", "Lote");
  modelPallet.setHeaderData("idVenda", "Pedido");

  ui->tablePallet->setModel(&modelPallet);

  if (not selectedIdBloco) { ui->tablePallet->showColumn("label"); }

  ui->tablePallet->hideColumn("idBloco");
  ui->tablePallet->hideColumn("idEstoque_idConsumo");
  ui->tablePallet->hideColumn("idVendaProduto2");

  //  ui->tablePallet->sortByColumn("idEstoque_idConsumo");
}

void WidgetGalpao::on_checkBoxEdicao_toggled(const bool checked) {
  if (checked and not User::isAdmin() and not User::isGerente()) {
    qApp->enqueueInformation("Necessário autorização de um gerente ou administrador!", this);

    LoginDialog dialog(LoginDialog::Tipo::Autorizacao, this);

    if (dialog.exec() != QDialog::Accepted) { return; }
  }

  // ----------------------------------------------

  ui->groupBoxEdicao->setVisible(checked);
  ui->checkBoxCriarPallet->setVisible(checked);
  ui->checkBoxMoverPallet->setVisible(checked);
  ui->pushButtonRemoverPallet->setVisible(checked);
  ui->pushButtonSalvarPallets->setVisible(checked);
  ui->labelNomePallet->setVisible(checked);
  ui->lineEditNomePallet->setVisible(checked);

  const auto items = scene->items();

  for (auto *item : items) {
    //      if (auto *pallet = dynamic_cast<PalletItem *>(item)) { pallet->setFlag(QGraphicsItem::ItemIsMovable, checked); }
    if (auto *pallet = dynamic_cast<PalletItem *>(item)) { pallet->setFlag(QGraphicsItem::ItemIsFocusable, checked); }
  }

  //  if (checked) { unselectOthers(); }
}

void WidgetGalpao::on_comboBoxPalletAtual_currentTextChanged(const QString &text) {
  unselectBloco();

  const auto items = scene->items();

  for (auto *item : items) {
    auto *pallet = dynamic_cast<PalletItem *>(item);

    if (not pallet) { continue; }

    if (pallet->getLabel() == text) {
      pallet->select();
      break;
    }
  }
}

// TODO: adicionar botao para criar pallet
// TODO: adicionar botao para remover pallet
// TODO: funcao de selecionar um caminhao e colorir todos os pallets correspondentes aos produtos agendados
// TODO: zoom
// TODO: guardar idEstoque em veiculo_has_produto
// TODO: listar os consumos que na venda esteja em 'estoque' e o restante dos estoques (livre)
// TODO: colocar nas permissoes de usuario uma coluna para 'galpao' para poder limitar quem ve essa aba
// TODO: quando marcar item entregue mudar bloco do consumo para fora dos pallets (usar um pallet invisivel ou apenas deixar vazio a coluna do bloco)

// TAREFAS:
// .implementar botão de quebra
// .implementar botão de fracionar para separar um produto em vários pallets (usar idRelacionado para vincular os estoques)
//      separar estoque em arvore?
// .implementar botão de ajustar quantidade, seja para mais ou para menos
// .arrumar view_estoque_contabil
// .colocar um botão de followup

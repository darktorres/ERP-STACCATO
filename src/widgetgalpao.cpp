#include "widgetgalpao.h"
#include "ui_widgetgalpao.h"

#include "acbrlib.h"
#include "application.h"
#include "followup.h"
#include "logindialog.h"
#include "sql.h"
#include "sqlquery.h"
#include "user.h"
#include "venda.h"

#include <QDebug>
#include <QGraphicsProxyWidget>
#include <QSqlError>

WidgetGalpao::WidgetGalpao(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetGalpao) { ui->setupUi(this); }

WidgetGalpao::~WidgetGalpao() { delete ui; }

void WidgetGalpao::resetTables() { modelIsSet = false; }

void WidgetGalpao::setConnections() {
  if (not blockingSignals.isEmpty()) { blockingSignals.pop(); } // avoid crashing on first setConnections

  if (not blockingSignals.isEmpty()) { return; } // delay setting connections until last unset/set block

  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

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
  connect(ui->pushButtonFollowup, &QPushButton::clicked, this, &WidgetGalpao::on_pushButtonFollowup_clicked, connectionType);
  connect(ui->pushButtonMover, &QPushButton::clicked, this, &WidgetGalpao::on_pushButtonMover_clicked, connectionType);
  connect(ui->pushButtonRemoverPallet, &QPushButton::clicked, this, &WidgetGalpao::on_pushButtonRemoverPallet_clicked, connectionType);
  connect(ui->pushButtonSalvarPallets, &QPushButton::clicked, this, &WidgetGalpao::on_pushButtonSalvarPallets_clicked, connectionType);
  connect(ui->tablePallet, &QTableView::doubleClicked, this, &WidgetGalpao::on_tablePallet_doubleClicked, connectionType);
  connect(ui->tableTranspAgend, &QTableView::doubleClicked, this, &WidgetGalpao::on_tableTranspAgend_doubleClicked, connectionType);
}

void WidgetGalpao::unsetConnections() {
  blockingSignals.push(0);

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
  disconnect(ui->pushButtonFollowup, &QPushButton::clicked, this, &WidgetGalpao::on_pushButtonFollowup_clicked);
  disconnect(ui->pushButtonMover, &QPushButton::clicked, this, &WidgetGalpao::on_pushButtonMover_clicked);
  disconnect(ui->pushButtonRemoverPallet, &QPushButton::clicked, this, &WidgetGalpao::on_pushButtonRemoverPallet_clicked);
  disconnect(ui->pushButtonSalvarPallets, &QPushButton::clicked, this, &WidgetGalpao::on_pushButtonSalvarPallets_clicked);
  disconnect(ui->tablePallet, &QTableView::doubleClicked, this, &WidgetGalpao::on_tablePallet_doubleClicked);
  disconnect(ui->tableTranspAgend, &QTableView::doubleClicked, this, &WidgetGalpao::on_tableTranspAgend_doubleClicked);
}

void WidgetGalpao::updateTables() {
  if (not isSet) {
    ui->groupBoxEdicao->hide();

    //---------------------------
    // TODO: re-enable these later
    ui->lineEditMoverParaPallet->hide();
    ui->labelAltura->hide();
    ui->spinBoxAltura->hide();
    //---------------------------

    scene = new QGraphicsScene(this);
    scene->setBackgroundBrush(Qt::white);

    auto *pixmapBackground = new QGraphicsPixmapItem(QPixmap("://novo_galpao2.png"));
    scene->addItem(pixmapBackground);

    ui->graphicsGalpao->setScene(scene);

    connect(ui->graphicsGalpao, &ViewGalpao::selectBloco, this, &WidgetGalpao::selectBloco);
    connect(ui->graphicsGalpao, &ViewGalpao::unselectBloco, this, &WidgetGalpao::unselectBloco);

    ui->itemBoxVeiculo->setSearchDialog(SearchDialog::veiculo(this));
    ui->dateTimeEdit->setDate(qApp->serverDate());

    setConnections();

    ui->graphicsGalpao->setSceneRect(pixmapBackground->boundingRect());

    isSet = true;
  }

  if (not modelIsSet) {
    setupTables();
    modelIsSet = true;
  }

  if (currentPallet) { return; }

  modelTranspAgend.select();
  modelPallet.select();
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

  connect(ui->tableTranspAgend->selectionModel(), &QItemSelectionModel::selectionChanged, this, &WidgetGalpao::on_tableTranspAgend_selectionChanged, connectionType);
}

void WidgetGalpao::carregarPallets() {
  const QString palletSelecionado = ui->comboBoxPalletAtual->currentText();
  const QString palletDestino = ui->comboBoxMoverParaPallet->currentText();

  ui->checkBoxCriarPallet->setChecked(false);
  ui->checkBoxMoverPallet->setChecked(false);

  unsetConnections();

  QString selectedLabel;

  if (currentPallet) {
    selectedLabel = currentPallet->getLabel();
    currentPallet = nullptr;
  }

  const auto items = scene->items();

  for (auto *item : items) {
    if (auto *pallet = dynamic_cast<PalletItem *>(item)) { delete pallet; }
  }

  // ------------------------------------------------

  SqlQuery queryBlocos;

  if (not queryBlocos.exec("SELECT * FROM galpao ORDER BY LENGTH(label) DESC, label ASC")) { throw RuntimeError("Erro buscando pallets: " + queryBlocos.lastError().text(), this); }

  ui->comboBoxPalletAtual->clear();
  ui->comboBoxMoverParaPallet->clear();

  while (queryBlocos.next()) {
    const QString idBloco = queryBlocos.value("idBloco").toString();
    const QString label = queryBlocos.value("label").toString();

    const QStringList posicaoList = queryBlocos.value("posicao").toString().split(",");
    const QPointF posicao = {posicaoList.at(0).toDouble(), posicaoList.at(1).toDouble()};

    const QStringList tamanhoList = queryBlocos.value("tamanho").toString().split(",");
    const QRectF tamanho = {0, 0, tamanhoList.at(0).toDouble(), tamanhoList.at(1).toDouble()};

    auto *pallet = new PalletItem(idBloco, label, posicao, tamanho);
    pallet->setFlag(QGraphicsItem::ItemIsSelectable);

    connect(pallet, &PalletItem::selectBloco, this, &WidgetGalpao::selectBloco);
    connect(pallet, &PalletItem::unselectBloco, this, &WidgetGalpao::unselectBloco);

    scene->addItem(pallet);

    // ------------------------------------------------

    ui->comboBoxPalletAtual->addItem(label, idBloco);
    ui->comboBoxMoverParaPallet->addItem(label, idBloco);
  }

  ui->comboBoxPalletAtual->insertItem(0, "Selecionar pallet...");
  ui->comboBoxMoverParaPallet->insertItem(0, "Mover para pallet...");

  ui->comboBoxPalletAtual->setCurrentIndex(0);
  ui->comboBoxMoverParaPallet->setCurrentIndex(0);

  if (palletSelecionado != "Selecionar pallet...") { ui->comboBoxPalletAtual->setCurrentText(palletSelecionado); }
  if (palletDestino != "Mover para pallet...") { ui->comboBoxMoverParaPallet->setCurrentText(palletDestino); }

  setConnections();

  // ------------------------------------------------

  const auto items2 = scene->items();

  for (auto *item : items2) {
    if (auto *pallet = dynamic_cast<PalletItem *>(item); pallet and pallet->getLabel() == selectedLabel) {
      pallet->select();
      break;
    }
  }
}

void WidgetGalpao::salvarPallets() {
  ui->checkBoxCriarPallet->setChecked(false);
  ui->checkBoxMoverPallet->setChecked(false);

  qApp->startTransaction("WidgetGalpao::salvarPallets");

  const auto items = scene->items();

  for (auto *item : items) {
    if (auto *pallet = dynamic_cast<PalletItem *>(item)) {
      if (pallet->getLabel().isEmpty()) {
        pallet->select();
        ui->graphicsGalpao->centerOn(pallet);
        throw RuntimeError("Pallet sem nome! Cadastre um nome antes de salvar!");
      }

      pallet->getIdBloco().isEmpty() ? inserirPallet(pallet) : atualizarPallet(pallet);
    }
  }

  qApp->endTransaction();

  unselectBloco();

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

  on_tableTranspAgend_selectionChanged();
}

void WidgetGalpao::on_tableTranspAgend_selectionChanged() {
  const auto items = scene->items();

  for (auto *item : items) {
    if (auto *pallet = dynamic_cast<PalletItem *>(item)) { pallet->setFlagHighlight(false); }
  }

  const auto list = ui->tableTranspAgend->selectionModel()->selectedRows();

  if (list.isEmpty()) { return scene->update(); }

  QStringList ids;

  for (const auto &index : list) { ids << modelTranspAgend.data(index.row(), "idVendaProduto2").toString(); }

  SqlQuery query;

  if (not query.exec("SELECT DISTINCT idBloco FROM estoque_has_consumo WHERE idVendaProduto2 IN (" + ids.join(", ") + ")")) { throw RuntimeException("Erro: " + query.lastError().text()); }

  while (query.next()) {
    for (auto *item : items) {
      const QString idBloco = query.value("idBloco").toString();

      if (auto *pallet = dynamic_cast<PalletItem *>(item); pallet and pallet->getIdBloco() == idBloco) { pallet->setFlagHighlight(true); }
    }
  }

  scene->update();
}

void WidgetGalpao::on_pushButtonRemoverPallet_clicked() {
  if (not currentPallet) { throw RuntimeError("Nenhum pallet selecionado!"); }

  if (modelPallet.rowCount() > 0) { throw RuntimeError("Pallet possui produtos! Transfira os produtos para outros pallets antes de remover!"); }

  if (not currentPallet->getIdBloco().isEmpty()) {
    qApp->startTransaction("WidgetGalpao::on_pushButtonRemoverPallet_clicked");

    SqlQuery query;

    if (not query.exec("DELETE FROM galpao WHERE idBloco = " + currentPallet->getIdBloco())) { throw RuntimeException("Erro removendo pallet: " + query.lastError().text()); }

    qApp->endTransaction();
  }

  delete currentPallet;
  unselectBloco();

  qApp->enqueueInformation("Pallet removido com sucesso!");
}

void WidgetGalpao::on_checkBoxCriarPallet_toggled(const bool checked) {
  unselectBloco();

  if (ui->checkBoxMoverPallet->isChecked()) {
    unsetConnections();

    ui->checkBoxMoverPallet->setChecked(false);

    setConnections();
  }

  ui->graphicsGalpao->setIsEditable(checked);

  if (not checked) {
    const auto items = scene->items();

    for (auto *item : items) {
      if (auto *pallet = dynamic_cast<PalletItem *>(item)) { pallet->setFlags(QGraphicsItem::ItemIsSelectable); }
    }
  }
}

void WidgetGalpao::on_checkBoxMoverPallet_toggled(const bool checked) {
  unselectBloco();

  if (ui->checkBoxCriarPallet->isChecked()) {
    unsetConnections();

    ui->checkBoxCriarPallet->setChecked(false);
    ui->graphicsGalpao->setIsEditable(false);

    setConnections();
  }

  const auto items = scene->items();

  for (auto *item : items) {
    if (auto *pallet = dynamic_cast<PalletItem *>(item)) { pallet->setFlags(checked ? QGraphicsItem::ItemIsMovable : QGraphicsItem::ItemIsSelectable); }
  }
}

void WidgetGalpao::on_pushButtonSalvarPallets_clicked() { salvarPallets(); }

void WidgetGalpao::selectBloco(PalletItem *const palletPtr) {
  //  qDebug() << "WidgetGalpao::selectBloco";

  if (not palletPtr) { return; }

  currentPallet = palletPtr;

  //------------------------------
  unsetConnections();

  ui->comboBoxPalletAtual->setCurrentText(currentPallet->getLabel());
  ui->lineEditNomePallet->setText(currentPallet->getLabel());

  ui->framePalletSelecionado->setEnabled(true);
  ui->frameEdicao->setDisabled(true);

  ui->lineEditBuscaPallet->clear();

  setConnections();
  //------------------------------

  if (ui->checkBoxEdicao->isChecked()) {
    // TODO: send a QEvent::RequestSoftwareInputPanel to open virtual keyboard
    ui->lineEditNomePallet->setFocus();
    return;
  }

  ui->tabWidget->setCurrentIndex(1);

  if (not currentPallet->getIdBloco().isEmpty()) {
    modelPallet.setQuery(Sql::view_galpao(currentPallet->getIdBloco()));

    modelPallet.select();

    //  modelPallet.sort("idEstoque_idConsumo");

    modelPallet.setHeaderData("tipo", "Tipo");
    modelPallet.setHeaderData("caixas", "Cx.");
    modelPallet.setHeaderData("descricao", "Produto");
    modelPallet.setHeaderData("codComercial", "Cód. Com.");
    modelPallet.setHeaderData("numeroNFe", "NFe");
    modelPallet.setHeaderData("lote", "Lote");
    modelPallet.setHeaderData("idVenda", "Venda");

    ui->tablePallet->setModel(&modelPallet);

    ui->tablePallet->hideColumn("idBloco");
    ui->tablePallet->hideColumn("idNFe");
    ui->tablePallet->hideColumn("label");
    ui->tablePallet->hideColumn("idEstoque_idConsumo");
    ui->tablePallet->hideColumn("idEstoque");
    ui->tablePallet->hideColumn("idVendaProduto2");

    //  ui->tablePallet->sortByColumn("idEstoque_idConsumo");
  }
}

void WidgetGalpao::unselectBloco() {
  //  qDebug() << "WidgetGalpao::unselectBloco";

  if (not currentPallet) { return; }

  unsetConnections();
  //----------------------

  ui->comboBoxPalletAtual->setCurrentIndex(0);

  modelPallet.setQuery("");
  modelPallet.select();

  ui->lineEditNomePallet->clear();

  ui->framePalletSelecionado->setDisabled(true);
  ui->frameEdicao->setEnabled(true);

  //----------------------
  setConnections();

  currentPallet->unselect();
  currentPallet = nullptr;
}

void WidgetGalpao::on_pushButtonMover_clicked() {
  if (ui->comboBoxPalletAtual->currentText() == "EM RECEBIMENTO") { throw RuntimeError("Não pode mover produtos ainda não recebidos!"); }

  if (ui->comboBoxMoverParaPallet->currentText() == "Mover para pallet..." /*and ui->lineEditMoverParaPallet->text().isEmpty()*/) { throw RuntimeError("Selecione um pallet de destino!"); }

  const auto selection = ui->tablePallet->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhum item selecionado!"); }

  const QString idBloco = ui->comboBoxMoverParaPallet->currentData().toString();

  if (idBloco.isEmpty()) { throw RuntimeException("idBloco vazio!"); }

  qApp->startTransaction("on_pushButtonSalvarMover_clicked");

  for (auto index : selection) {
    const QString tipo = modelPallet.data(index.row(), "tipo").toString();
    const QString id = modelPallet.data(index.row(), "idEstoque_idConsumo").toString();

    SqlQuery query;

    if (tipo == "EST. LOJA") {
      if (not query.exec("UPDATE estoque SET idBloco = " + idBloco + " WHERE idEstoque = " + id)) { throw RuntimeError("Erro movendo itens: " + query.lastError().text()); }
    }

    if (tipo == "CLIENTE") {
      if (not query.exec("UPDATE estoque_has_consumo SET idBloco = " + idBloco + " WHERE idConsumo = " + id)) { throw RuntimeError("Erro movendo itens: " + query.lastError().text()); }
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

  if (currentPallet) { currentPallet->setLabel(text); }
}

void WidgetGalpao::on_pushButtonBuscar_clicked() {
  const QString text = ui->lineEditBuscaPallet->text();

  const QString idBloco = (currentPallet) ? currentPallet->getIdBloco() : "";

  modelPallet.setQuery(Sql::view_galpao(idBloco, text));

  modelPallet.select();

  //  modelPallet.sort("idEstoque_idConsumo");

  modelPallet.setHeaderData("label", "Pallet");
  modelPallet.setHeaderData("tipo", "Tipo");
  modelPallet.setHeaderData("caixas", "Cx.");
  modelPallet.setHeaderData("descricao", "Produto");
  modelPallet.setHeaderData("codComercial", "Cód. Com.");
  modelPallet.setHeaderData("numeroNFe", "NFe");
  modelPallet.setHeaderData("lote", "Lote");
  modelPallet.setHeaderData("idVenda", "Venda");

  ui->tablePallet->setModel(&modelPallet);

  if (not currentPallet) { ui->tablePallet->showColumn("label"); }

  ui->tablePallet->hideColumn("idBloco");
  ui->tablePallet->hideColumn("idNFe");
  ui->tablePallet->hideColumn("idEstoque_idConsumo");
  ui->tablePallet->hideColumn("idEstoque");
  ui->tablePallet->hideColumn("idVendaProduto2");

  //  ui->tablePallet->sortByColumn("idEstoque_idConsumo");
}

void WidgetGalpao::on_checkBoxEdicao_toggled(const bool checked) {
  // TODO: o loginDialog de autorizacao aceita qualquer pessoa que seja do administrativo, refazer a lógica abaixo para considerar isso
  if (checked and not User::isAdmin() and not User::isGerente()) {
    qApp->enqueueInformation("Necessário autorização de um gerente ou administrador!", this);

    LoginDialog dialog(LoginDialog::Tipo::Autorizacao, this);

    if (dialog.exec() != QDialog::Accepted) { return; }
  }

  if (not checked) {
    // TODO: avisar usuario ou descartar alteracoes
    // TODO: auto salvar?

    unsetConnections();

    ui->checkBoxCriarPallet->setChecked(false);
    ui->checkBoxMoverPallet->setChecked(false);
    ui->graphicsGalpao->setIsEditable(false);

    const auto items = scene->items();

    for (auto *item : items) {
      if (auto *pallet = dynamic_cast<PalletItem *>(item)) { pallet->setFlags(QGraphicsItem::ItemIsSelectable); }
    }

    setConnections();
  }

  // ----------------------------------------------

  ui->frameAcessorio->setVisible(not checked);
  ui->groupBoxEdicao->setVisible(checked);
}

void WidgetGalpao::on_comboBoxPalletAtual_currentTextChanged() {
  const QString idBloco = ui->comboBoxPalletAtual->currentData().toString();

  if (idBloco.isEmpty()) { return unselectBloco(); }

  const auto items = scene->items();

  for (auto *item : items) {
    auto *pallet = dynamic_cast<PalletItem *>(item);

    if (not pallet) { continue; }

    if (pallet->getIdBloco() == idBloco) {
      pallet->select();
      break;
    }
  }
}

void WidgetGalpao::on_tableTranspAgend_doubleClicked(const QModelIndex &index) {
  const QString header = modelTranspAgend.headerData(index.column(), Qt::Horizontal).toString();

  if (header == "Venda") {
    auto *venda = new Venda(this);
    venda->setAttribute(Qt::WA_DeleteOnClose);
    venda->viewRegisterById(modelTranspAgend.data(index.row(), "idVenda"));
    venda->show();
  }
}

void WidgetGalpao::on_tablePallet_doubleClicked(const QModelIndex &index) {
  const QString header = modelPallet.headerData(index.column(), Qt::Horizontal).toString();

  if (header == "NFe") { ACBrLib::gerarDanfe(modelPallet.data(index.row(), "idNFe").toInt()); }

  if (header == "Venda") {
    QStringList ids = modelPallet.data(index.row(), "idVenda").toString().split(", ");

    for (const auto &id : ids) {
      auto *venda = new Venda(this);
      venda->setAttribute(Qt::WA_DeleteOnClose);
      venda->viewRegisterById(id);
      venda->show();
    }
  }
}

void WidgetGalpao::on_pushButtonFollowup_clicked() {
  const auto selection = ui->tablePallet->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeException("Nenhuma linha selecionada!"); }

  const QString idEstoque = modelPallet.data(selection.first().row(), "idEstoque").toString();

  auto *followup = new FollowUp(idEstoque, FollowUp::Tipo::Estoque, this);
  followup->setAttribute(Qt::WA_DeleteOnClose);
  followup->show();
}

// TODO: zoom por touch (e zoom por slider?)
// TODO: guardar idEstoque em veiculo_has_produto
// TODO: quando marcar item entregue mudar bloco do consumo para fora dos pallets (usar um pallet invisivel ou apenas deixar vazio a coluna do bloco)

// TAREFAS:
// .implementar botão de quebra
// .implementar botão de fracionar para separar um produto em vários pallets (usar idRelacionado para vincular os estoques)
// =====> separar estoque em arvore?
// .implementar botão de ajustar quantidade, seja para mais ou para menos
// .lidar com devoluções, marcar como 'entrada' ao devolver para estoque?
// .altura
// .ao selecionar linhas mostrar a soma das cxs.

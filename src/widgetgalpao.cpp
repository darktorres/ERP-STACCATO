#include "widgetgalpao.h"
#include "ui_widgetgalpao.h"

#if __has_include("lrreportengine.h")
#include "lrreportengine.h"
#endif

#include "application.h"
#include "file.h"
#include "followup.h"
#include "logindialog.h"
#include "sql.h"
#include "sqlquery.h"
#include "user.h"

#include <QAuthenticator>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QGraphicsProxyWidget>
#include <QSqlError>

WidgetGalpao::WidgetGalpao(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetGalpao) { ui->setupUi(this); }

WidgetGalpao::~WidgetGalpao() { delete ui; }

void WidgetGalpao::resetTables() { setupTables(); }

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
  connect(ui->pushButtonImprimir, &QPushButton::clicked, this, &WidgetGalpao::on_pushButtonImprimir_clicked, connectionType);
  connect(ui->pushButtonMover, &QPushButton::clicked, this, &WidgetGalpao::on_pushButtonMover_clicked, connectionType);
  connect(ui->pushButtonRemoverPallet, &QPushButton::clicked, this, &WidgetGalpao::on_pushButtonRemoverPallet_clicked, connectionType);
  connect(ui->pushButtonSalvarPallets, &QPushButton::clicked, this, &WidgetGalpao::on_pushButtonSalvarPallets_clicked, connectionType);
  connect(ui->pushButtonSelecionarMapa, &QPushButton::clicked, this, &WidgetGalpao::on_pushButtonSelecionarMapa_clicked, connectionType);
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
  disconnect(ui->pushButtonImprimir, &QPushButton::clicked, this, &WidgetGalpao::on_pushButtonImprimir_clicked);
  disconnect(ui->pushButtonMover, &QPushButton::clicked, this, &WidgetGalpao::on_pushButtonMover_clicked);
  disconnect(ui->pushButtonRemoverPallet, &QPushButton::clicked, this, &WidgetGalpao::on_pushButtonRemoverPallet_clicked);
  disconnect(ui->pushButtonSalvarPallets, &QPushButton::clicked, this, &WidgetGalpao::on_pushButtonSalvarPallets_clicked);
  disconnect(ui->pushButtonSelecionarMapa, &QPushButton::clicked, this, &WidgetGalpao::on_pushButtonSelecionarMapa_clicked);
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

    ui->graphicsGalpao->setScene(scene);

    connect(ui->graphicsGalpao, &ViewGalpao::selectBloco, this, &WidgetGalpao::selectBloco);
    connect(ui->graphicsGalpao, &ViewGalpao::unselectBloco, this, &WidgetGalpao::unselectBloco);

    ui->itemBoxVeiculo->setSearchDialog(SearchDialog::veiculo(this));
    ui->dateTimeEdit->setDate(qApp->serverDate());

    //----------------------------------

    const QString ip = qApp->getWebDavIp();
    const QString url = "https://" + ip + "/webdav/MAPA GALPAO/mapa.png";

    auto *manager = new QNetworkAccessManager(this);
    manager->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);

    connect(manager, &QNetworkAccessManager::authenticationRequired, this, [&](QNetworkReply *reply, QAuthenticator *authenticator) {
      Q_UNUSED(reply)

      authenticator->setUser(User::usuario);
      authenticator->setPassword(User::senha);
    });

    auto *reply = manager->get(QNetworkRequest(QUrl(url)));

    connect(reply, &QNetworkReply::finished, this, [=, this] {
      if (reply->error() != QNetworkReply::NoError) { return; }

      QByteArray imageData = reply->readAll();

      QPixmap pixmap;
      pixmap.loadFromData(imageData);

      if (!pixmap.isNull()) {
        auto *pixmapBackground = new QGraphicsPixmapItem(pixmap);
        pixmapBackground->setZValue(-1);
        scene->addItem(pixmapBackground);
        ui->graphicsGalpao->setSceneRect(pixmapBackground->boundingRect());
      }
    });

    setupTables();

    setConnections();
    isSet = true;
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
  modelTranspAgend.setHeaderData("observacao", "Observação");
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

  //----------------------

  modelPallet.setQuery("");

  ui->tablePallet->setModel(&modelPallet);
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

  const auto selection = ui->tableTranspAgend->selectionModel()->selectedRows();

  if (selection.isEmpty()) { return scene->update(); }

  QStringList ids;

  for (const auto &index : selection) { ids << modelTranspAgend.data(index.row(), "idVendaProduto2").toString(); }

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
    modelPallet.setHeaderData("formComercial", "Formato");
    modelPallet.setHeaderData("codComercial", "Cód. Com.");
    modelPallet.setHeaderData("numeroNFe", "NF-e");
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
  modelPallet.setHeaderData("numeroNFe", "NF-e");
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
  if (checked and not User::temPermissao("ajusteFrete")) {
    qApp->enqueueInformation("Necessário autorização do administrativo!", this);

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
  if (not index.isValid()) { return; }

  const QString header = modelTranspAgend.headerData(index.column(), Qt::Horizontal).toString();

  if (header == "Venda") { return qApp->abrirVenda(modelTranspAgend.data(index.row(), "idVenda")); }
}

void WidgetGalpao::on_tablePallet_doubleClicked(const QModelIndex &index) {
  if (not index.isValid()) { return; }

  const QString header = modelPallet.headerData(index.column(), Qt::Horizontal).toString();

  if (header == "NF-e") { return qApp->abrirNFe(modelPallet.data(index.row(), "idNFe")); }

  if (header == "Venda") {
    const QStringList ids = modelPallet.data(index.row(), "idVenda").toString().split(", ");

    for (const auto &id : ids) { qApp->abrirVenda(id); }

    return;
  }
}

void WidgetGalpao::on_pushButtonFollowup_clicked() {
  const auto selection = ui->tablePallet->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!"); }

  const QString idEstoque = modelPallet.data(selection.first().row(), "idEstoque").toString();

  auto *followup = new FollowUp(idEstoque, FollowUp::Tipo::Estoque, this);
  followup->setAttribute(Qt::WA_DeleteOnClose);
  followup->show();
}

void WidgetGalpao::on_pushButtonImprimir_clicked() {
#if __has_include("lrreportengine.h")
  const auto selection = ui->tablePallet->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!"); }

  // ------------------------------------------------------

  for (auto index : selection) {
    // TODO: imprimir varias etiquetas na mesma folha

    const int row = index.row();

    //  const int row = selection.first().row();

    // TODO: usar um foreach para imprimir varios pallets de uma vez?

    LimeReport::ReportEngine report;
    auto *dataManager = report.dataManager();

    const QString modelo = QDir::currentPath() + "/modelos/pallet.lrxml";

    if (not report.loadFromFile(modelo)) { throw RuntimeException("Não encontrou o modelo de impressão!"); }

    // set idVenda

    QString idVenda = modelPallet.data(row, "idVenda").toString();

    if (idVenda.isEmpty()) { idVenda = "EST. LOJA"; }

    dataManager->setReportVariable("idVenda", idVenda);

    // set produtos

    const QString produto = modelPallet.data(row, "descricao").toString() + " " + modelPallet.data(row, "formComercial").toString();
    const QString caixas = modelPallet.data(row, "caixas").toString();

    dataManager->setReportVariable("produto", produto);
    dataManager->setReportVariable("caixas", caixas);
    dataManager->setReportVariable("nfe", modelPallet.data(row, "numeroNFe").toInt());

    // TODO: falta colocar a quantidade total do produto que veio na NF-e

    const QString idEstoque = modelPallet.data(row, "idEstoque").toString();

    // ------------------------------------------------------

    const QString fileName = QDir::currentPath() + "/pallet_" + idEstoque + ".pdf";

    File file(fileName);

    if (not file.open(QFile::WriteOnly)) { throw RuntimeError("Não foi possível abrir o arquivo '" + fileName + "' para escrita: " + file.errorString()); }

    file.close();

    if (not report.printToPDF(fileName)) { throw RuntimeException("Erro gerando PDF: " + report.lastError()); }

    if (not QDesktopServices::openUrl(QUrl::fromLocalFile(fileName))) { throw RuntimeException("Erro abrindo arquivo: " + QDir::currentPath() + fileName); }
  }

#else
  qApp->enqueueWarning("LimeReport desativado!");
#endif
}

void WidgetGalpao::on_pushButtonSelecionarMapa_clicked()
{
  const QString filePath = QFileDialog::getOpenFileName(this, "Imagens", "", "(*.jpg *.jpeg *.png *.tif *.bmp *.pdf)");

  if (filePath.isEmpty()) { return; }

  File file(filePath);

  if (not file.open(QFile::ReadOnly)) { throw RuntimeException("Erro lendo arquivo: " + file.errorString(), this); }

  auto *manager = new QNetworkAccessManager(this);
  manager->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);

  connect(manager, &QNetworkAccessManager::authenticationRequired, this, [&](QNetworkReply *reply, QAuthenticator *authenticator) {
    Q_UNUSED(reply)

    authenticator->setUser(User::usuario);
    authenticator->setPassword(User::senha);
  });

  const QString ip = qApp->getWebDavIp();

  QFileInfo info(file);

  const QString extension = info.suffix();

//  const QString url = "https://" + ip + "/webdav/MAPA GALPAO/mapa." + extension;
  const QString url = "https://" + ip + "/webdav/MAPA GALPAO/mapa.png";

  const auto fileContent = file.readAll();

  manager->put(QNetworkRequest(QUrl(url)), fileContent);

  ui->lineEditMapa->setText("Enviando...");

  connect(manager, &QNetworkAccessManager::finished, this, [=, this](QNetworkReply *reply) {
    const QUrl redirect = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();

    if (redirect.isValid()) {
      manager->put(QNetworkRequest(redirect), fileContent);
      return;
    }

    if (reply->error() != QNetworkReply::NoError) {
      ui->lineEditMapa->setStyleSheet("background-color: rgb(255, 0, 0); color: rgb(0, 0, 0);");
      throw RuntimeException("Erro enviando foto: " + reply->errorString());
    }

    ui->lineEditMapa->setText(reply->url().toString());
    ui->lineEditMapa->setStyleSheet("background-color: rgb(0, 255, 0); color: rgb(0, 0, 0);");

    ui->checkBoxEdicao->setChecked(false);
    isSet = false;
    updateTables();
  });
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

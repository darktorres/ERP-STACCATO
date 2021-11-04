#include "widgetgalpao.h"
#include "ui_widgetgalpao.h"

#include "application.h"
#include "palletitem.h"
#include "sqlquery.h"

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
  connect(ui->checkBoxCriarApagar, &QCheckBox::toggled, this, &WidgetGalpao::on_checkBoxCriarApagar_toggled, connectionType);
  connect(ui->checkBoxMover, &QCheckBox::toggled, this, &WidgetGalpao::on_checkBoxMover_toggled, connectionType);
  connect(ui->dateTimeEdit, &QDateTimeEdit::dateChanged, this, &WidgetGalpao::on_dateTimeEdit_dateChanged, connectionType);
  connect(ui->groupBoxEdicao, &QGroupBox::toggled, this, &WidgetGalpao::on_groupBoxEdicao_toggled, connectionType);
  connect(ui->itemBoxVeiculo, &ItemBox::textChanged, this, &WidgetGalpao::on_itemBoxVeiculo_textChanged, connectionType);
  connect(ui->pushButtonCriarPallet, &QPushButton::clicked, this, &WidgetGalpao::on_pushButtonCriarPallet_clicked, connectionType);
  connect(ui->pushButtonRemoverPallet, &QPushButton::clicked, this, &WidgetGalpao::on_pushButtonRemoverPallet_clicked, connectionType);
  connect(ui->pushButtonSalvar, &QPushButton::clicked, this, &WidgetGalpao::on_pushButtonSalvar_clicked, connectionType);
}

void WidgetGalpao::unsetConnections() {
  blockingSignals.push(0);

  disconnect(ui->checkBoxConteudo, &QCheckBox::toggled, this, &WidgetGalpao::on_checkBoxConteudo_toggled);
  disconnect(ui->checkBoxCriarApagar, &QCheckBox::toggled, this, &WidgetGalpao::on_checkBoxCriarApagar_toggled);
  disconnect(ui->checkBoxMover, &QCheckBox::toggled, this, &WidgetGalpao::on_checkBoxMover_toggled);
  disconnect(ui->dateTimeEdit, &QDateTimeEdit::dateChanged, this, &WidgetGalpao::on_dateTimeEdit_dateChanged);
  disconnect(ui->groupBoxEdicao, &QGroupBox::toggled, this, &WidgetGalpao::on_groupBoxEdicao_toggled);
  disconnect(ui->itemBoxVeiculo, &ItemBox::textChanged, this, &WidgetGalpao::on_itemBoxVeiculo_textChanged);
  disconnect(ui->pushButtonCriarPallet, &QPushButton::clicked, this, &WidgetGalpao::on_pushButtonCriarPallet_clicked);
  disconnect(ui->pushButtonRemoverPallet, &QPushButton::clicked, this, &WidgetGalpao::on_pushButtonRemoverPallet_clicked);
  disconnect(ui->pushButtonSalvar, &QPushButton::clicked, this, &WidgetGalpao::on_pushButtonSalvar_clicked);
}

void WidgetGalpao::updateTables() {
  if (not isSet) {
    ui->groupBoxEdicao->hide();

    // TODO: usar 2 scenes para não misturar pallets com estoques?
    // uma outra opção seria usar uma TableView no lugar do graphicsPallet

    ui->graphicsPallet->hide();

    scene = new QGraphicsScene(this);
    scene->setBackgroundBrush(Qt::white);

    auto pixmapBackground = new QGraphicsPixmapItem(QPixmap("://novo_galpao2.png"));
    scene->addItem(pixmapBackground);

    ui->graphicsGalpao->setResizable(true);

    ui->graphicsGalpao->setScene(scene);
    ui->graphicsPallet->setScene(scene);

    ui->itemBoxVeiculo->setSearchDialog(SearchDialog::veiculo(this));
    ui->dateTimeEdit->setDate(qApp->serverDate());

    setConnections();

    //    ui->graphicsGalpao->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    //    ui->graphicsGalpao->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    ui->graphicsPallet->setSceneRect(pixmapBackground->boundingRect().width() + 70, 0, 842, 99999);
    ui->graphicsGalpao->setSceneRect(pixmapBackground->boundingRect());

    isSet = true;
  }

  if (not modelIsSet) {
    setupTables();
    modelIsSet = true;
  }

  modelTranspAgend.select();

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
  unsetConnections();

  ui->checkBoxCriarApagar->setChecked(false);
  ui->checkBoxMover->setChecked(false);

  setConnections();

  const auto items = scene->items();

  for (auto *item : items) {
    if (auto *pallet = dynamic_cast<PalletItem *>(item)) { delete pallet; }
  }

  // 1. ler pallets do banco de dados e inserir na scene
  // 2. ler conteudo dos pallets e inserir nos pallets existentes na scene

  // 1
  SqlQuery queryBlocos;

  if (not queryBlocos.exec("SELECT * FROM galpao")) { throw RuntimeError("Erro buscando pallets: " + queryBlocos.lastError().text(), this); }

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

    palletsHash.insert(pallet->getIdBloco(), pallet);
    scene->addItem(pallet);
  }

  // 2

  SqlQuery query;

  if (not query.exec("SELECT "
                     "  g.idBloco, "
                     "  v.idEstoque_idConsumo, "
                     "  v.tipo, "
                     "  CAST(v.caixas AS DECIMAL(15, 2)) AS caixas, "
                     "  REPLACE(v.descricao, '-', '') AS descricao, "
                     "  v.idVendaProduto2 "
                     "FROM "
                     "  galpao g "
                     "LEFT JOIN "
                     "  view_galpao v ON g.idBloco = v.idBloco "
                     "WHERE "
                     "  idEstoque_idConsumo IS NOT NULL "
                     "ORDER BY CAST(g.idBloco AS UNSIGNED) ASC, idEstoque_idConsumo ASC")) {
    throw RuntimeError("Erro buscando dados do galpão: " + query.lastError().text(), this);
  }

  QHash<QString, QString> blocos;

  while (query.next()) {
    const QString idBloco = query.value("idBloco").toString();
    const QString idEstoque_idConsumo = query.value("idEstoque_idConsumo").toString();
    const QString tipo = query.value("tipo").toString();
    const QString caixas = QString::number(query.value("caixas").toDouble());
    const QString descricao = query.value("descricao").toString();
    const QString idVendaProduto2 = query.value("idVendaProduto2").toString();

    const QString item = idEstoque_idConsumo + " - " + tipo + " - " + caixas + "cx - " + descricao + " - " + idVendaProduto2;

    palletsHash.value(idBloco)->addEstoque(item);
  }
}

void WidgetGalpao::salvarPallets() {
  const auto items = scene->items();

  SqlQuery query;

  for (auto *item : items) {
    if (auto *pallet = dynamic_cast<PalletItem *>(item)) {
      if (pallet->getIdBloco().isEmpty()) {
        if (not query.exec("INSERT INTO galpao (label, posicao, tamanho) VALUES ('" + pallet->getLabel() + "', '" + pallet->getPosicao() + "', '" + pallet->getTamanho() + "')")) {
          throw RuntimeError("Erro salvando dados do galpão: " + query.lastError().text(), this);
        }
      } else {
        if (not query.exec("UPDATE galpao SET label = '" + pallet->getLabel() + "', posicao = '" + pallet->getPosicao() + "', tamanho = '" + pallet->getTamanho() +
                           "' WHERE idBloco = " + pallet->getIdBloco())) {
          throw RuntimeError("Erro salvando dados do galpão: " + query.lastError().text(), this);
        }
      }
    }
  }

  qApp->enqueueInformation("Pallets salvos com sucesso!", this);
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

void WidgetGalpao::on_pushButtonCriarPallet_clicked() {
  // 1. on mousePressEvent store the mouse position
  // 2. on mouseMoveEvent store the second position and draw the rectangle
  // 3. for every subsequent mouseMoveEvent redo step 2
  // 4. on mouseReleaseEvent store the positions on a PalletItem
}

void WidgetGalpao::on_pushButtonRemoverPallet_clicked() {
  //
}

void WidgetGalpao::on_groupBoxEdicao_toggled(const bool checked) {
  const auto items = scene->items();

  for (auto *item : items) {
    if (auto *pallet = dynamic_cast<PalletItem *>(item)) { pallet->setFlag(QGraphicsItem::ItemIsMovable, checked); }
  }

  if (checked) { unselectOthers(); }

  ui->graphicsGalpao->setIsEditable(checked);

  ui->pushButtonCriarPallet->setDisabled(true);
  ui->pushButtonRemoverPallet->setDisabled(true);
}

void WidgetGalpao::resizeEvent(QResizeEvent *event) {
  //  qDebug() << "widget resize: " << event;

  QWidget::resizeEvent(event);
}

void WidgetGalpao::on_checkBoxCriarApagar_toggled(bool checked) {
  if (ui->checkBoxMover->isChecked()) {
    unsetConnections();

    ui->checkBoxMover->setChecked(false);

    const auto items = scene->items();

    for (auto *item : items) {
      if (auto *pallet = dynamic_cast<PalletItem *>(item)) {
        pallet->setFlag(QGraphicsItem::ItemIsMovable, false);
        pallet->setFlag(QGraphicsItem::ItemIsSelectable, false);
      }
    }

    setConnections();
  }

  ui->graphicsGalpao->setIsEditable(checked);

  if (not checked and not ui->checkBoxMover->isChecked()) {
    const auto items = scene->items();

    for (auto *item : items) {
      if (auto *pallet = dynamic_cast<PalletItem *>(item)) { pallet->setFlag(QGraphicsItem::ItemIsSelectable, true); }
    }
  }
}

void WidgetGalpao::on_checkBoxMover_toggled(bool checked) {
  if (ui->checkBoxCriarApagar->isChecked()) {
    unsetConnections();

    ui->checkBoxCriarApagar->setChecked(false);

    ui->graphicsGalpao->setIsEditable(false);

    setConnections();
  }

  const auto items = scene->items();

  for (auto *item : items) {
    if (auto *pallet = dynamic_cast<PalletItem *>(item)) {
      pallet->setFlag(QGraphicsItem::ItemIsMovable, checked);
      pallet->setFlag(QGraphicsItem::ItemIsSelectable, not checked);
    }
  }

  if (checked) { unselectOthers(); }

  if (not checked and not ui->checkBoxCriarApagar->isChecked()) {
    const auto items = scene->items();

    for (auto *item : items) {
      if (auto *pallet = dynamic_cast<PalletItem *>(item)) { pallet->setFlag(QGraphicsItem::ItemIsSelectable, true); }
    }
  }
}

void WidgetGalpao::on_pushButtonSalvar_clicked() { salvarPallets(); }

void WidgetGalpao::on_checkBoxConteudo_toggled(bool checked) { ui->graphicsPallet->setVisible(checked); }

// TODO: adicionar botao para criar pallet
// TODO: adicionar botao para remover pallet
// TODO: funcao de selecionar um caminhao e colorir todos os pallets correspondentes aos produtos agendados
// TODO: zoom
// TODO: guardar idEstoque em veiculo_has_produto
// TODO: listar os consumos que na venda esteja em 'estoque' e o restante dos estoques (livre)
// TODO: colocar nas permissoes de usuario uma coluna para 'galpao' para poder limitar quem ve essa aba
// TODO: quando marcar item entregue mudar bloco do consumo para fora dos pallets (usar um pallet invisivel ou apenas deixar vazio a coluna do bloco)

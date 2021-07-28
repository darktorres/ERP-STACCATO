#include "widgetgalpao.h"
#include "ui_widgetgalpao.h"

#include "application.h"
#include "palletitem.h"
#include "sqlquery.h"

#include <QDebug>
#include <QDrag>
#include <QGraphicsProxyWidget>
#include <QGraphicsRectItem>
#include <QSqlError>

WidgetGalpao::WidgetGalpao(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetGalpao) { ui->setupUi(this); }

WidgetGalpao::~WidgetGalpao() { delete ui; }

void WidgetGalpao::resetTables() { modelIsSet = false; }

void WidgetGalpao::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->dateTimeEdit, &QDateTimeEdit::dateChanged, this, &WidgetGalpao::on_dateTimeEdit_dateChanged, connectionType);
  connect(ui->groupBoxEdicao, &QGroupBox::toggled, this, &WidgetGalpao::on_groupBoxEdicao_toggled, connectionType);
  connect(ui->itemBoxVeiculo, &ItemBox::textChanged, this, &WidgetGalpao::on_itemBoxVeiculo_textChanged, connectionType);
  connect(ui->pushButtonCriarPallet, &QPushButton::clicked, this, &WidgetGalpao::on_pushButtonCriarPallet_clicked, connectionType);
  connect(ui->pushButtonRemoverPallet, &QPushButton::clicked, this, &WidgetGalpao::on_pushButtonRemoverPallet_clicked, connectionType);
}

void WidgetGalpao::updateTables() {
  if (not isSet) {
    scene = new QGraphicsScene(this);
    ui->graphicsGalpao->setScene(scene);
    ui->graphicsPallet->setScene(scene);

    ui->itemBoxVeiculo->setSearchDialog(SearchDialog::veiculo(this));
    ui->dateTimeEdit->setDate(qApp->serverDate());

    setConnections();

    ui->graphicsGalpao->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->graphicsGalpao->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    ui->graphicsPallet->setSceneRect(650, 0, 842, 10000);

    ui->graphicsGalpao->setSceneRect(0, 0, 624, 586);

    ui->graphicsGalpao->fitInView(0, 0, 624, 586, Qt::KeepAspectRatio);

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
  scene->clear();

  scene->addItem(new QGraphicsPixmapItem(QPixmap("://galpao2.png")));

  SqlQuery query;

  if (not query.exec("SELECT g.*, v.id, v.tipo, CAST(v.caixas AS DECIMAL(15, 2)) AS caixas, REPLACE(v.descricao, '-', '') AS descricao, v.idVendaProduto2 FROM galpao g LEFT JOIN view_galpao v ON "
                     "g.bloco = v.bloco ORDER BY CAST(g.bloco AS UNSIGNED) ASC, id ASC")) {
    throw RuntimeError("Erro buscando dados do galpão: " + query.lastError().text(), this);
  }

  QHash<QString, QString> blocos;

  while (query.next()) {
    // agrupar valores de cada bloco

    const QString posicao = query.value("posicao").toString();
    const QString tamanho = query.value("tamanho").toString();

    QString item = query.value("id").toString() + " - " + query.value("tipo").toString() + " - " + QString::number(query.value("caixas").toDouble()) + "cx - " + query.value("descricao").toString() +
                   " - " + query.value("idVendaProduto2").toString();

    QString existente = blocos.value(query.value("bloco").toString());

    if (existente.isEmpty()) {
      existente += posicao;
      existente += "\n";
      existente += tamanho;
    }

    if (query.value("id").toInt() > 0) {
      if (not existente.isEmpty()) { existente += "\n"; }

      existente += item;
    }

    blocos.insert(query.value("bloco").toString(), existente);
  }

  auto iterator = blocos.constBegin();

  while (iterator != blocos.constEnd()) {
    QStringList strings = iterator.value().split("\n");

    const QStringList posicao = strings.at(0).split(",");
    const QStringList tamanho = strings.at(1).split(",");

    strings.removeFirst();
    strings.removeFirst();

    auto *pallet = new PalletItem(QRect(0, 0, tamanho.at(0).toDouble(), tamanho.at(1).toDouble()));
    pallet->setPos(posicao.at(0).toDouble(), posicao.at(1).toDouble());
    pallet->setLabel(iterator.key());
    pallet->setText(strings.join("\n"));

    connect(pallet, &PalletItem::save, this, &WidgetGalpao::salvarPallets);
    connect(pallet, &PalletItem::unselectOthers, this, &WidgetGalpao::unselectOthers);

    scene->addItem(pallet);

    ++iterator;
  }
}

void WidgetGalpao::salvarPallets() {
  auto items = scene->items();

  for (auto item : items) {
    if (auto pallet = dynamic_cast<PalletItem *>(item)) {
      QString pos = QString::number(pallet->scenePos().x()) + "," + QString::number(pallet->scenePos().y());

      SqlQuery query;

      if (not query.exec("UPDATE galpao SET posicao = '" + pos + "' WHERE bloco = '" + pallet->getLabel() + "'")) {
        throw RuntimeError("Erro salvando dados do galpão: " + query.lastError().text(), this);
      }
    }
  }
}

void WidgetGalpao::on_dateTimeEdit_dateChanged(const QDate) {
  if (ui->itemBoxVeiculo->text().isEmpty()) { return; }

  setFilter();
}

void WidgetGalpao::on_itemBoxVeiculo_textChanged(const QString &) { setFilter(); }

void WidgetGalpao::setFilter() {
  modelTranspAgend.setFilter("idVeiculo = " + ui->itemBoxVeiculo->getId().toString() + " AND status != 'FINALIZADO' AND DATE(data) = '" + ui->dateTimeEdit->date().toString("yyyy-MM-dd") + "'");

  modelTranspAgend.select();

  on_table_selectionChanged();
}

void WidgetGalpao::on_table_selectionChanged() {
  auto items = scene->items();

  for (auto item : items) {
    if (auto pallet = dynamic_cast<PalletItem *>(item)) { pallet->setFlagHighlight(false); }
  }

  const auto list = ui->tableTranspAgend->selectionModel()->selectedRows();

  for (auto index : list) {
    const int idVendaProduto2 = modelTranspAgend.data(index.row(), "idVendaProduto2").toInt();

    for (auto item : items) {
      if (auto pallet = dynamic_cast<PalletItem *>(item)) {
        auto palletItems = pallet->childItems();

        for (auto palletItem : palletItems) {
          if (auto estoque = dynamic_cast<EstoqueItem *>(palletItem)) {
            if (idVendaProduto2 == estoque->idVendaProduto2) { pallet->setFlagHighlight(true); }
          }
        }
      }
    }
  }

  scene->update();
}

void WidgetGalpao::unselectOthers() {
  auto items = scene->items();

  for (auto item : items) {
    if (auto pallet = dynamic_cast<PalletItem *>(item)) { pallet->unselect(); }
  }
}

void WidgetGalpao::on_pushButtonCriarPallet_clicked() {
  //
}

void WidgetGalpao::on_pushButtonRemoverPallet_clicked() {
  //
}

void WidgetGalpao::on_groupBoxEdicao_toggled(bool checked) {
  auto items = scene->items();

  for (auto item : items) {
    if (auto pallet = dynamic_cast<PalletItem *>(item)) { pallet->setFlag(QGraphicsItem::ItemIsMovable, checked); }
  }

  if (checked) { unselectOthers(); }

  ui->pushButtonCriarPallet->setDisabled(true);
  ui->pushButtonRemoverPallet->setDisabled(true);
}

void WidgetGalpao::resizeEvent(QResizeEvent *event) {
  ui->graphicsGalpao->fitInView(0, 0, 624, 586, Qt::KeepAspectRatio);

  QWidget::resizeEvent(event);
}

// TODO: adicionar botao para criar pallet
// TODO: adicionar botao para remover pallet
// TODO: funcao de selecionar um caminhao e colorir todos os pallets correspondentes aos produtos agendados
// TODO: zoom
// TODO: guardar idEstoque em veiculo_has_produto
// TODO: listar os consumos que na venda esteja em 'estoque' e o restante dos estoques (livre)
// TODO: colocar nas permissoes de usuario uma coluna para 'galpao' para poder limitar quem ve essa aba
// TODO: quando marcar item entregue mudar bloco do consumo para fora dos pallets (usar um pallet invisivel ou apenas deixar vazio a coluna do bloco)

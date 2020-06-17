#include "galpao.h"
#include "ui_galpao.h"

#include "application.h"
#include "palletitem.h"

#include <QDebug>
#include <QDrag>
#include <QGraphicsRectItem>
#include <QSqlError>
#include <QSqlQuery>

Galpao::Galpao(QWidget *parent) : QWidget(parent), ui(new Ui::Galpao) {
  ui->setupUi(this);

  scene = new GraphicsScene(this);
  ui->graphicsView->setScene(scene);

  setupTables();

  ui->itemBoxVeiculo->setSearchDialog(SearchDialog::veiculo(this));
  ui->dateTimeEdit->setDate(qApp->serverDate());

  connect(ui->itemBoxVeiculo, &ItemBox::textChanged, this, &Galpao::on_itemBoxVeiculo_textChanged);
  connect(ui->dateTimeEdit, &QDateTimeEdit::dateChanged, this, &Galpao::on_dateTimeEdit_dateChanged);

  scene->addItem(new QGraphicsPixmapItem(QPixmap("://galpao.png")));

  ui->graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  ui->graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  //  ui->graphicsView->setFixedSize(842, 595);
  //  ui->graphicsView->setSceneRect(0, 0, 842, 595);
  //  ui->graphicsView->fitInView(0, 0, 0, 0, Qt::KeepAspectRatio);

  carregarPallets();
}

Galpao::~Galpao() { delete ui; }

void Galpao::setupTables() {
  modelTranspAgend.setTable("veiculo_has_produto");

  modelTranspAgend.setHeaderData("data", "Agendado");
  modelTranspAgend.setHeaderData("idEstoque", "Estoque");
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
}

void Galpao::carregarPallets() {
  QSqlQuery query;

  if (not query.exec("select g.*, group_concat(concat(v.idEstoque, ' - ', CAST(v.caixas AS DECIMAL(15,2)), 'cx - ', v.descricao) separator '\n') as descricao from galpao g left join "
                     "view_estoque_contabil v on g.bloco = v.bloco group by g.bloco order by CAST(g.bloco AS UNSIGNED) desc")) {
    return qApp->enqueueError("Erro buscando dados do galpão: " + query.lastError().text(), this);
  }

  while (query.next()) {
    const QStringList posicao = query.value("posicao").toString().split(",");
    const QStringList tamanho = query.value("tamanho").toString().split(",");

    auto *pallet = new PalletItem(QRect(0, 0, tamanho.at(0).toInt(), tamanho.at(1).toInt()));
    pallet->setPos(posicao.at(0).toInt(), posicao.at(1).toInt());
    pallet->setLabel(query.value("bloco").toString());
    pallet->setText(query.value("descricao").toString());
    //    pallet->setFlags(QGraphicsItem::ItemIsMovable);
    //    rect->setPen(QPen(QColor(Qt::red)));

    connect(pallet, &PalletItem::save, this, &Galpao::salvarPallets);

    scene->addItem(pallet);
  }
}

void Galpao::salvarPallets() {
  auto items = scene->items();

  for (auto item : items) {
    if (auto pallet = dynamic_cast<PalletItem *>(item)) {
      QString pos = QString::number(pallet->scenePos().x()) + "," + QString::number(pallet->scenePos().y());

      QSqlQuery query;

      if (not query.exec("UPDATE galpao SET posicao = '" + pos + "' WHERE bloco = '" + pallet->getLabel() + "'")) {
        return qApp->enqueueError("Erro salvando dados do galpão: " + query.lastError().text(), this);
      }
    }
  }
}

void Galpao::on_dateTimeEdit_dateChanged(const QDate &date) {
  modelTranspAgend.setFilter("idVeiculo = " + ui->itemBoxVeiculo->getId().toString() + " AND status != 'FINALIZADO' AND DATE(data) = '" + date.toString("yyyy-MM-dd") + "'");

  if (not modelTranspAgend.select()) { qApp->enqueueError("Erro: " + modelTranspAgend.lastError().text(), this); }
}

void Galpao::on_itemBoxVeiculo_textChanged(const QString &) {
  modelTranspAgend.setFilter("idVeiculo = " + ui->itemBoxVeiculo->getId().toString() + " AND status != 'FINALIZADO' AND DATE(data) = '" + ui->dateTimeEdit->date().toString("yyyy-MM-dd") + "'");

  if (not modelTranspAgend.select()) { qApp->enqueueError("Erro: " + modelTranspAgend.lastError().text(), this); }

  // TODO: pintar pallets aqui
}

// TODO: adicionar botao para criar pallet
// TODO: adicionar botao para remover pallet
// TODO: funcao de selecionar um caminhao e colorir todos os pallets correspondentes aos produtos agendados
// TODO: zoom
// TODO: guardar idEstoque em veiculo_has_produto
// TODO: listar os consumos que na venda esteja em 'estoque' e o restante dos estoques (livre)
// TODO: colocar nas permissoes de usuario uma coluna para 'galpao' para poder limitar quem ve essa aba

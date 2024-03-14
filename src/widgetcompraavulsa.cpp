#include "widgetcompraavulsa.h"
#include "ui_widgetcompraavulsa.h"

#include "comboboxdelegate.h"
#include "compraavulsa.h"
#include "dateformatdelegate.h"
#include "itemboxdelegate.h"
#include "lineeditdelegate.h"
#include "reaisdelegate.h"
#include "sortfilterproxymodel.h"

WidgetCompraAvulsa::WidgetCompraAvulsa(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetCompraAvulsa) { ui->setupUi(this); }

WidgetCompraAvulsa::~WidgetCompraAvulsa() { delete ui; }

void WidgetCompraAvulsa::setupTables() {
  modelCompra.setTable("compra_avulsa");

  modelCompra.setEditStrategy(QSqlTableModel::OnFieldChange);

  modelCompra.setFilter("");

  modelCompra.setHeaderData("status", "Status");
  modelCompra.setHeaderData("fornecedor", "Fornecedor");
  modelCompra.setHeaderData("descricao", "Produto");
  modelCompra.setHeaderData("obs", "Obs.");
  modelCompra.setHeaderData("codComercial", "Cód. Com.");
  modelCompra.setHeaderData("quant", "Quant.");
  modelCompra.setHeaderData("un", "Un.");
  modelCompra.setHeaderData("prcUnitario", "R$ Unit.");
  modelCompra.setHeaderData("preco", "R$");
  modelCompra.setHeaderData("dataRealCompra", "Data");

  ui->tableCompra->setModel(&modelCompra);

  ui->tableCompra->setItemDelegateForColumn("status", new ComboBoxDelegate(ComboBoxDelegate::Tipo::Pagar, this));
  ui->tableCompra->setItemDelegateForColumn("prcUnitario", new ReaisDelegate(this));
  ui->tableCompra->setItemDelegateForColumn("preco", new ReaisDelegate(this));

  ui->tableCompra->setPersistentColumns({"status"});

  ui->tableCompra->hideColumn("idPedido1");
  ui->tableCompra->hideColumn("idRelacionado");
  ui->tableCompra->hideColumn("idFollowup");
  ui->tableCompra->hideColumn("selecionado");
  ui->tableCompra->hideColumn("aliquotaSt");
  ui->tableCompra->hideColumn("st");
  ui->tableCompra->hideColumn("statusFinanceiro");
  ui->tableCompra->hideColumn("ordemCompra");
  ui->tableCompra->hideColumn("ordemRepresentacao");
  ui->tableCompra->hideColumn("codFornecedor");
  ui->tableCompra->hideColumn("idVenda");
  ui->tableCompra->hideColumn("idVendaProduto1");
  ui->tableCompra->hideColumn("idVendaProduto2");
  ui->tableCompra->hideColumn("idCompra");
  ui->tableCompra->hideColumn("idProduto");
  ui->tableCompra->hideColumn("colecao");
  ui->tableCompra->hideColumn("quantUpd");
  ui->tableCompra->hideColumn("un2");
  ui->tableCompra->hideColumn("caixas");
  ui->tableCompra->hideColumn("desconto");
  ui->tableCompra->hideColumn("kgcx");
  ui->tableCompra->hideColumn("formComercial");
  ui->tableCompra->hideColumn("codBarras");
  ui->tableCompra->hideColumn("dataPrevCompra");
  ui->tableCompra->hideColumn("dataPrevConf");
  ui->tableCompra->hideColumn("dataRealConf");
  ui->tableCompra->hideColumn("dataPrevFat");
  ui->tableCompra->hideColumn("dataRealFat");
  ui->tableCompra->hideColumn("dataPrevColeta");
  ui->tableCompra->hideColumn("dataRealColeta");
  ui->tableCompra->hideColumn("dataPrevReceb");
  ui->tableCompra->hideColumn("dataRealReceb");
  ui->tableCompra->hideColumn("dataPrevEnt");
  ui->tableCompra->hideColumn("dataRealEnt");
  ui->tableCompra->hideColumn("created");
  ui->tableCompra->hideColumn("lastUpdated");

  // -------------------------------------------------------------------------

  modelPagar.setTable("conta_a_pagar_has_pagamento");

  modelPagar.setEditStrategy(QSqlTableModel::OnFieldChange);

  modelPagar.setFilter("compraAvulsa = TRUE");

  modelPagar.setHeaderData("dataEmissao", "Data Emissão");
  modelPagar.setHeaderData("contraParte", "Contraparte");
  modelPagar.setHeaderData("idNFe", "NF-e cadastrada");
  modelPagar.setHeaderData("nfe", "NF-e");
  modelPagar.setHeaderData("valor", "R$");
  modelPagar.setHeaderData("tipo", "Tipo");
  modelPagar.setHeaderData("parcela", "Parcela");
  modelPagar.setHeaderData("dataPagamento", "Vencimento");
  modelPagar.setHeaderData("observacao", "Obs.");
  modelPagar.setHeaderData("status", "Status");
  modelPagar.setHeaderData("dataRealizado", "Data Realizado");
  modelPagar.setHeaderData("centroCusto", "Centro Custo");
  modelPagar.setHeaderData("grupo", "Grupo");
  modelPagar.setHeaderData("subGrupo", "SubGrupo");

  modelPagar.setSort("dataPagamento");

  modelPagar.proxyModel = new SortFilterProxyModel(&modelPagar, this);

  ui->tablePagar->setModel(&modelPagar);

  ui->tablePagar->setItemDelegateForColumn("valorReal", new ReaisDelegate(this));
  ui->tablePagar->setItemDelegateForColumn("dataEmissao", new DateFormatDelegate(this));
  ui->tablePagar->setItemDelegateForColumn("valor", new ReaisDelegate(this));
  ui->tablePagar->setItemDelegateForColumn("valorReal", new ReaisDelegate(this));
  ui->tablePagar->setItemDelegateForColumn("dataRealizado", new DateFormatDelegate(modelPagar.fieldIndex("dataPagamento"), modelPagar.fieldIndex("tipo"), false, this));

  ui->tablePagar->setItemDelegateForColumn("status", new ComboBoxDelegate(ComboBoxDelegate::Tipo::Pagar, this));

  ui->tablePagar->setItemDelegateForColumn("idNFe", new ItemBoxDelegate(ItemBoxDelegate::Tipo::NFe, false, this));
  ui->tablePagar->setItemDelegateForColumn("idConta", new ItemBoxDelegate(ItemBoxDelegate::Tipo::Conta, false, this));
  ui->tablePagar->setItemDelegateForColumn("centroCusto", new ItemBoxDelegate(ItemBoxDelegate::Tipo::Loja, false, this));
  ui->tablePagar->setItemDelegateForColumn("grupo", new LineEditDelegate(LineEditDelegate::Tipo::Grupo, this));

  ui->tablePagar->setPersistentColumns({"status", "idNFe"});

  ui->tablePagar->hideColumn("idPagamento");
  ui->tablePagar->hideColumn("idCompra");
  ui->tablePagar->hideColumn("idVenda");
  ui->tablePagar->hideColumn("idLoja");
  ui->tablePagar->hideColumn("idCnab");
  ui->tablePagar->hideColumn("valorReal");
  ui->tablePagar->hideColumn("tipoReal");
  ui->tablePagar->hideColumn("parcelaReal");
  ui->tablePagar->hideColumn("idConta");
  ui->tablePagar->hideColumn("tipoDet");
  ui->tablePagar->hideColumn("compraAvulsa");
  ui->tablePagar->hideColumn("desativado");
  ui->tablePagar->hideColumn("created");
  ui->tablePagar->hideColumn("lastUpdated");
}

void WidgetCompraAvulsa::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->checkBoxFiltroAgendado, &QCheckBox::toggled, this, &WidgetCompraAvulsa::montaFiltro, connectionType);
  connect(ui->checkBoxFiltroCancelado, &QCheckBox::toggled, this, &WidgetCompraAvulsa::montaFiltro, connectionType);
  connect(ui->checkBoxFiltroConferido, &QCheckBox::toggled, this, &WidgetCompraAvulsa::montaFiltro, connectionType);
  connect(ui->checkBoxFiltroPago, &QCheckBox::toggled, this, &WidgetCompraAvulsa::montaFiltro, connectionType);
  connect(ui->checkBoxFiltroPendente, &QCheckBox::toggled, this, &WidgetCompraAvulsa::montaFiltro, connectionType);
  connect(ui->groupBoxStatus, &QGroupBox::toggled, this, &WidgetCompraAvulsa::on_groupBoxStatus_toggled, connectionType);
  connect(ui->pushButtonCadastrar, &QPushButton::clicked, this, &WidgetCompraAvulsa::on_pushButtonCadastrar_clicked, connectionType);
}

void WidgetCompraAvulsa::on_groupBoxStatus_toggled(const bool enabled) {
  const auto container = ui->groupBoxStatus->findChildren<QCheckBox *>(QRegularExpression("checkBox"));

  for (const auto &child : container) {
    disconnect(child, &QCheckBox::toggled, this, &WidgetCompraAvulsa::montaFiltro);
    child->setEnabled(true);
    child->setChecked(enabled);
    connect(child, &QCheckBox::toggled, this, &WidgetCompraAvulsa::montaFiltro);
  }

  montaFiltro();
}

void WidgetCompraAvulsa::montaFiltro() {
  QStringList filtros;

  //-------------------------------------

  QStringList filtroCheck;

  const auto children = ui->groupBoxStatus->findChildren<QCheckBox *>(QRegularExpression("checkBox"));

  for (const auto &child : children) {
    if (child->isChecked()) { filtroCheck << "'" + child->text().toUpper() + "'"; }
  }

  if (not filtroCheck.isEmpty()) { filtros << "status IN (" + filtroCheck.join(", ") + ")"; }

  //-------------------------------------

  modelCompra.setFilter(filtros.join(" AND "));

  filtros << "compraAvulsa = TRUE";

  modelPagar.setFilter(filtros.join(" AND "));
}

void WidgetCompraAvulsa::updateTables() {
  if (not isSet) {
    setupTables();
    setConnections();
    montaFiltro();
    isSet = true;
  }

  modelCompra.select();
  modelPagar.select();
}

void WidgetCompraAvulsa::resetTables() {
  setupTables();
}

void WidgetCompraAvulsa::on_pushButtonCadastrar_clicked() {
  auto *compraAvulsa = new CompraAvulsa(this);
  compraAvulsa->setAttribute(Qt::WA_DeleteOnClose);

  compraAvulsa->show();
}

// TODO: colocar um autopreenchimento no prcUnitario/preco

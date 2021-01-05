#include "widgetestoque.h"
#include "ui_widgetestoque.h"

#include "application.h"
#include "doubledelegate.h"
#include "estoque.h"
#include "file.h"
#include "searchdialogproxymodel.h"
#include "sortfilterproxymodel.h"
#include "usersession.h"
#include "xlsxdocument.h"

#include <QDebug>
#include <QDesktopServices>
#include <QFileDialog>
#include <QSqlError>

WidgetEstoque::WidgetEstoque(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetEstoque) { ui->setupUi(this); }

WidgetEstoque::~WidgetEstoque() { delete ui; }

void WidgetEstoque::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetEstoque::escolheFiltro, connectionType);
  connect(ui->lineEditBusca2, &QLineEdit::textChanged, this, &WidgetEstoque::montaFiltro2, connectionType);
  connect(ui->pushButtonRelatorio, &QPushButton::clicked, this, &WidgetEstoque::on_pushButtonRelatorio_clicked, connectionType);
  connect(ui->radioButtonEstoque, &QRadioButton::toggled, this, &WidgetEstoque::on_radioButtonEstoque_toggled, connectionType);
  connect(ui->radioButtonEstoqueContabil, &QRadioButton::clicked, this, &WidgetEstoque::montaFiltroContabil, connectionType);
  connect(ui->radioButtonEstoqueZerado, &QRadioButton::clicked, this, &WidgetEstoque::montaFiltro, connectionType);
  connect(ui->radioButtonMaior, &QRadioButton::clicked, this, &WidgetEstoque::montaFiltro, connectionType);
  connect(ui->radioButtonStaccatoOFF, &QRadioButton::toggled, this, &WidgetEstoque::on_radioButtonStaccatoOFF_toggled, connectionType);
  connect(ui->radioButtonTodos, &QRadioButton::toggled, this, &WidgetEstoque::on_radioButtonTodos_toggled, connectionType);
  connect(ui->tableEstoque, &TableView::activated, this, &WidgetEstoque::on_table_activated, connectionType);
}

void WidgetEstoque::setupTables() {
  escolheFiltro();

  model.proxyModel = new SortFilterProxyModel(&model, this);

  ui->tableEstoque->setModel(&model);

  ui->tableEstoque->setItemDelegate(new DoubleDelegate(this));

  //----------------------------------------------

  modelProdutos.setTable("view_produto");

  modelProdutos.setFilter("estoque = TRUE AND descontinuado = FALSE AND desativado = FALSE");

  modelProdutos.proxyModel = new SearchDialogProxyModel(&modelProdutos, this);

  modelProdutos.select();

  modelProdutos.setHeaderData("fornecedor", "Fornecedor");
  modelProdutos.setHeaderData("statusEstoque", "Estoque");
  modelProdutos.setHeaderData("descricao", "Descrição");
  modelProdutos.setHeaderData("estoqueRestante", "Estoque Disp.");
  modelProdutos.setHeaderData("estoqueCaixa", "Estoque Cx.");
  modelProdutos.setHeaderData("lote", "Lote");
  modelProdutos.setHeaderData("un", "Un.");
  modelProdutos.setHeaderData("un2", "Un.2");
  modelProdutos.setHeaderData("colecao", "Coleção");
  modelProdutos.setHeaderData("tipo", "Tipo");
  modelProdutos.setHeaderData("minimo", "Mínimo");
  modelProdutos.setHeaderData("multiplo", "Múltiplo");
  modelProdutos.setHeaderData("m2cx", "M/Cx.");
  modelProdutos.setHeaderData("pccx", "Pç./Cx.");
  modelProdutos.setHeaderData("kgcx", "Kg./Cx.");
  modelProdutos.setHeaderData("formComercial", "Form. Com.");
  modelProdutos.setHeaderData("codComercial", "Cód. Com.");
  modelProdutos.setHeaderData("precoVenda", "R$");
  modelProdutos.setHeaderData("validade", "Validade");
  modelProdutos.setHeaderData("ui", "UI");

  ui->tableProdutos->setModel(&modelProdutos);

  ui->tableProdutos->hideColumn("idProduto");
  ui->tableProdutos->hideColumn("estoque");
  ui->tableProdutos->hideColumn("promocao");
  ui->tableProdutos->hideColumn("descontinuado");
  ui->tableProdutos->hideColumn("desativado");
  ui->tableProdutos->hideColumn("representacao");
}

void WidgetEstoque::setHeaderData() {
  model.setHeaderData("cnpjDest", "CNPJ");
  model.setHeaderData("status", "Status");
  model.setHeaderData("idEstoque", "Estoque");
  model.setHeaderData("fornecedor", "Fornecedor");
  model.setHeaderData("descricao", "Produto");
  model.setHeaderData("restante", "Quant. Rest.");

  if (ui->radioButtonEstoqueContabil->isChecked()) {
    model.setHeaderData("contabil", "Contábil");
    model.setHeaderData("caixasContabil", "Caixas");
  }

  model.setHeaderData("unEst", "Un.");
  model.setHeaderData("caixas", "Caixas");
  model.setHeaderData("lote", "Lote");
  model.setHeaderData("local", "Local");
  model.setHeaderData("bloco", "Bloco");
  model.setHeaderData("codComercial", "Cód. Com.");
  model.setHeaderData("nfe", "NFe");
  model.setHeaderData("dataPrevColeta", "Prev. Coleta");
  model.setHeaderData("dataRealColeta", "Coleta");
  model.setHeaderData("dataPrevReceb", "Prev. Receb.");
  model.setHeaderData("dataRealReceb", "Receb.");
}

void WidgetEstoque::updateTables() {
  if (not isSet) {
    ui->dateEditMes->setDate(qApp->serverDate());
    setConnections();

    const QString tipoUsuario = UserSession::tipoUsuario;

    if (tipoUsuario == "VENDEDOR" or tipoUsuario == "VENDEDOR ESPECIAL") {
      ui->groupBoxFiltros->hide();
      ui->groupBoxRelatorio->hide();
    }

    isSet = true;
  }

  if (not modelIsSet) {
    setupTables();
    modelIsSet = true;
  }

  const QString currentTab = ui->tabWidget->tabText(ui->tabWidget->currentIndex());

  if (currentTab == "Estoques") {
    model.setQuery(model.query().executedQuery());

    if (model.lastError().isValid()) { throw RuntimeException("Erro lendo tabela estoque: " + model.lastError().text()); }
  }

  if (currentTab == "Produtos") { modelProdutos.select(); }
}

void WidgetEstoque::resetTables() { modelIsSet = false; }

void WidgetEstoque::on_table_activated(const QModelIndex &index) {
  const QString idEstoque = model.data(index, "idEstoque").toString();

  auto *estoque = new Estoque(idEstoque, true, this);
  estoque->setAttribute(Qt::WA_DeleteOnClose);
}

void WidgetEstoque::escolheFiltro() { ui->radioButtonEstoqueContabil->isChecked() ? montaFiltroContabil() : montaFiltro(); }

void WidgetEstoque::montaFiltro() {
  const QString having = (ui->radioButtonMaior->isChecked()) ? "restante > 0" : "restante <= 0";

  model.setQuery(
      "SELECT n.cnpjDest AS cnpjDest, e.status, e.idEstoque, p.fornecedor, e.descricao, e.restante AS restante, e.un AS unEst, e.restante / p.quantCaixa AS caixas, e.lote, e.local, e.bloco, "
      "e.codComercial, ANY_VALUE(n.numeroNFe) AS nfe, ANY_VALUE(pf.dataPrevColeta) AS dataPrevColeta, ANY_VALUE(pf.dataRealColeta) AS dataRealColeta, ANY_VALUE(pf.dataPrevReceb) AS dataPrevReceb, "
      "ANY_VALUE(pf.dataRealReceb) AS dataRealReceb FROM estoque e LEFT JOIN estoque_has_compra ehc2 ON e.idEstoque = ehc2.idEstoque LEFT JOIN pedido_fornecedor_has_produto2 pf ON pf.idPedido2 = "
      "ehc2.idPedido2 LEFT JOIN nfe n ON e.idNFe = n.idNFe LEFT JOIN produto p ON e.idProduto = p.idProduto WHERE e.status NOT IN ('CANCELADO', 'QUEBRADO')" +
      getMatch() + " GROUP BY e.idEstoque HAVING " + having);

  if (model.lastError().isValid()) { throw RuntimeException("Erro lendo tabela estoque: " + model.lastError().text(), this); }

  setHeaderData();
}

void WidgetEstoque::montaFiltro2() {
  const QString text = ui->lineEditBusca2->text();

  modelProdutos.setFilter("estoque = TRUE AND descontinuado = FALSE AND desativado = FALSE AND (fornecedor LIKE '%" + text + "%' OR descricao LIKE '%" + text + "%' OR codComercial LIKE '%" + text +
                          "%')");
}

void WidgetEstoque::montaFiltroContabil() {
  // TODO: trocar o NOW() por uma data escolhida pelo usuario

  model.setQuery(
      "SELECT n.cnpjDest AS cnpjDest, e.status, e.idEstoque, p.fornecedor, e.descricao, e.quant + COALESCE(ehc.contabil, 0) + COALESCE(e.ajuste, 0) AS contabil,  (e.quant + COALESCE(ehc.contabil, 0) "
      "+ COALESCE(e.ajuste, 0)) / p.quantCaixa AS caixasContabil, e.restante AS restante, e.restante / p.quantCaixa AS caixas, e.un AS unEst, e.lote, e.local, e.bloco, e.codComercial, "
      "ANY_VALUE(n.numeroNFe) AS nfe, ANY_VALUE(pf.dataPrevColeta) AS dataPrevColeta, ANY_VALUE(pf.dataRealColeta) AS dataRealColeta, ANY_VALUE(pf.dataPrevReceb) AS dataPrevReceb, "
      "ANY_VALUE(pf.dataRealReceb) AS dataRealReceb FROM estoque e LEFT JOIN (SELECT ehc.idEstoque, SUM(ehc.quant) AS contabil FROM estoque_has_consumo ehc LEFT JOIN venda_has_produto2 vp ON "
      "ehc.idVendaProduto2 = vp.idVendaProduto2 WHERE (vp.dataRealEnt < NOW()) AND ehc.status != 'CANCELADO' GROUP BY ehc.idEstoque) ehc ON e.idEstoque = ehc.idEstoque LEFT JOIN estoque_has_compra "
      "ehc2 ON e.idEstoque = ehc2.idEstoque LEFT JOIN pedido_fornecedor_has_produto2 pf ON pf.idPedido2 = ehc2.idPedido2 LEFT JOIN nfe n ON e.idNFe = n.idNFe LEFT JOIN produto p ON e.idProduto = "
      "p.idProduto WHERE e.status NOT IN ('CANCELADO', 'QUEBRADO')" +
      getMatch() + " GROUP BY e.idEstoque HAVING contabil > 0");

  if (model.lastError().isValid()) { throw RuntimeException("Erro lendo tabela estoque: " + model.lastError().text(), this); }

  setHeaderData();
}

QString WidgetEstoque::getMatch() const {
  const QString text = qApp->sanitizeSQL(ui->lineEditBusca->text());

  if (text.isEmpty()) { return QString(); }

  QStringList strings = text.split(" ", Qt::SkipEmptyParts);

  for (auto &string : strings) { string.contains("-") ? string.prepend("\"").append("\"") : string.prepend("+").append("*"); }

  const QString text2 = strings.join(" ");

  const QString match =
      " AND (MATCH (e.descricao , e.codComercial) AGAINST ('" + text2 + "' IN BOOLEAN MODE) OR MATCH (p.fornecedor) AGAINST ('" + text2 + "' IN BOOLEAN MODE) OR e.idEstoque = '" + text + "')";

  return match;
}

void WidgetEstoque::on_pushButtonRelatorio_clicked() {
  // NOTE: verificar se deve considerar os estoques não recebidos (em coleta/recebimento)

  // 1. codigo produto
  // 2. descricao
  // 3. ncm
  // 4. unidade
  // 5. quantidade
  // 6. valor
  // 7. aliquota icms (se tiver)

  const QString data = ui->dateEditMes->date().addDays(1).toString("yyyy-MM-dd");

  SqlQueryModel modelContabil;
  modelContabil.setQuery(
      "SELECT e.idEstoque, GROUP_CONCAT(DISTINCT n.cnpjDest SEPARATOR ',') AS cnpjDest, e.status, p.fornecedor, e.descricao, e.quant + COALESCE(ehc.contabil, 0) + COALESCE(ajuste, 0) AS contabil, "
      "e.restante AS disponivel, e.un AS unEst, p.un AS unProd, (e.quant + COALESCE(ehc.contabil, 0) + COALESCE(ajuste, 0)) / p.quantCaixa AS caixasContabil, e.lote, e.local, e.bloco, "
      "e.codComercial, e.ncm, e.cstICMS, e.pICMS, e.cstIPI, e.cstPIS, e.cstCOFINS, GROUP_CONCAT(DISTINCT n.numeroNFe SEPARATOR ', ') AS nfe, e.valorUnid AS custoUnit, p.precoVenda AS precoVendaUnit, "
      "e.valorUnid * (e.quant + COALESCE(ehc.contabil, 0) + COALESCE(ajuste, 0)) AS custo, p.precoVenda * (e.quant + COALESCE(ehc.contabil, 0) + COALESCE(ajuste, 0)) AS precoVenda, ehc.idVenda FROM "
      "estoque e LEFT JOIN (SELECT ehc.idEstoque, SUM(ehc.quant) AS contabil, SUM(IF(vp.status != 'DEVOLVIDO ESTOQUE', vp.quant, 0)) AS consumoVenda, GROUP_CONCAT(DISTINCT vp.idVenda) AS idVenda "
      "FROM estoque_has_consumo ehc LEFT JOIN venda_has_produto2 vp ON ehc.idVendaProduto2 = vp.idVendaProduto2 WHERE (vp.dataRealEnt < '" +
      data +
      "') AND ehc.status != 'CANCELADO' GROUP BY ehc.idEstoque) ehc ON e.idEstoque = ehc.idEstoque LEFT JOIN estoque_has_compra ehc2 ON e.idEstoque = ehc2.idEstoque LEFT JOIN "
      "pedido_fornecedor_has_produto2 pf2 ON pf2.idPedido2 = ehc2.idPedido2 LEFT JOIN nfe n ON e.idNFe = n.idNFe LEFT JOIN produto p ON e.idProduto = p.idProduto WHERE e.status = 'ESTOQUE' AND "
      "e.created < '" +
      data + "' GROUP BY e.idEstoque HAVING contabil > 0 ORDER BY codComercial, lote");

  if (modelContabil.lastError().isValid()) { throw RuntimeException("Erro lendo tabela: " + modelContabil.lastError().text()); }

  const QString dir = QFileDialog::getExistingDirectory(this, "Pasta para salvar relatório");

  if (dir.isEmpty()) { return; }

  const QString arquivoModelo = QDir::currentPath() + "/modelos/relatorio_contabil.xlsx";

  File modelo(arquivoModelo);

  if (not modelo.exists()) { throw RuntimeException("Não encontrou o modelo do Excel!"); }

  const QString fileName = dir + "/relatorio_contabil_" + data + ".xlsx";

  File file(fileName);

  if (not file.open(QFile::WriteOnly)) { throw RuntimeException("Não foi possível abrir o arquivo '" + fileName + "' para escrita: " + file.errorString()); }

  file.close();

  gerarExcel(arquivoModelo, fileName, modelContabil);

  QDesktopServices::openUrl(QUrl::fromLocalFile(fileName));
  qApp->enqueueInformation("Arquivo salvo como " + fileName, this);
}

void WidgetEstoque::gerarExcel(const QString &arquivoModelo, const QString &fileName, const SqlQueryModel &modelContabil) {
  QXlsx::Document xlsx(arquivoModelo, this);

  xlsx.selectSheet("Sheet1");

  char column = 'A';

  for (int col = 0; col < modelContabil.columnCount(); ++col, ++column) { xlsx.write(column + QString::number(1), modelContabil.headerData(col, Qt::Horizontal).toString()); }

  column = 'A';

  for (int row = 0; row < modelContabil.rowCount(); ++row) {
    for (int col = 0; col < modelContabil.columnCount(); ++col, ++column) { xlsx.write(column + QString::number(row + 2), modelContabil.data(row, col)); }

    column = 'A';
  }

  if (not xlsx.saveAs(fileName)) { throw RuntimeException("Ocorreu algum erro ao salvar o arquivo!"); }
}

void WidgetEstoque::on_radioButtonTodos_toggled(bool checked) {
  if (not checked) { return; }

  modelProdutos.setFilter("estoque = TRUE AND descontinuado = FALSE AND desativado = FALSE");

  modelProdutos.select();
}

void WidgetEstoque::on_radioButtonStaccatoOFF_toggled(bool checked) {
  if (not checked) { return; }

  modelProdutos.setFilter("estoque = TRUE AND promocao = 2 AND descontinuado = FALSE AND desativado = FALSE");

  modelProdutos.select();
}

void WidgetEstoque::on_radioButtonEstoque_toggled(bool checked) {
  if (not checked) { return; }

  modelProdutos.setFilter("estoque = TRUE AND promocao = 0 AND descontinuado = FALSE AND desativado = FALSE");

  modelProdutos.select();
}

// NOTE: gerenciar lugares de estoque (cadastro/permissoes)
// TODO: 3tem produto com unidade barra que na verdade significa ML

// TODO: 5colocar um filtro para mostrar os cancelados/quebrados?
// TODO: 2poder trocar bloco do estoque
// TODO: -1verificar se o custo do pedido_fornecedor bate com os valores do estoque/consumo
// TODO: terminar de arrumar relatorio estoque
// TODO: [Conrado] colocar filtro/tela para buscar por pedido e mostrar os estoques em que foi consumido
// TODO: fix fulltext indexes (put match against inside subquery)
// TODO: criar um segundo relatorio para os gerentes sem o custo

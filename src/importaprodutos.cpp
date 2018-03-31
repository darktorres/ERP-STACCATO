#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>

#include "dateformatdelegate.h"
#include "doubledelegate.h"
#include "importaprodutos.h"
#include "importaprodutosproxymodel.h"
#include "porcentagemdelegate.h"
#include "ui_importaprodutos.h"
#include "validadedialog.h"

ImportaProdutos::ImportaProdutos(QWidget *parent) : Dialog(parent), ui(new Ui::ImportaProdutos) {
  ui->setupUi(this);

  connect(ui->checkBoxRepresentacao, &QCheckBox::toggled, this, &ImportaProdutos::on_checkBoxRepresentacao_toggled);
  connect(ui->pushButtonSalvar, &QPushButton::clicked, this, &ImportaProdutos::on_pushButtonSalvar_clicked);
  connect(ui->tableErro, &TableView::entered, this, &ImportaProdutos::on_tableErro_entered);
  connect(ui->tableProdutos, &TableView::entered, this, &ImportaProdutos::on_tableProdutos_entered);
  connect(ui->tabWidget, &QTabWidget::currentChanged, this, &ImportaProdutos::on_tabWidget_currentChanged);

  setWindowFlags(Qt::Window);

  setVariantMap();
  setProgressDialog();
  setupTables();
}

ImportaProdutos::~ImportaProdutos() { delete ui; }

bool ImportaProdutos::expiraPrecosAntigos() {
  QSqlQuery query;
  query.prepare("UPDATE produto_has_preco SET expirado = TRUE WHERE idProduto = :idProduto");
  query.bindValue(":idProduto", modelProduto.data(row, "idProduto"));

  if (not query.exec()) {
    emit errorSignal("Erro expirando preços antigos: " + query.lastError().text());
    return false;
  }

  return true;
}

void ImportaProdutos::importarProduto() {
  if (not readFile()) { return; }
  if (not readValidade()) { return; }

  tipo = Tipo::Produto;
  importarTabela();
}

void ImportaProdutos::importarEstoque() {
  emit errorSignal("Temporariamente desativado!");
  return;

  // NOTE: ao inves de cadastrar uma tabela de estoque, quando o estoque for gerado (importacao de xml) criar uma linha
  // correspondente na tabela produto com flag estoque, esse produto vai ser listado junto dos outros porem com cor
  // diferente

  // estoques gerados por tabela nao terao dados de impostos enquanto os de xml sim

  // obs1: o orcamento nao gera pré-consumo mas ao fechar pedido o estoque pode não estar mais disponivel
  // obs2: quando fechar pedido gerar pré-consumo
  // obs3: quando fechar pedido mudar status de 'pendente' para 'estoque' para nao aparecer na tela de compras
  // obs4: colocar na tabela produto um campo para indicar qual o estoque relacionado

  //  if (not readFile()) { return; }

  //  validade = 730;
  //  tipo = Tipo::Estoque;

  //  importarTabela();
}

void ImportaProdutos::importarPromocao() {
  if (not readFile()) { return; }
  if (not readValidade()) { return; }

  tipo = Tipo::Promocao;
  importarTabela();
}

bool ImportaProdutos::verificaSeRepresentacao() {
  QSqlQuery queryFornecedor;
  queryFornecedor.prepare("SELECT representacao FROM fornecedor WHERE razaoSocial = :razaoSocial");
  queryFornecedor.bindValue(":razaoSocial", fornecedor);

  if (not queryFornecedor.exec()) {
    emit errorSignal("Erro lendo tabela fornecedor: " + queryFornecedor.lastError().text());
    return false;
  }

  if (queryFornecedor.first()) ui->checkBoxRepresentacao->setChecked(queryFornecedor.value("representacao").toBool());

  return true;
}

bool ImportaProdutos::atualizaProduto() {
  row = hash.value(variantMap.value("fornecedor").toString() + variantMap.value("codComercial").toString() + variantMap.value("ui").toString() + QString::number(static_cast<int>(tipo)));

  if (hashAtualizado.value(row) == true) {
    variantMap.insert("colecao", "REPETIDO");
    return insereEmErro();
  }

  hashAtualizado[row] = true;

  if (not atualizaCamposProduto()) { return false; }
  if (not guardaNovoPrecoValidade()) { return false; }
  if (not marcaProdutoNaoDescontinuado()) { return false; }

  return true;
}

bool ImportaProdutos::importar() {
  db = QSqlDatabase::contains("Excel Connection") ? QSqlDatabase::database("Excel Connection") : QSqlDatabase::addDatabase("QODBC", "Excel Connection");

  db.setDatabaseName("DRIVER={Microsoft Excel Driver (*.xls, *.xlsx, *.xlsm, *.xlsb)};HDR=Yes;MaxScanRows=0;DBQ=" + file);

  if (not db.open()) {
    emit errorSignal("Ocorreu um erro ao abrir o arquivo, verifique se o mesmo não está aberto: " + db.lastError().text());
    return false;
  }

  const QSqlRecord record = db.record("BASE$");

  if (not verificaTabela(record)) { return false; }
  if (not cadastraFornecedores()) { return false; }
  mostraApenasEstesFornecedores();
  if (not verificaSeRepresentacao()) { return false; }
  if (not marcaTodosProdutosDescontinuados()) { return false; }

  modelProduto.setFilter("idFornecedor IN (" + idsFornecedor.join(",") + ") AND estoque = FALSE AND promocao = " + QString::number(static_cast<int>(tipo)));

  if (not modelProduto.select()) {
    emit errorSignal("Erro lendo tabela produto: " + modelProduto.lastError().text());
    return false;
  }

  const QString red = QString::number(static_cast<int>(FieldColors::Red));

  modelErro.setFilter("idFornecedor IN (" + idsFornecedor.join(",") + ") AND estoque = FALSE AND promocao = " + QString::number(static_cast<int>(tipo)) + " AND (m2cxUpd = " + red +
                      " OR pccxUpd = " + red + " OR codComercialUpd = " + red + " OR custoUpd = " + red + " OR precoVendaUpd = " + red + ")");

  if (not modelErro.select()) emit errorSignal("Erro lendo tabela erro: " + modelErro.lastError().text());

  itensExpired = modelProduto.rowCount();

  for (int row = 0, rowCount = modelProduto.rowCount(); row < rowCount; ++row) {
    hash[modelProduto.data(row, "fornecedor").toString() + modelProduto.data(row, "codComercial").toString() + modelProduto.data(row, "ui").toString() +
         modelProduto.data(row, "promocao").toString()] = row;
  }

  contaProdutos();

  int current = 0;

  QSqlQuery query("SELECT * FROM [BASE$]", db);

  bool canceled = false;

  while (query.next()) {
    if (progressDialog->wasCanceled()) {
      canceled = true;
      break;
    }

    if (query.value(record.indexOf("fornecedor")).toString().isEmpty()) continue;

    variantMap.insert("fornecedor", query.value(record.indexOf("fornecedor")));
    progressDialog->setValue(current++);

    leituraProduto(query, record);
    consistenciaDados();

    if (camposForaDoPadrao()) {
      insereEmErro();
      continue;
    }

    if (not(verificaSeProdutoJaCadastrado() ? atualizaProduto() : cadastraProduto())) { return false; }
  }

  progressDialog->cancel();

  if (canceled) { return false; }

  show();
  ui->tableProdutos->resizeColumnsToContents();

  db.close();

  const QString resultado = "Produtos importados: " + QString::number(itensImported) + "\nProdutos atualizados: " + QString::number(itensUpdated) +
                            "\nNão modificados: " + QString::number(itensNotChanged) + "\nDescontinuados: " + QString::number(itensExpired) + "\nCom erro: " + QString::number(itensError);

  QMessageBox::information(this, "Aviso!", resultado);

  return true;
}

void ImportaProdutos::importarTabela() {
  emit transactionStarted();

  if (not QSqlQuery("SET SESSION TRANSACTION ISOLATION LEVEL SERIALIZABLE").exec()) { return; }
  if (not QSqlQuery("START TRANSACTION").exec()) { return; }

  if (not importar()) {
    // REFAC: closeEvent does a rollback
    QSqlQuery("ROLLBACK").exec();
    emit transactionEnded();
    close();
  }
}

void ImportaProdutos::setProgressDialog() {
  progressDialog = new QProgressDialog(this);
  progressDialog->reset();
  progressDialog->setCancelButton(nullptr);
  progressDialog->setLabelText("Importando...");
  progressDialog->setWindowTitle("ERP Staccato");
  progressDialog->setWindowModality(Qt::WindowModal);
  progressDialog->setMinimum(0);
  progressDialog->setMaximum(0);
  progressDialog->setCancelButtonText("Cancelar");
}

bool ImportaProdutos::readFile() {
  file = QFileDialog::getOpenFileName(this, "Importar tabela genérica", QDir::currentPath(), tr("Excel (*.xlsx)"));

  if (file.isEmpty()) { return false; }

  setWindowTitle(file);

  return true;
}

bool ImportaProdutos::readValidade() {
  auto *validadeDlg = new ValidadeDialog();

  if (not validadeDlg->exec()) { return false; }

  validade = validadeDlg->getValidade();

  return true;
}

void ImportaProdutos::setupTables() {
  modelProduto.setTable("produto");
  modelProduto.setEditStrategy(QSqlTableModel::OnManualSubmit);

  modelProduto.setHeaderData("fornecedor", "Fornecedor");
  modelProduto.setHeaderData("descricao", "Descrição");
  modelProduto.setHeaderData("un", "Un.");
  modelProduto.setHeaderData("un2", "Un.2");
  modelProduto.setHeaderData("colecao", "Coleção");
  modelProduto.setHeaderData("tipo", "Tipo");
  modelProduto.setHeaderData("minimo", "Mínimo");
  modelProduto.setHeaderData("multiplo", "Múltiplo");
  modelProduto.setHeaderData("m2cx", "M./Cx.");
  modelProduto.setHeaderData("pccx", "Pç./Cx.");
  modelProduto.setHeaderData("kgcx", "Kg./Cx.");
  modelProduto.setHeaderData("formComercial", "Form. Com.");
  modelProduto.setHeaderData("codComercial", "Cód. Com.");
  modelProduto.setHeaderData("codBarras", "Cód. Barras");
  modelProduto.setHeaderData("ncm", "NCM");
  modelProduto.setHeaderData("ncmEx", "NCM EX");
  modelProduto.setHeaderData("icms", "ICMS");
  modelProduto.setHeaderData("cst", "CST");
  modelProduto.setHeaderData("qtdPallet", "Qt. Pallet");
  modelProduto.setHeaderData("custo", "Custo");
  modelProduto.setHeaderData("ipi", "IPI");
  modelProduto.setHeaderData("st", "ST");
  modelProduto.setHeaderData("precoVenda", "Preço Venda");
  modelProduto.setHeaderData("comissao", "Comissão");
  modelProduto.setHeaderData("observacoes", "Obs.");
  modelProduto.setHeaderData("origem", "Origem");
  modelProduto.setHeaderData("ui", "UI");
  modelProduto.setHeaderData("validade", "Validade");
  modelProduto.setHeaderData("markup", "Markup");

  ui->tableProdutos->setModel(new ImportaProdutosProxyModel(&modelProduto, this));

  for (int column = 0; column < modelProduto.columnCount(); ++column) {
    if (modelProduto.record().fieldName(column).endsWith("Upd")) ui->tableProdutos->setColumnHidden(column, true);
  }

  ui->tableProdutos->hideColumn("idProduto");
  ui->tableProdutos->hideColumn("idEstoque");
  ui->tableProdutos->hideColumn("estoqueRestante");
  ui->tableProdutos->hideColumn("idFornecedor");
  ui->tableProdutos->hideColumn("desativado");
  ui->tableProdutos->hideColumn("descontinuado");
  ui->tableProdutos->hideColumn("estoque");
  ui->tableProdutos->hideColumn("cfop");
  ui->tableProdutos->hideColumn("atualizarTabelaPreco");
  ui->tableProdutos->hideColumn("temLote");
  ui->tableProdutos->hideColumn("tipo");
  ui->tableProdutos->hideColumn("comissao");
  ui->tableProdutos->hideColumn("observacoes");
  ui->tableProdutos->hideColumn("origem");
  ui->tableProdutos->hideColumn("representacao");
  ui->tableProdutos->hideColumn("icms");
  ui->tableProdutos->hideColumn("cst");
  ui->tableProdutos->hideColumn("ipi");
  ui->tableProdutos->hideColumn("st");
  ui->tableProdutos->hideColumn("estoque");
  ui->tableProdutos->hideColumn("promocao");
  ui->tableProdutos->hideColumn("idProdutoRelacionado");

  ui->tableProdutos->setItemDelegateForColumn("validade", new DateFormatDelegate(this));

  auto *doubledelegate = new DoubleDelegate(this, 4);
  ui->tableProdutos->setItemDelegateForColumn("m2cx", doubledelegate);
  ui->tableProdutos->setItemDelegateForColumn("kgcx", doubledelegate);
  ui->tableProdutos->setItemDelegateForColumn("qtdPallet", doubledelegate);
  ui->tableProdutos->setItemDelegateForColumn("custo", doubledelegate);
  ui->tableProdutos->setItemDelegateForColumn("precoVenda", doubledelegate);

  auto *porcDelegate = new PorcentagemDelegate(this);
  ui->tableProdutos->setItemDelegateForColumn("icms", porcDelegate);
  ui->tableProdutos->setItemDelegateForColumn("ipi", porcDelegate);
  ui->tableProdutos->setItemDelegateForColumn("markup", porcDelegate);
  ui->tableProdutos->setItemDelegateForColumn("st", porcDelegate);

  //-------------------------------------------------------------//

  modelErro.setTable("produto");
  modelErro.setEditStrategy(QSqlTableModel::OnManualSubmit);

  modelErro.setHeaderData("fornecedor", "Fornecedor");
  modelErro.setHeaderData("descricao", "Descrição");
  modelErro.setHeaderData("un", "Un.");
  modelErro.setHeaderData("un2", "Un.2");
  modelErro.setHeaderData("colecao", "Coleção");
  modelErro.setHeaderData("tipo", "Tipo");
  modelErro.setHeaderData("m2cx", "M./Cx.");
  modelErro.setHeaderData("pccx", "Pç./Cx.");
  modelErro.setHeaderData("kgcx", "Kg./Cx.");
  modelErro.setHeaderData("minimo", "Mínimo");
  modelErro.setHeaderData("multiplo", "Múltiplo");
  modelErro.setHeaderData("formComercial", "Form. Com.");
  modelErro.setHeaderData("codComercial", "Cód. Com.");
  modelErro.setHeaderData("codBarras", "Cód. Barras");
  modelErro.setHeaderData("ncm", "NCM");
  modelErro.setHeaderData("ncmEx", "NCM EX");
  modelErro.setHeaderData("icms", "ICMS");
  modelErro.setHeaderData("cst", "CST");
  modelErro.setHeaderData("qtdPallet", "Qt. Pallet");
  modelErro.setHeaderData("custo", "Custo");
  modelErro.setHeaderData("ipi", "IPI");
  modelErro.setHeaderData("st", "ST");
  modelErro.setHeaderData("precoVenda", "Preço Venda");
  modelErro.setHeaderData("comissao", "Comissão");
  modelErro.setHeaderData("observacoes", "Obs.");
  modelErro.setHeaderData("origem", "Origem");
  modelErro.setHeaderData("ui", "UI");
  modelErro.setHeaderData("validade", "Validade");
  modelErro.setHeaderData("markup", "Markup");

  ui->tableErro->setModel(new ImportaProdutosProxyModel(&modelErro, this));

  for (int column = 0; column < modelErro.columnCount(); ++column) {
    if (modelErro.record().fieldName(column).endsWith("Upd")) ui->tableErro->setColumnHidden(column, true);
  }

  ui->tableErro->hideColumn("idProduto");
  ui->tableErro->hideColumn("idEstoque");
  ui->tableErro->hideColumn("estoqueRestante");
  ui->tableErro->hideColumn("idFornecedor");
  ui->tableErro->hideColumn("desativado");
  ui->tableErro->hideColumn("descontinuado");
  ui->tableErro->hideColumn("estoque");
  ui->tableErro->hideColumn("cfop");
  ui->tableErro->hideColumn("atualizarTabelaPreco");
  ui->tableErro->hideColumn("temLote");
  ui->tableErro->hideColumn("tipo");
  ui->tableErro->hideColumn("comissao");
  ui->tableErro->hideColumn("observacoes");
  ui->tableErro->hideColumn("origem");
  ui->tableErro->hideColumn("representacao");
  ui->tableErro->hideColumn("icms");
  ui->tableErro->hideColumn("cst");
  ui->tableErro->hideColumn("ipi");
  ui->tableErro->hideColumn("estoque");
  ui->tableErro->hideColumn("promocao");
  ui->tableErro->hideColumn("idProdutoRelacionado");

  ui->tableErro->setItemDelegateForColumn("validade", new DateFormatDelegate(this));

  ui->tableErro->setItemDelegateForColumn("m2cx", doubledelegate);
  ui->tableErro->setItemDelegateForColumn("kgcx", doubledelegate);
  ui->tableErro->setItemDelegateForColumn("qtdPallet", doubledelegate);
  ui->tableErro->setItemDelegateForColumn("custo", doubledelegate);
  ui->tableErro->setItemDelegateForColumn("precoVenda", doubledelegate);

  ui->tableErro->setItemDelegateForColumn("icms", porcDelegate);
  ui->tableErro->setItemDelegateForColumn("ipi", porcDelegate);
  ui->tableErro->setItemDelegateForColumn("markup", porcDelegate);
  ui->tableErro->setItemDelegateForColumn("st", porcDelegate);
}

void ImportaProdutos::setVariantMap() {
  variantMap.insert("fornecedor", QVariant(QVariant::String));
  variantMap.insert("descricao", QVariant(QVariant::String));
  variantMap.insert("un", QVariant(QVariant::String));
  variantMap.insert("colecao", QVariant(QVariant::String));
  variantMap.insert("m2cx", QVariant(QVariant::Double));
  variantMap.insert("pccx", QVariant(QVariant::Double));
  variantMap.insert("kgcx", QVariant(QVariant::Double));
  variantMap.insert("minimo", QVariant(QVariant::Double));
  variantMap.insert("formComercial", QVariant(QVariant::String));
  variantMap.insert("codComercial", QVariant(QVariant::String));
  variantMap.insert("codBarras", QVariant(QVariant::String));
  variantMap.insert("ncm", QVariant(QVariant::String));
  variantMap.insert("qtdPallet", QVariant(QVariant::Double));
  variantMap.insert("custo", QVariant(QVariant::Double));
  variantMap.insert("precoVenda", QVariant(QVariant::Double));
  variantMap.insert("ui", QVariant(QVariant::String));
  variantMap.insert("un2", QVariant(QVariant::String));
}

bool ImportaProdutos::cadastraFornecedores() {
  QSqlQuery query("SELECT DISTINCT(fornecedor) FROM [BASE$]", db);

  while (query.next()) {
    if (query.value("fornecedor").toString().isEmpty()) continue;

    fornecedor = query.value("fornecedor").toString();

    QSqlQuery queryFornecedor;
    queryFornecedor.prepare("UPDATE fornecedor SET validadeProdutos = :validade WHERE razaoSocial = :razaoSocial");
    queryFornecedor.bindValue(":validade", QDate::currentDate().addDays(validade));
    queryFornecedor.bindValue(":razaoSocial", fornecedor);

    if (not queryFornecedor.exec()) {
      emit errorSignal("Erro salvando validade: " + queryFornecedor.lastError().text());
      return false;
    }

    int idFornecedor;
    if (not buscarCadastrarFornecedor(fornecedor, idFornecedor)) { return false; }

    fornecedores.insert(fornecedor, idFornecedor);
  }

  if (fornecedores.isEmpty()) {
    emit errorSignal("Erro ao cadastrar fornecedores.");
    return false;
  }

  return true;
}

void ImportaProdutos::mostraApenasEstesFornecedores() {
  Q_FOREACH (const auto &fornecedor, fornecedores) { idsFornecedor.append(QString::number(fornecedor)); }
}

bool ImportaProdutos::marcaTodosProdutosDescontinuados() {
  QSqlQuery query;

  if (not query.exec("UPDATE produto SET descontinuado = TRUE WHERE idFornecedor IN (" + idsFornecedor.join(",") + ") AND estoque = FALSE AND promocao = " + QString::number(static_cast<int>(tipo)))) {
    emit errorSignal("Erro marcando produtos descontinuados: " + query.lastError().text());
    return false;
  }

  return true;
}

void ImportaProdutos::contaProdutos() {
  QSqlQuery queryProdSize("SELECT COUNT(*) FROM [BASE$]", db);
  if (not queryProdSize.first()) { return; }
  progressDialog->setMaximum(queryProdSize.value(0).toInt());
}

void ImportaProdutos::consistenciaDados() {
  Q_FOREACH (const auto &key, variantMap.keys()) {
    if (variantMap.value(key).toString().contains("*")) variantMap.insert(key, variantMap.value(key).toString().remove("*"));
  }

  if (variantMap.value("ui").isNull()) variantMap.insert("ui", 0);

  if (variantMap.value("ncm").toString().length() == 10) {
    variantMap.insert("ncmEx", variantMap.value("ncm").toString().right(2));
    variantMap.insert("ncm", variantMap.value("ncm").toString().left(8));
  }

  const QString un = variantMap.value("un").toString().toUpper();

  (un == "M2" or un == "M²") ? variantMap.insert("un", "M²") : variantMap.insert("un", un);

  variantMap.insert("ncm", variantMap.value("ncm").toString().remove(".").remove(",").remove("-").remove(" "));
  variantMap.insert("codBarras", variantMap.value("codBarras").toString().remove(".").remove(","));
  variantMap.insert("codComercial", variantMap.value("codComercial").toString().remove(".").remove(","));

  variantMap.insert("minimo", variantMap.value("minimo").toDouble());

  if (qFuzzyIsNull(variantMap.value("minimo").toDouble())) variantMap.insert("minimo", QVariant());
  if (qFuzzyIsNull(variantMap.value("multiplo").toDouble())) variantMap.insert("multiplo", QVariant());

  // NOTE: cast other fields to the correct type?
}

void ImportaProdutos::leituraProduto(const QSqlQuery &query, const QSqlRecord &record) {
  Q_FOREACH (const auto &key, variantMap.keys()) {
    if (key == "ncmEx") continue;
    if (key == "multiplo") continue;

    QVariant value = query.value(record.indexOf(key));

    if (value.type() == QVariant::Double) value = QString::number(value.toDouble(), 'f', 4).toDouble();

    variantMap.insert(key, value);
  }
}

bool ImportaProdutos::atualizaCamposProduto() {
  bool changed = false;

  Q_FOREACH (const auto &key, variantMap.keys()) {
    if (not variantMap.value(key).isNull() and modelProduto.data(row, key) != variantMap.value(key)) {
      if (not modelProduto.setData(row, key, variantMap.value(key))) { return false; }
      if (not modelProduto.setData(row, key + "Upd", static_cast<int>(FieldColors::Yellow))) { return false; }
    } else {
      if (not modelProduto.setData(row, key + "Upd", static_cast<int>(FieldColors::White))) { return false; }
    }
  }

  if (modelProduto.data(row, "ncmEx").toString().isEmpty()) {
    if (not modelProduto.setData(row, "ncmExUpd", static_cast<int>(FieldColors::White))) { return false; }
  }

  const QString validadeStr = QDate::currentDate().addDays(validade).toString("yyyy-MM-dd");

  if (modelProduto.data(row, "validade") != validadeStr) {
    if (not modelProduto.setData(row, "validade", validadeStr)) { return false; }
    if (not modelProduto.setData(row, "validadeUpd", static_cast<int>(FieldColors::Yellow))) { return false; }
    changed = true;
  } else {
    if (not modelProduto.setData(row, "validadeUpd", static_cast<int>(FieldColors::White))) { return false; }
  }

  double markup = 100 * ((variantMap.value("precoVenda").toDouble() / variantMap.value("custo").toDouble()) - 1.);
  const QString markupRound = QString::number(markup, 'f', 4);
  markup = markupRound.toDouble();

  if (not qFuzzyCompare(modelProduto.data(row, "markup").toDouble(), markup)) {
    if (not modelProduto.setData(row, "markup", markup)) { return false; }
    if (not modelProduto.setData(row, "markupUpd", static_cast<int>(FieldColors::Yellow))) { return false; }
    changed = true;
  } else {
    if (not modelProduto.setData(row, "markupUpd", static_cast<int>(FieldColors::White))) { return false; }
  }

  changed ? itensUpdated++ : itensNotChanged++;

  return true;
}

bool ImportaProdutos::marcaProdutoNaoDescontinuado() {
  if (not modelProduto.setData(row, "descontinuado", false)) { return false; }

  itensExpired--;

  return true;
}

bool ImportaProdutos::guardaNovoPrecoValidade() {
  // TODO: [Eduardo] esse trecho deixa a importacao muito lenta, verificar se posso remover a tabela de precos

  if (not expiraPrecosAntigos()) { return false; }

  QSqlQuery query;
  query.prepare("INSERT INTO produto_has_preco (idProduto, preco, validadeInicio, validadeFim) VALUES (:idProduto, :preco, :validadeInicio, :validadeFim)");
  query.bindValue(":idProduto", modelProduto.data(row, "idProduto"));
  query.bindValue(":preco", variantMap.value("precoVenda"));
  query.bindValue(":validadeInicio", QDate::currentDate());
  query.bindValue(":validadeFim", QDate::currentDate().addDays(validade));

  if (not query.exec()) {
    emit errorSignal("Erro inserindo dados em produto_has_preco: " + query.lastError().text());
    return false;
  }

  return true;
}

bool ImportaProdutos::verificaSeProdutoJaCadastrado() {
  return hash.contains(variantMap.value("fornecedor").toString() + variantMap.value("codComercial").toString() + variantMap.value("ui").toString() + QString::number(static_cast<int>(tipo)));
}

bool ImportaProdutos::pintarCamposForaDoPadrao(const int row) {
  // Fora do padrão
  if (variantMap.value("ncm").toString() == "0" or variantMap.value("ncm").toString().isEmpty() or
      (variantMap.value("ncm").toString().length() != 8 and variantMap.value("ncm").toString().length() != 10)) {
    if (not modelErro.setData(row, "ncmUpd", static_cast<int>(FieldColors::Gray))) { return false; }
  }

  if (variantMap.value("codBarras").toString() == "0" or variantMap.value("codBarras").toString().isEmpty()) {
    if (not modelErro.setData(row, "codBarrasUpd", static_cast<int>(FieldColors::Gray))) { return false; }
  }

  // Errados
  if (variantMap.value("colecao").toString() == "REPETIDO") {
    if (not modelErro.setData(row, "colecaoUpd", static_cast<int>(FieldColors::Red))) { return false; }
  }

  if ((variantMap.value("un").toString() == "M2" or variantMap.value("un").toString() == "M²" or variantMap.value("un").toString() == "ML") and variantMap.value("m2cx") <= 0.) {
    if (not modelErro.setData(row, "m2cxUpd", static_cast<int>(FieldColors::Red))) { return false; }
  }

  if (variantMap.value("un").toString() != "M2" and variantMap.value("un").toString() != "M²" and variantMap.value("un").toString() != "ML" and variantMap.value("pccx") < 1) {
    if (not modelErro.setData(row, "pccxUpd", static_cast<int>(FieldColors::Red))) { return false; }
  }

  if (variantMap.value("codComercial").toString() == "0" or variantMap.value("codComercial").toString().isEmpty()) {
    if (not modelErro.setData(row, "codComercialUpd", static_cast<int>(FieldColors::Red))) { return false; }
  }

  if (variantMap.value("custo") <= 0.) {
    if (not modelErro.setData(row, "custoUpd", static_cast<int>(FieldColors::Red))) { return false; }
  }

  if (variantMap.value("precoVenda") <= 0.) {
    if (not modelErro.setData(row, "precoVendaUpd", static_cast<int>(FieldColors::Red))) { return false; }
  }

  if (variantMap.value("precoVenda") < variantMap.value("custo")) {
    if (not modelErro.setData(row, "precoVendaUpd", static_cast<int>(FieldColors::Red))) { return false; }
  }

  return true;
}

bool ImportaProdutos::camposForaDoPadrao() {
  // Errados
  if (variantMap.value("colecao").toString() == "REPETIDO") return true;

  const QString un = variantMap.value("un").toString();
  if ((un == "M2" or un == "M²" or un == "ML") and variantMap.value("m2cx") <= 0.) return true;
  if (un != "M2" and un != "M²" and un != "ML" and variantMap.value("pccx") < 1) return true;

  const QString codComercial = variantMap.value("codComercial").toString();
  if (codComercial == "0" or codComercial.isEmpty()) return true;

  if (variantMap.value("custo") <= 0.) return true;
  if (variantMap.value("precoVenda") <= 0.) return true;
  if (variantMap.value("precoVenda") < variantMap.value("custo")) return true;

  return false;
}

bool ImportaProdutos::insereEmErro() {
  const int row = modelErro.rowCount();
  if (not modelErro.insertRow(row)) { return false; }

  Q_FOREACH (const auto &key, variantMap.keys()) {
    if (not modelErro.setData(row, key, variantMap.value(key))) { return false; }
    if (not modelErro.setData(row, key + "Upd", static_cast<int>(FieldColors::Green))) { return false; }
  }

  if (not modelErro.setData(row, "idFornecedor", fornecedores.value(variantMap.value("fornecedor").toString()))) { return false; }

  if (not modelErro.setData(row, "atualizarTabelaPreco", true)) { return false; }
  const QString data = QDate::currentDate().addDays(validade).toString("yyyy-MM-dd");
  if (not modelErro.setData(row, "validade", data)) { return false; }
  if (not modelErro.setData(row, "validadeUpd", static_cast<int>(FieldColors::Green))) { return false; }

  const double markup = 100 * ((variantMap.value("precoVenda").toDouble() / variantMap.value("custo").toDouble()) - 1.);
  if (not modelErro.setData(row, "markup", markup)) { return false; }
  if (not modelErro.setData(row, "markupUpd", static_cast<int>(FieldColors::Green))) { return false; }

  if (variantMap.value("ncm").toString().length() == 10) {
    if (not modelErro.setData(row, "ncmEx", variantMap.value("ncm").toString().right(2))) { return false; }
    if (not modelErro.setData(row, "ncmExUpd", static_cast<int>(FieldColors::Green))) { return false; }
    variantMap.insert("ncm", variantMap.value("ncm").toString().left(8));
  } else {
    if (not modelErro.setData(row, "ncmExUpd", static_cast<int>(FieldColors::Green))) { return false; }
  }

  if (not pintarCamposForaDoPadrao(row)) { return false; }

  hasError = true;
  itensError++;

  return true;
}

bool ImportaProdutos::insereEmOk() {
  const int row = modelProduto.rowCount();

  if (not modelProduto.insertRow(row)) { return false; }

  Q_FOREACH (const auto &key, variantMap.keys()) {
    if (not modelProduto.setData(row, key, variantMap.value(key))) { return false; }
    if (not modelProduto.setData(row, key + "Upd", static_cast<int>(FieldColors::Green))) { return false; }
  }

  if (not modelProduto.setData(row, "promocao", static_cast<int>(tipo))) { return false; }

  if (tipo == Tipo::Estoque or tipo == Tipo::Promocao) {
    QSqlQuery query;
    query.prepare("SELECT idProduto FROM produto WHERE idFornecedor = :idFornecedor AND codComercial = :codComercial AND promocao = FALSE AND estoque = FALSE");
    query.bindValue(":idFornecedor", fornecedores.value(variantMap.value("fornecedor").toString()));
    query.bindValue(":codComercial", modelProduto.data(row, "codComercial"));

    if (not query.exec()) {
      emit errorSignal("Erro buscando produto relacionado: " + query.lastError().text());
      return false;
    }

    if (query.first() and not modelProduto.setData(row, "idProdutoRelacionado", query.value("idProduto"))) { return false; }
  }

  const int idFornecedor = fornecedores.value(variantMap.value("fornecedor").toString());
  if (not modelProduto.setData(row, "idFornecedor", idFornecedor)) { return false; }

  if (not modelProduto.setData(row, "atualizarTabelaPreco", true)) { return false; }
  if (not modelProduto.setData(row, "validade", QDate::currentDate().addDays(validade).toString("yyyy-MM-dd"))) { return false; }
  if (not modelProduto.setData(row, "validadeUpd", static_cast<int>(FieldColors::Green))) { return false; }

  const double markup = 100 * ((variantMap.value("precoVenda").toDouble() / variantMap.value("custo").toDouble()) - 1.);
  if (not modelProduto.setData(row, "markup", markup)) { return false; }
  if (not modelProduto.setData(row, "markupUpd", static_cast<int>(FieldColors::Green))) { return false; }

  if (variantMap.value("ncm").toString().length() == 10) {
    if (not modelProduto.setData(row, "ncmEx", variantMap.value("ncm").toString().right(2))) { return false; }
    if (not modelProduto.setData(row, "ncmExUpd", static_cast<int>(FieldColors::Green))) { return false; }
    variantMap.insert("ncm", variantMap.value("ncm").toString().left(8));
  } else {
    if (not modelProduto.setData(row, "ncmExUpd", static_cast<int>(FieldColors::Green))) { return false; }
  }

  hash[modelProduto.data(row, "fornecedor").toString() + modelProduto.data(row, "codComercial").toString() + modelProduto.data(row, "ui").toString() + modelProduto.data(row, "promocao").toString()] =
      row;

  hashAtualizado[row] = true;

  itensImported++;

  return true;
}

bool ImportaProdutos::cadastraProduto() { return insereEmOk(); }

bool ImportaProdutos::buscarCadastrarFornecedor(const QString &fornecedor, int &id) {
  QSqlQuery queryFornecedor;
  queryFornecedor.prepare("SELECT idFornecedor FROM fornecedor WHERE razaoSocial = :razaoSocial");
  queryFornecedor.bindValue(":razaoSocial", fornecedor);

  if (not queryFornecedor.exec()) {
    emit errorSignal("Erro buscando fornecedor: " + queryFornecedor.lastError().text());
    return false;
  }

  if (not queryFornecedor.next()) {
    emit informationSignal("Fornecedor ainda não cadastrado! Cadastrando...");

    queryFornecedor.prepare("INSERT INTO fornecedor (razaoSocial) VALUES (:razaoSocial)");
    queryFornecedor.bindValue(":razaoSocial", fornecedor);

    if (not queryFornecedor.exec()) {
      emit errorSignal("Erro cadastrando fornecedor: " + queryFornecedor.lastError().text());
      return false;
    }

    if (queryFornecedor.lastInsertId().isNull()) {
      emit errorSignal("Erro lastInsertId");
      return false;
    }

    id = queryFornecedor.lastInsertId().toInt();
    return true;
  }

  id = queryFornecedor.value("idFornecedor").toInt();
  return true;
}

void ImportaProdutos::salvar() {
  if (not modelProduto.submitAll()) {
    emit errorSignal("Ocorreu um erro ao salvar os dados: " + modelProduto.lastError().text());
    // TODO: 5refactor this because after this runs the transaction is no more
    // REFAC: wrap in a lambda
    QSqlQuery("ROLLBACK").exec();
    emit transactionEnded();
    return;
  }

  QSqlQuery queryPrecos;
  queryPrecos.prepare("INSERT INTO produto_has_preco (idProduto, preco, validadeInicio, validadeFim) SELECT idProduto, precoVenda, :validadeInicio AS validadeInicio, :validadeFim AS validadeFim FROM "
                      "produto WHERE atualizarTabelaPreco = TRUE");
  queryPrecos.bindValue(":validadeInicio", QDate::currentDate().toString("yyyy-MM-dd"));
  queryPrecos.bindValue(":validadeFim", QDate::currentDate().addDays(validade).toString("yyyy-MM-dd"));

  if (not queryPrecos.exec()) {
    emit errorSignal("Erro inserindo dados em produto_has_preco: " + queryPrecos.lastError().text());
    QSqlQuery("ROLLBACK").exec();
    emit transactionEnded();
    return;
  }

  if (not queryPrecos.exec("UPDATE produto SET atualizarTabelaPreco = FALSE")) {
    emit errorSignal("Erro comunicando com banco de dados: " + queryPrecos.lastError().text());
    return;
  }

  if (not QSqlQuery("COMMIT").exec()) { return; }

  emit transactionEnded();

  emit informationSignal("Tabela salva com sucesso!");

  close();
}

void ImportaProdutos::on_pushButtonSalvar_clicked() {
  if (hasError) {
    QMessageBox msgBox(QMessageBox::Question, "Atenção!", "Produtos com erro não serão salvos. Deseja continuar?", QMessageBox::Yes | QMessageBox::No, this);
    msgBox.setButtonText(QMessageBox::Yes, "Continuar");
    msgBox.setButtonText(QMessageBox::No, "Voltar");

    if (msgBox.exec() == QMessageBox::No) { return; }
  }

  salvar();
}

bool ImportaProdutos::verificaTabela(const QSqlRecord &record) {
  Q_FOREACH (const auto &key, variantMap.keys()) {
    if (not record.contains(key)) {
      emit errorSignal(R"(Tabela não possui coluna ")" + key + R"(")");
      return false;
    }
  }

  return true;
}

void ImportaProdutos::on_tableProdutos_entered(const QModelIndex &) { ui->tableProdutos->resizeColumnsToContents(); }

void ImportaProdutos::on_tableErro_entered(const QModelIndex &) { ui->tableErro->resizeColumnsToContents(); }

void ImportaProdutos::on_tabWidget_currentChanged(const int index) {
  if (index == 0) ui->tableProdutos->resizeColumnsToContents();
  if (index == 1) ui->tableErro->resizeColumnsToContents();
}

void ImportaProdutos::closeEvent(QCloseEvent *event) {
  QSqlQuery("ROLLBACK").exec();
  emit transactionEnded();

  QDialog::closeEvent(event);
}

void ImportaProdutos::on_checkBoxRepresentacao_toggled(const bool checked) {
  for (int row = 0, rowCount = modelProduto.rowCount(); row < rowCount; ++row) {
    if (not modelProduto.setData(row, "representacao", checked)) emit errorSignal("Erro guardando 'Representacao' em Produto: " + modelProduto.lastError().text());
  }

  QSqlQuery query;
  if (not query.exec("UPDATE fornecedor SET representacao = " + QString(checked ? "TRUE" : "FALSE") + " WHERE idFornecedor IN (" + idsFornecedor.join(",") + ")")) {
    emit errorSignal("Erro guardando 'Representacao' em Fornecedor: " + query.lastError().text());
    return;
  }
}

// NOTE: 3colocar tabela relacao para precos diferenciados por loja (associar produto_has_preco <->
// produto_has_preco_has_loja ou guardar idLoja em produto_has_preco)
// NOTE: remover idProdutoRelacionado?
// TODO: 2unificar m2cx/pccx em uncx, podendo ter uma segunda coluna pccx/kgcx

// TODO: 4como esta tabela é colorida usar o delegate para pintar texto de branco quando for fundo escuro
// TODO: 4markup esta exibindo errado ou salvando errado
// TODO: 4nao mostrar promocao descontinuado
// TODO: 0se der erro durante a leitura o arquivo nao é fechado
// TODO: 0nao marcou produtos representacao com flag 1
// TODO: 0ler 'multiplo' na importacao (para produtos que usam minimo)
// TODO: mostrar os totais na tela e nao apenas na caixa de dialogo

// NOTE: para arrumar o problema da ambiguidade m2cx/pccx:
//       -colocar uma coluna 'unCaixa' na tabela de produtos para dizer qual a unidade da caixa;
//       -usar uma segunda coluna 'pccx' tambem
//       -no caso dos produtos por metro é usado ambas as colunas m2cx/pccx mas nos outros produtos apenas o 'pccx'
//       -para minimo/multiplo usar em relacao a 'unCaixa' de forma que se o minimo for uma caixa, entao o minimo é 1,
//        e o multiplo sendo 1/4 de caixa será 0,25. esses numeros serão portanto os valores de minimo e singlestep respectivamente
//        do spinbox.

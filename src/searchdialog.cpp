#include <QDebug>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>

#include "application.h"
#include "doubledelegate.h"
#include "porcentagemdelegate.h"
#include "reaisdelegate.h"
#include "searchdialog.h"
#include "searchdialogproxymodel.h"
#include "ui_searchdialog.h"
#include "usersession.h"

SearchDialog::SearchDialog(const QString &title, const QString &table, const QString &primaryKey, const QStringList &textKeys, const QString &fullTextIndex, const QString &filter, QWidget *parent)
    : QDialog(parent), primaryKey(primaryKey), fullTextIndex(fullTextIndex), textKeys(textKeys), filter(filter), model(100), ui(new Ui::SearchDialog) {
  ui->setupUi(this);

  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &SearchDialog::on_lineEditBusca_textChanged);
  connect(ui->pushButtonSelecionar, &QPushButton::clicked, this, &SearchDialog::on_pushButtonSelecionar_clicked);
  connect(ui->radioButtonProdAtivos, &QRadioButton::clicked, this, &SearchDialog::on_radioButtonProdAtivos_toggled);
  connect(ui->radioButtonProdDesc, &QRadioButton::clicked, this, &SearchDialog::on_radioButtonProdDesc_toggled);
  connect(ui->table, &TableView::doubleClicked, this, &SearchDialog::on_table_doubleClicked);

  setWindowTitle(title);
  setWindowModality(Qt::NonModal);
  setWindowFlags(Qt::Window);

  setupTables(table);

  if (fullTextIndex.isEmpty()) {
    ui->lineEditBusca->hide();
    ui->labelBusca->hide();
  }

  ui->radioButtonProdAtivos->hide();
  ui->radioButtonProdDesc->hide();
  ui->lineEditEstoque_2->hide();
  ui->lineEditEstoque->hide();
  ui->lineEditPromocao->hide();
  ui->lineEditBusca->setFocus();
}

SearchDialog::~SearchDialog() { delete ui; }

void SearchDialog::setupTables(const QString &table) {
  model.setTable(table);

  setFilter(filter);

  ui->table->setModel(new SearchDialogProxyModel(&model, this));

  ui->table->setItemDelegate(new DoubleDelegate(this));
}

void SearchDialog::on_lineEditBusca_textChanged(const QString &) {
  const QString text = ui->lineEditBusca->text().replace("-", " ").replace("(", "").replace(")", "");

  if (text.isEmpty()) {
    model.setFilter(filter);
    return;
  }

  QStringList strings = text.split(" ", QString::SkipEmptyParts);

  for (auto &string : strings) { string.prepend("+").append("*"); }

  QString searchFilter = "MATCH(" + fullTextIndex + ") AGAINST('" + strings.join(" ") + "' IN BOOLEAN MODE)";

  if (model.tableName() == "produto") {
    const QString descontinuado = " AND descontinuado = " + QString(ui->radioButtonProdAtivos->isChecked() ? "FALSE" : "TRUE");
    const QString representacao = showAllProdutos ? "" : (isRepresentacao ? " AND representacao = TRUE" : " AND representacao = FALSE");

    model.setFilter(searchFilter + descontinuado + " AND desativado = FALSE" + representacao + fornecedorRep);

    return;
  }

  if (not filter.isEmpty()) { searchFilter.append(" AND (" + filter + ")"); }

  model.setFilter(searchFilter);
}

void SearchDialog::sendUpdateMessage() {
  const auto selection = ui->table->selectionModel()->selection().indexes();

  if (selection.isEmpty()) { return; }

  emit itemSelected(model.data(selection.first().row(), primaryKey).toString());
}

bool SearchDialog::prepare_show() {
  model.setFilter(filter);

  if (not isSet) {
    if (not model.select()) { return false; }
    isSet = true;
  }

  ui->lineEditBusca->setFocus();
  ui->lineEditBusca->clear();

  ui->lineEditEstoque_2->setText("Staccato OFF");
  ui->lineEditEstoque->setText("Estoque");
  ui->lineEditPromocao->setText("Promoção");

  return true;
}

void SearchDialog::show() {
  if (not prepare_show()) { return; }

  QDialog::show();
}

void SearchDialog::showMaximized() {
  if (not prepare_show()) { return; }

  QDialog::showMaximized();
}

void SearchDialog::on_table_doubleClicked(const QModelIndex &) { on_pushButtonSelecionar_clicked(); }

void SearchDialog::setFilter(const QString &newFilter) {
  filter = newFilter;
  model.setFilter(filter);
}

void SearchDialog::hideColumns(const QStringList &columns) {
  for (const auto &column : columns) { ui->table->hideColumn(column); }
}

void SearchDialog::on_pushButtonSelecionar_clicked() {
  if (not permitirDescontinuados and ui->radioButtonProdDesc->isChecked()) {
    return qApp->enqueueError("Não pode selecionar produtos descontinuados!\nEntre em contato com o Dept. de Compras!", this);
  }

  if (model.tableName() == "produto") {
    const auto selection = ui->table->selectionModel()->selection().indexes();
    const bool isEstoque = model.data(selection.first().row(), "estoque").toBool();

    if (not silent and not selection.isEmpty() and isEstoque) { qApp->enqueueWarning("Verificar com o Dept. de Compras a disponibilidade do estoque antes de vender!", this); }
  }

  sendUpdateMessage();
  close();
}

QString SearchDialog::getText(const QVariant &id) {
  if (id == 0) { return QString(); }
  if (model.tableName().contains("endereco") and id == 1) { return "Não há/Retira"; }

  QString queryText;

  for (const auto &key : std::as_const(textKeys)) { queryText += queryText.isEmpty() ? key : ", " + key; }

  queryText = "SELECT " + queryText + " FROM " + model.tableName() + " WHERE " + primaryKey + " = '" + id.toString() + "'";

  QSqlQuery query;

  if (not query.exec(queryText) or not query.first()) {
    qApp->enqueueError("Erro na query getText: " + query.lastError().text(), this);
    return QString();
  }

  QString res;

  for (const auto &key : std::as_const(textKeys)) {
    if (query.value(key).isValid() and not query.value(key).toString().isEmpty()) { res += (res.isEmpty() ? "" : " - ") + query.value(key).toString(); }
  }

  return res;
}

void SearchDialog::setHeaderData(const QString &column, const QString &newHeader) { model.setHeaderData(column, newHeader); }

SearchDialog *SearchDialog::cliente(QWidget *parent) {
  SearchDialog *sdCliente = new SearchDialog("Buscar Cliente", "cliente", "idCliente", {"nome_razao"}, "nome_razao, nomeFantasia, cpf, cnpj", "desativado = FALSE", parent);

  sdCliente->hideColumns({"idCliente", "inscEstadual", "credito", "idUsuarioRel", "idCadastroRel", "idProfissionalRel", "incompleto", "desativado"});

  sdCliente->setHeaderData("pfpj", "Tipo");
  sdCliente->setHeaderData("nome_razao", "Cliente");
  sdCliente->setHeaderData("cpf", "CPF");
  sdCliente->setHeaderData("cnpj", "CNPJ");
  sdCliente->setHeaderData("dataNasc", "Data Nasc.");
  sdCliente->setHeaderData("contatoNome", "Nome - Contato");
  sdCliente->setHeaderData("contatoCPF", "CPF - Contato");
  sdCliente->setHeaderData("contatoApelido", "Apelido - Contato");
  sdCliente->setHeaderData("contatoRG", "RG - Contato");
  sdCliente->setHeaderData("nomeFantasia", "Fantasia/Apelido");
  sdCliente->setHeaderData("tel", "Tel.");
  sdCliente->setHeaderData("telCel", "Tel. Cel.");
  sdCliente->setHeaderData("telCom", "Tel. Com.");
  sdCliente->setHeaderData("idNextel", "id Nextel");
  sdCliente->setHeaderData("nextel", "Nextel");
  sdCliente->setHeaderData("email", "E-mail");

  return sdCliente;
}

SearchDialog *SearchDialog::loja(QWidget *parent) {
  SearchDialog *sdLoja = new SearchDialog("Buscar Loja", "loja", "idLoja", {"nomeFantasia"}, "descricao, nomeFantasia, razaoSocial", "desativado = FALSE", parent);

  sdLoja->ui->table->setItemDelegateForColumn("porcentagemFrete", new PorcentagemDelegate(parent));
  sdLoja->ui->table->setItemDelegateForColumn("valorMinimoFrete", new ReaisDelegate(parent));

  sdLoja->hideColumns({"idLoja", "codUF", "desativado"});

  sdLoja->setHeaderData("descricao", "Descrição");
  sdLoja->setHeaderData("nomeFantasia", "Nome Fantasia");
  sdLoja->setHeaderData("razaoSocial", "Razão Social");
  sdLoja->setHeaderData("tel", "Tel.");
  sdLoja->setHeaderData("inscEstadual", "Insc. Est.");
  sdLoja->setHeaderData("sigla", "Sigla");
  sdLoja->setHeaderData("cnpj", "CNPJ");
  sdLoja->setHeaderData("porcentagemFrete", "Frete");
  sdLoja->setHeaderData("valorMinimoFrete", "Mínimo Frete");

  return sdLoja;
}

SearchDialog *SearchDialog::produto(const bool permitirDescontinuados, const bool silent, const bool showAllProdutos, QWidget *parent) {
  // TODO: 3nao mostrar promocao vencida no descontinuado
  SearchDialog *sdProd = new SearchDialog("Buscar Produto", "produto", "idProduto", {"descricao"}, "fornecedor, descricao, colecao, codcomercial", "idProduto = 0", parent);

  sdProd->permitirDescontinuados = permitirDescontinuados;
  sdProd->silent = silent;
  sdProd->showAllProdutos = showAllProdutos;

  sdProd->hideColumns(
      {"idEstoque", "atualizarTabelaPreco", "cfop", "codBarras", "comissao", "cst",   "custo",       "desativado", "descontinuado", "estoque",       "promocao", "icms",   "idFornecedor",
       "idProduto", "idProdutoRelacionado", "ipi",  "markup",    "ncm",      "ncmEx", "observacoes", "origem",     "qtdPallet",     "representacao", "st",       "temLote"});

  for (int column = 0, columnCount = sdProd->model.columnCount(); column < columnCount; ++column) {
    if (sdProd->model.record().fieldName(column).endsWith("Upd")) { sdProd->ui->table->setColumnHidden(column, true); }
  }

  sdProd->setHeaderData("fornecedor", "Fornecedor");
  sdProd->setHeaderData("descricao", "Descrição");
  sdProd->setHeaderData("estoqueRestante", "Estoque Disp.");
  sdProd->setHeaderData("un", "Un.");
  sdProd->setHeaderData("un2", "Un.2");
  sdProd->setHeaderData("colecao", "Coleção");
  sdProd->setHeaderData("tipo", "Tipo");
  sdProd->setHeaderData("minimo", "Mínimo");
  sdProd->setHeaderData("multiplo", "Múltiplo");
  sdProd->setHeaderData("m2cx", "M/Cx.");
  sdProd->setHeaderData("pccx", "Pç./Cx.");
  sdProd->setHeaderData("kgcx", "Kg./Cx.");
  sdProd->setHeaderData("formComercial", "Form. Com.");
  sdProd->setHeaderData("codComercial", "Cód. Com.");
  sdProd->setHeaderData("precoVenda", "R$");
  sdProd->setHeaderData("validade", "Validade");
  sdProd->setHeaderData("ui", "UI");

  sdProd->ui->radioButtonProdAtivos->show();
  sdProd->ui->radioButtonProdDesc->show();
  sdProd->ui->lineEditEstoque->show();
  sdProd->ui->lineEditEstoque_2->show();
  sdProd->ui->lineEditPromocao->show();
  sdProd->ui->radioButtonProdAtivos->setChecked(true);

  return sdProd;
}

SearchDialog *SearchDialog::fornecedor(QWidget *parent) {
  SearchDialog *sdFornecedor =
      new SearchDialog("Buscar Fornecedor", "fornecedor", "idFornecedor", {"nomeFantasia", "razaoSocial"}, "razaoSocial, nomeFantasia, contatoCPF, cnpj", "desativado = FALSE", parent);

  sdFornecedor->hideColumns({"aliquotaSt", "comissao1", "comissao2", "comissaoLoja", "desativado", "email", "idFornecedor", "idNextel", "inscEstadual", "nextel", "representacao", "st", "tel",
                             "telCel", "telCom", "especialidade"});

  sdFornecedor->setHeaderData("razaoSocial", "Razão Social");
  sdFornecedor->setHeaderData("nomeFantasia", "Nome Fantasia");
  sdFornecedor->setHeaderData("contatoNome", "Nome do Contato");
  sdFornecedor->setHeaderData("cnpj", "CNPJ");
  sdFornecedor->setHeaderData("contatoCPF", "CPF do Contato");
  sdFornecedor->setHeaderData("contatoApelido", "Apelido do Contato");
  sdFornecedor->setHeaderData("contatoRG", "RG do Contato");
  sdFornecedor->setHeaderData("validadeProdutos", "Validade Produtos");

  return sdFornecedor;
}

SearchDialog *SearchDialog::transportadora(QWidget *parent) {
  SearchDialog *sdTransportadora = new SearchDialog("Buscar Transportadora", "transportadora", "idTransportadora", {"nomeFantasia"}, "razaoSocial, nomeFantasia", "desativado = FALSE", parent);

  sdTransportadora->hideColumns({"idTransportadora", "desativado"});

  sdTransportadora->setHeaderData("razaoSocial", "Razão Social");
  sdTransportadora->setHeaderData("nomeFantasia", "Nome Fantasia");
  sdTransportadora->setHeaderData("cnpj", "CNPJ");
  sdTransportadora->setHeaderData("inscEstadual", "Insc. Est.");
  sdTransportadora->setHeaderData("antt", "ANTT");
  sdTransportadora->setHeaderData("tel", "Tel.");

  return sdTransportadora;
}

SearchDialog *SearchDialog::veiculo(QWidget *parent) {
  SearchDialog *sdTransportadora = new SearchDialog("Buscar Veículo", "view_busca_veiculo", "idVeiculo", {"razaoSocial", "modelo", "placa"}, "modelo, placa", "desativado = FALSE", parent);

  sdTransportadora->hideColumns({"idVeiculo", "desativado"});

  sdTransportadora->setHeaderData("razaoSocial", "Transportadora");
  sdTransportadora->setHeaderData("modelo", "Modelo");
  sdTransportadora->setHeaderData("capacidade", "Carga");
  sdTransportadora->setHeaderData("placa", "Placa");

  return sdTransportadora;
}

SearchDialog *SearchDialog::usuario(QWidget *parent) {
  SearchDialog *sdUsuario = new SearchDialog("Buscar Usuário", "usuario", "idUsuario", {"nome"}, "nome, tipo", "usuario.desativado = FALSE", parent);

  sdUsuario->hideColumns({"idUsuario", "user", "passwd", "especialidade", "desativado"});

  sdUsuario->setHeaderData("idLoja", "Loja");
  sdUsuario->setHeaderData("tipo", "Função");
  sdUsuario->setHeaderData("nome", "Nome");
  sdUsuario->setHeaderData("sigla", "Sigla");
  sdUsuario->setHeaderData("email", "E-mail");

  sdUsuario->model.setRelation(sdUsuario->model.fieldIndex("idLoja"), QSqlRelation("loja", "idLoja", "descricao"));

  return sdUsuario;
}

SearchDialog *SearchDialog::vendedor(QWidget *parent) {
  const int idLoja = UserSession::idLoja();
  const bool specialSeller = (UserSession::tipoUsuario() == "VENDEDOR ESPECIAL");

  const QString filtro = "desativado = FALSE AND tipo IN ('VENDEDOR', 'VENDEDOR ESPECIAL')";
  const QString filtroLoja = (idLoja == 1 or specialSeller) ? "" : " AND idLoja = " + QString::number(idLoja);
  const QString filtroAdmin = (UserSession::tipoUsuario() == "ADMINISTRADOR") ? "" : " AND nome != 'REPOSIÇÂO'";

  SearchDialog *sdVendedor = new SearchDialog("Buscar Vendedor", "usuario", "idUsuario", {"nome"}, "nome, tipo", filtro + filtroLoja + filtroAdmin, parent);

  sdVendedor->model.setSort("nome", Qt::AscendingOrder);

  sdVendedor->hideColumns({"idUsuario", "idLoja", "user", "passwd", "especialidade", "desativado"});

  sdVendedor->setHeaderData("tipo", "Função");
  sdVendedor->setHeaderData("nome", "Nome");
  sdVendedor->setHeaderData("sigla", "Sigla");
  sdVendedor->setHeaderData("email", "E-mail");

  return sdVendedor;
}

SearchDialog *SearchDialog::conta(QWidget *parent) {
  SearchDialog *sdConta = new SearchDialog("Buscar Conta", "loja_has_conta", "idConta", {"banco", "agencia", "conta"}, {}, "", parent);

  sdConta->hideColumns({"idConta", "idLoja", "desativado"});

  sdConta->setHeaderData("banco", "Banco");
  sdConta->setHeaderData("agencia", "Agência");
  sdConta->setHeaderData("conta", "Conta");

  return sdConta;
}

SearchDialog *SearchDialog::enderecoCliente(QWidget *parent) {
  SearchDialog *sdEndereco = new SearchDialog("Buscar Endereço", "cliente_has_endereco", "idEndereco", {"logradouro", "numero", "bairro", "cidade", "uf"}, {}, "idEndereco = 1", parent);

  sdEndereco->hideColumns({"idEndereco", "idCliente", "codUF", "desativado"});

  sdEndereco->setHeaderData("descricao", "Descrição");
  sdEndereco->setHeaderData("cep", "CEP");
  sdEndereco->setHeaderData("logradouro", "End.");
  sdEndereco->setHeaderData("numero", "Número");
  sdEndereco->setHeaderData("complemento", "Comp.");
  sdEndereco->setHeaderData("bairro", "Bairro");
  sdEndereco->setHeaderData("cidade", "Cidade");
  sdEndereco->setHeaderData("uf", "UF");

  return sdEndereco;
}

SearchDialog *SearchDialog::profissional(const bool mostrarNaoHa, QWidget *parent) {
  const QString filtroNaoHa = mostrarNaoHa ? "" : " AND idProfissional NOT IN (1)";

  SearchDialog *sdProfissional = new SearchDialog("Buscar Profissional", "profissional", "idProfissional", {"nome_razao"}, "nome_razao, tipoProf", "desativado = FALSE" + filtroNaoHa, parent);

  sdProfissional->hideColumns({"idLoja", "idUsuarioRel", "idProfissional", "inscEstadual", "contatoNome", "contatoCPF", "contatoApelido", "contatoRG", "banco", "agencia", "cc", "nomeBanco",
                               "cpfBanco", "desativado", "comissao", "poupanca", "cnpjBanco"});

  sdProfissional->setHeaderData("pfpj", "Tipo");
  sdProfissional->setHeaderData("nome_razao", "Profissional");
  sdProfissional->setHeaderData("nomeFantasia", "Fantasia/Apelido");
  sdProfissional->setHeaderData("cpf", "CPF");
  sdProfissional->setHeaderData("cnpj", "CNPJ");
  sdProfissional->setHeaderData("tel", "Tel.");
  sdProfissional->setHeaderData("telCel", "Tel. Cel.");
  sdProfissional->setHeaderData("telCom", "Tel. Com.");
  sdProfissional->setHeaderData("idNextel", "id Nextel");
  sdProfissional->setHeaderData("nextel", "Nextel");
  sdProfissional->setHeaderData("email", "E-mail");
  sdProfissional->setHeaderData("tipoProf", "Profissão");

  return sdProfissional;
}

void SearchDialog::setFornecedorRep(const QString &newFornecedorRep) { fornecedorRep = newFornecedorRep.isEmpty() ? "" : " AND fornecedor = '" + newFornecedorRep + "'"; }

QString SearchDialog::getFilter() const { return filter; }

void SearchDialog::setRepresentacao(const bool isRepresentacao) { this->isRepresentacao = isRepresentacao; }

void SearchDialog::on_radioButtonProdAtivos_toggled(const bool) { on_lineEditBusca_textChanged(QString()); }

void SearchDialog::on_radioButtonProdDesc_toggled(const bool) { on_lineEditBusca_textChanged(QString()); }

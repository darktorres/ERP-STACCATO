#include <QDebug>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>

#include "doubledelegate.h"
#include "porcentagemdelegate.h"
#include "reaisdelegate.h"
#include "searchdialog.h"
#include "searchdialogproxymodel.h"
#include "ui_searchdialog.h"
#include "usersession.h"

SearchDialog::SearchDialog(const QString &title, const QString &table, const QStringList &indexes, const QString &filter, const bool permitirDescontinuados, QWidget *parent)
    : Dialog(parent), indexes(indexes), permitirDescontinuados(permitirDescontinuados), model(50), ui(new Ui::SearchDialog) {
  ui->setupUi(this);

  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &SearchDialog::on_lineEditBusca_textChanged);
  connect(ui->pushButtonSelecionar, &QPushButton::clicked, this, &SearchDialog::on_pushButtonSelecionar_clicked);
  connect(ui->radioButtonProdAtivos, &QRadioButton::toggled, this, &SearchDialog::on_radioButtonProdAtivos_toggled);
  connect(ui->radioButtonProdDesc, &QRadioButton::toggled, this, &SearchDialog::on_radioButtonProdDesc_toggled);
  connect(ui->table, &TableView::doubleClicked, this, &SearchDialog::on_table_doubleClicked);
  connect(ui->table, &TableView::entered, this, &SearchDialog::on_table_entered);

  setWindowTitle(title);
  setWindowModality(Qt::NonModal);
  setWindowFlags(Qt::Window);

  setupTables(table, filter);

  if (indexes.isEmpty()) {
    ui->lineEditBusca->hide();
    ui->labelBusca->hide();
    ui->table->setFocus();
  } else {
    textKeys.append(indexes.first());
    primaryKey = indexes.first();
    ui->lineEditBusca->setFocus();
  }

  ui->radioButtonProdAtivos->hide();
  ui->radioButtonProdDesc->hide();
  ui->lineEditEstoque_2->hide();
  ui->lineEditEstoque->hide();
  ui->lineEditPromocao->hide();
  ui->lineEditBusca->setFocus();
}

SearchDialog::~SearchDialog() { delete ui; }

void SearchDialog::setupTables(const QString &table, const QString &filter) {
  model.setTable(table);
  model.setEditStrategy(QSqlTableModel::OnManualSubmit);
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

  QString searchFilter = "MATCH(" + indexes.join(", ") + ") AGAINST('" + strings.join(" ") + "' IN BOOLEAN MODE)";

  if (model.tableName() == "produto") {
    model.setFilter(searchFilter + " AND descontinuado = " + (ui->radioButtonProdAtivos->isChecked() ? "FALSE" : "TRUE") + " AND desativado = FALSE" + representacao + fornecedorRep);
    return;
  }

  if (not filter.isEmpty()) { searchFilter.append(" AND (" + filter + ")"); }

  model.setFilter(searchFilter);

  if (not model.select()) { return; }
}

void SearchDialog::sendUpdateMessage() {
  const auto selection = ui->table->selectionModel()->selection().indexes();

  if (selection.isEmpty()) { return; }

  emit itemSelected(model.data(selection.first().row(), primaryKey).toString());
}

void SearchDialog::show() {
  model.setFilter(filter);

  if (not model.select()) { return; }

  ui->lineEditBusca->setFocus();
  ui->lineEditBusca->clear();

  QDialog::show();

  ui->lineEditEstoque_2->setText("Staccato OFF");
  ui->lineEditEstoque->setText("Estoque");
  ui->lineEditPromocao->setText("Promoção");

  ui->table->resizeColumnsToContents();
}

void SearchDialog::showMaximized() {
  if (not model.select()) { return; }

  ui->lineEditBusca->setFocus();

  QDialog::showMaximized();
  ui->table->resizeColumnsToContents();
}

void SearchDialog::on_table_doubleClicked(const QModelIndex &) { on_pushButtonSelecionar_clicked(); }

void SearchDialog::setFilter(const QString &value) {
  filter = value;
  model.setFilter(filter);
}

void SearchDialog::hideColumns(const QStringList &columns) {
  for (const auto &column : columns) { ui->table->hideColumn(column); }
}

void SearchDialog::on_pushButtonSelecionar_clicked() {
  if (not permitirDescontinuados and ui->radioButtonProdDesc->isChecked()) {
    QMessageBox::critical(this, "Erro!", "Não pode selecionar produtos descontinuados!\nEntre em contato com o Dept. de Compras!");
    return;
  }

  if (model.tableName() == "produto") {
    const auto selection = ui->table->selectionModel()->selection().indexes();

    if (not selection.isEmpty() and model.data(selection.first().row(), "estoque").toBool()) { emit warningSignal("Verificar com o Dept. de Compras a disponibilidade do estoque antes de vender!"); }
  }

  sendUpdateMessage();
  close();
}

void SearchDialog::setTextKeys(const QStringList &value) { textKeys = value; }

void SearchDialog::setPrimaryKey(const QString &value) { primaryKey = value; }

QString SearchDialog::getText(const QVariant &value) {
  if (model.tableName().contains("endereco") and value == 1) { return "Não há/Retira"; }
  if (value == 0) { return QString(); }

  QString queryText;

  Q_FOREACH (const auto &key, textKeys) { queryText += queryText.isEmpty() ? key : ", " + key; }

  queryText = "SELECT " + queryText + " FROM " + model.tableName() + " WHERE " + primaryKey + " = '" + value.toString() + "'";

  QSqlQuery query(queryText);

  if (not query.exec() or not query.first()) {
    QMessageBox::critical(this, "Erro!", "Erro na query getText: " + query.lastError().text());
    return QString();
  }

  QString res;

  Q_FOREACH (const auto &key, textKeys) {
    if (query.value(key).isValid() and not query.value(key).toString().isEmpty()) { res += (res.isEmpty() ? "" : " - ") + query.value(key).toString(); }
  }

  return res;
}

void SearchDialog::setHeaderData(const QString &column, const QString &value) { model.setHeaderData(column, value); }

SearchDialog *SearchDialog::cliente(QWidget *parent) {
  SearchDialog *sdCliente = new SearchDialog("Buscar Cliente", "cliente", {"nome_razao", "nomeFantasia", "cpf", "cnpj"}, "desativado = FALSE", false, parent);

  sdCliente->setPrimaryKey("idCliente");
  sdCliente->setTextKeys({"nome_razao"});

  sdCliente->hideColumns(
      {"idCliente", "inscEstadual", "idEnderecoFaturamento", "idEnderecoCobranca", "credito", "idEnderecoEntrega", "idUsuarioRel", "idCadastroRel", "idProfissionalRel", "incompleto", "desativado"});

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
  SearchDialog *sdLoja = new SearchDialog("Buscar Loja", "loja", {"descricao, nomeFantasia, razaoSocial"}, "desativado = FALSE", false, parent);

  sdLoja->ui->table->setItemDelegateForColumn("porcentagemFrete", new PorcentagemDelegate(parent));
  sdLoja->ui->table->setItemDelegateForColumn("valorMinimoFrete", new ReaisDelegate(parent));

  sdLoja->setPrimaryKey("idLoja");
  sdLoja->setTextKeys({"nomeFantasia"});

  sdLoja->hideColumns({"idLoja", "idEndereco", "codUF", "desativado"});

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

SearchDialog *SearchDialog::produto(const bool permitirDescontinuados, QWidget *parent) {
  // TODO: 1retornar um SearchDialogProxy direto aqui? (assim o consumidor do codigo nao precisa saber quando nem que
  // precisa usar o proxy)
  SearchDialog *sdProd = new SearchDialog(
      // TODO: 3nao mostrar promocao vencida no descontinuado
      "Buscar Produto", "produto", {"fornecedor", "descricao", "colecao", "codcomercial"}, "idProduto = 0", permitirDescontinuados, parent);

  sdProd->setPrimaryKey("idProduto");
  sdProd->setTextKeys({"descricao"});

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
  SearchDialog *sdFornecedor = new SearchDialog("Buscar Fornecedor", "fornecedor", {"razaoSocial", "nomeFantasia", "contatoCPF", "cnpj"}, "desativado = FALSE", false, parent);

  sdFornecedor->setPrimaryKey("idFornecedor");
  sdFornecedor->setTextKeys({"nomeFantasia", "razaoSocial"});

  sdFornecedor->hideColumns({"aliquotaSt",
                             "coleta",
                             "comissao1",
                             "comissao2",
                             "comissaoLoja",
                             "desativado",
                             "email",
                             "idCadastroRel",
                             "idEnderecoCobranca",
                             "idEnderecoEntrega",
                             "idEnderecoFaturamento",
                             "idFornecedor",
                             "idNextel",
                             "idProfissionalRel",
                             "idUsuarioRel",
                             "inscEstadual",
                             "nextel",
                             "representacao",
                             "st",
                             "tel",
                             "telCel",
                             "telCom"});

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
  SearchDialog *sdTransportadora = new SearchDialog("Buscar Transportadora", "transportadora", {"razaoSocial", "nomeFantasia"}, "desativado = FALSE", false, parent);

  sdTransportadora->setPrimaryKey("idTransportadora");
  sdTransportadora->setTextKeys({"nomeFantasia"});

  sdTransportadora->hideColumns({"idTransportadora", "idEndereco", "desativado"});

  sdTransportadora->setHeaderData("razaoSocial", "Razão Social");
  sdTransportadora->setHeaderData("nomeFantasia", "Nome Fantasia");
  sdTransportadora->setHeaderData("cnpj", "CNPJ");
  sdTransportadora->setHeaderData("inscEstadual", "Insc. Est.");
  sdTransportadora->setHeaderData("antt", "ANTT");
  sdTransportadora->setHeaderData("tel", "Tel.");

  return sdTransportadora;
}

SearchDialog *SearchDialog::veiculo(QWidget *parent) {
  SearchDialog *sdTransportadora = new SearchDialog("Buscar Veículo", "view_busca_veiculo", {"modelo", "placa"}, "desativado = FALSE", false, parent);

  sdTransportadora->setPrimaryKey("idVeiculo");
  sdTransportadora->setTextKeys({"razaoSocial", "modelo", "placa"});

  sdTransportadora->hideColumns({"idVeiculo", "idTransportadora", "desativado"});

  sdTransportadora->setHeaderData("razaoSocial", "Transportadora");
  sdTransportadora->setHeaderData("modelo", "Modelo");
  sdTransportadora->setHeaderData("capacidade", "Carga");
  sdTransportadora->setHeaderData("placa", "Placa");

  return sdTransportadora;
}

SearchDialog *SearchDialog::usuario(QWidget *parent) {
  SearchDialog *sdUsuario = new SearchDialog("Buscar Usuário", "usuario", {"nome, tipo"}, "usuario.desativado = FALSE", false, parent);

  sdUsuario->setPrimaryKey("idUsuario");
  sdUsuario->setTextKeys({"nome"});

  sdUsuario->hideColumns({"idUsuario", "user", "passwd", "desativado"});

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

  const QString filtro = (idLoja == 1) ? "" : " AND idLoja = " + QString::number(idLoja);

  SearchDialog *sdVendedor = new SearchDialog("Buscar Vendedor", "usuario", {"nome, tipo"}, "desativado = FALSE AND (tipo = 'VENDEDOR' OR tipo = 'VENDEDOR ESPECIAL')" + filtro, false, parent);

  sdVendedor->setFilter(UserSession::tipoUsuario() == "ADMINISTRADOR" ? "desativado = FALSE AND (tipo = 'VENDEDOR' OR tipo = 'VENDEDOR ESPECIAL')"
                                                                      : "desativado = FALSE AND (tipo = 'VENDEDOR' OR tipo = 'VENDEDOR ESPECIAL') AND nome != 'REPOSIÇÂO'");

  sdVendedor->setPrimaryKey("idUsuario");
  sdVendedor->setTextKeys({"nome"});

  sdVendedor->hideColumns({"idUsuario", "idLoja", "user", "passwd", "desativado"});

  sdVendedor->setHeaderData("tipo", "Função");
  sdVendedor->setHeaderData("nome", "Nome");
  sdVendedor->setHeaderData("sigla", "Sigla");
  sdVendedor->setHeaderData("email", "E-mail");

  return sdVendedor;
}

SearchDialog *SearchDialog::conta(QWidget *parent) {
  SearchDialog *sdConta = new SearchDialog("Buscar Conta", "loja_has_conta", {}, "", false, parent);

  sdConta->setPrimaryKey("idConta");
  sdConta->setTextKeys({"banco", "agencia", "conta"});

  sdConta->hideColumns({"idConta", "idLoja", "desativado"});

  sdConta->setHeaderData("banco", "Banco");
  sdConta->setHeaderData("agencia", "Agência");
  sdConta->setHeaderData("conta", "Conta");

  return sdConta;
}

SearchDialog *SearchDialog::enderecoCliente(QWidget *parent) {
  SearchDialog *sdEndereco = new SearchDialog("Buscar Endereço", "cliente_has_endereco", {}, "idEndereco = 1", false, parent);

  sdEndereco->setPrimaryKey("idEndereco");
  sdEndereco->setTextKeys({"logradouro", "numero", "bairro", "cidade", "uf"});

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

SearchDialog *SearchDialog::profissional(QWidget *parent) {
  SearchDialog *sdProfissional = new SearchDialog("Buscar Profissional", "profissional", {"nome_razao, tipoProf"}, "desativado = FALSE", false, parent);

  sdProfissional->setPrimaryKey("idProfissional");
  sdProfissional->setTextKeys({"nome_razao"});

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

void SearchDialog::on_table_entered(const QModelIndex &) { ui->table->resizeColumnsToContents(); }

void SearchDialog::setFornecedorRep(const QString &value) { fornecedorRep = value.isEmpty() ? "" : " AND fornecedor = '" + value + "'"; }

QString SearchDialog::getFilter() const { return filter; }

void SearchDialog::setRepresentacao(const bool isRepresentacao) { representacao = isRepresentacao ? " AND representacao = TRUE" : " AND representacao = FALSE"; }

void SearchDialog::on_radioButtonProdAtivos_toggled(const bool) { on_lineEditBusca_textChanged(QString()); }

void SearchDialog::on_radioButtonProdDesc_toggled(const bool) { on_lineEditBusca_textChanged(QString()); }

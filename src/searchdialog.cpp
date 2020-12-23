#include "searchdialog.h"
#include "ui_searchdialog.h"

#include "application.h"
#include "doubledelegate.h"
#include "file.h"
#include "porcentagemdelegate.h"
#include "reaisdelegate.h"
#include "searchdialogproxymodel.h"
#include "usersession.h"
#include "xml.h"

#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QNetworkReply>
#include <QSqlError>
#include <QSqlRecord>

SearchDialog::SearchDialog(const QString &title, const QString &table, const QString &primaryKey, const QStringList &textKeys, const QString &fullTextIndex, const QString &filter, QWidget *parent)
    : QDialog(parent), primaryKey(primaryKey), fullTextIndex(fullTextIndex), textKeys(textKeys), filter(filter), model(1000), ui(new Ui::SearchDialog) {
  ui->setupUi(this);

  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &SearchDialog::on_lineEditBusca_textChanged);
  connect(ui->pushButtonModelo3d, &QPushButton::clicked, this, &SearchDialog::on_pushButtonModelo3d_clicked);
  connect(ui->pushButtonSelecionar, &QPushButton::clicked, this, &SearchDialog::on_pushButtonSelecionar_clicked);
  connect(ui->radioButtonProdAtivos, &QRadioButton::clicked, this, &SearchDialog::on_radioButtonProdAtivos_toggled);
  connect(ui->radioButtonProdDesc, &QRadioButton::clicked, this, &SearchDialog::on_radioButtonProdDesc_toggled);
  connect(ui->table, &TableView::clicked, this, &SearchDialog::on_table_clicked);
  connect(ui->table, &TableView::doubleClicked, this, &SearchDialog::on_table_doubleClicked);

  setWindowTitle(title);
  setWindowModality(Qt::NonModal);
  setWindowFlags(Qt::Window);

  setupTables(table);

  if (fullTextIndex.isEmpty()) {
    ui->lineEditBusca->hide();
    ui->labelBusca->hide();
  }

  ui->lineEditEstoque->hide();
  ui->lineEditEstoque_2->hide();
  ui->lineEditPromocao->hide();
  ui->radioButtonProdAtivos->hide();
  ui->radioButtonProdDesc->hide();
  ui->treeView->hide();
  ui->pushButtonModelo3d->hide();

  ui->lineEditBusca->setFocus();
}

SearchDialog::~SearchDialog() { delete ui; }

void SearchDialog::setupTables(const QString &table) {
  model.setTable(table);

  setFilter(filter);

  model.proxyModel = new SearchDialogProxyModel(&model, this);

  ui->table->setModel(&model);

  ui->table->setItemDelegate(new DoubleDelegate(this));
}

void SearchDialog::on_lineEditBusca_textChanged(const QString &) {
  const QString text = qApp->sanitizeSQL(ui->lineEditBusca->text());

  if (text.isEmpty()) { return model.setFilter(filter); }

  if (model.tableName() == "view_nfe_baixada") { return model.setFilter("numeroNFe LIKE '%" + text + "%' OR xml LIKE '%" + text + "%' OR chaveAcesso LIKE '%" + text + "%'"); }

  QStringList strings = text.split(" ", Qt::SkipEmptyParts);

  for (auto &string : strings) { string.contains("-") ? string.prepend("\"").append("\"") : string.prepend("+").append("*"); }

  QString searchFilter = "MATCH(" + fullTextIndex + ") AGAINST('" + strings.join(" ") + "' IN BOOLEAN MODE)";

  if (model.tableName() == "view_produto") {
    const QString descontinuado = " AND descontinuado = " + QString(ui->radioButtonProdAtivos->isChecked() ? "FALSE" : "TRUE");
    const QString representacao = showAllProdutos ? "" : (isRepresentacao ? " AND representacao = TRUE" : " AND representacao = FALSE");
    const QString showEstoque = compraAvulsa ? " AND estoque = FALSE AND promocao <= 1" : "";

    return model.setFilter(searchFilter + descontinuado + " AND desativado = FALSE" + representacao + showEstoque + fornecedorRep);
  }

  if (not filter.isEmpty()) { searchFilter.append(" AND (" + filter + ")"); }

  model.setFilter(searchFilter);
}

void SearchDialog::sendUpdateMessage(const QModelIndex &index) { emit itemSelected(model.data(index.row(), primaryKey)); }

void SearchDialog::prepare_show() {
  model.setFilter(filter);

  if (not isSet) {
    model.select();
    isSet = true;
  }

  ui->lineEditBusca->setFocus();
  ui->lineEditBusca->clear();

  ui->lineEditEstoque_2->setText("STACCATO OFF");
  ui->lineEditEstoque->setText("Estoque");
  ui->lineEditPromocao->setText("Promoção");
}

void SearchDialog::show() {
  prepare_show();

  QDialog::show();
}

void SearchDialog::showMaximized() {
  prepare_show();

  QDialog::showMaximized();
}

void SearchDialog::on_table_clicked(const QModelIndex &index) {
  if (model.tableName() == "view_nfe_baixada" and index.isValid()) {
    XML *xml = new XML(model.data(index.row(), "xml").toByteArray(), XML::Tipo::Nulo, this);
    ui->treeView->setModel(&xml->model);
    ui->treeView->expandAll();
  }
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
  // TODO: if rowCount == 1 select first line with enter?

  const auto selection = ui->table->selectionModel()->selection().indexes();

  if (selection.isEmpty()) { return; }

  if (not permitirDescontinuados and ui->radioButtonProdDesc->isChecked()) { throw RuntimeError("Não pode selecionar produtos descontinuados!\nEntre em contato com o Dept. de Compras!", this); }

  if (model.tableName() == "view_produto") {
    const bool isEstoque = model.data(selection.first().row(), "estoque").toBool();

    if (not silent and isEstoque) { qApp->enqueueWarning("Verificar com o Dept. de Compras a disponibilidade do estoque antes de vender!", this); }
  }

  close();
  sendUpdateMessage(selection.first());
}

QString SearchDialog::getText(const QVariant &id) {
  if (id == 0) { return QString(); }
  if (model.tableName().contains("endereco") and id == 1) { return "Não há/Retira"; }

  QString queryText;

  for (const auto &key : std::as_const(textKeys)) { queryText += queryText.isEmpty() ? key : ", " + key; }

  queryText = "SELECT " + queryText + " FROM " + model.tableName() + " WHERE " + primaryKey + " = '" + id.toString() + "'";

  SqlQuery query;

  if (not query.exec(queryText) or not query.first()) {
    throw RuntimeException("Erro na query getText: " + query.lastError().text(), this);
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

  sdCliente->ui->lineEditBusca->setPlaceholderText("Cliente/Fantasia/CPF/CNPJ");

  return sdCliente;
}

SearchDialog *SearchDialog::loja(QWidget *parent) {
  SearchDialog *sdLoja = new SearchDialog("Buscar Loja", "loja", "idLoja", {"nomeFantasia"}, "descricao, nomeFantasia, razaoSocial", "desativado = FALSE", parent);

  sdLoja->ui->table->setItemDelegateForColumn("porcentagemFrete", new PorcentagemDelegate(false, parent));
  sdLoja->ui->table->setItemDelegateForColumn("valorMinimoFrete", new ReaisDelegate(parent));

  sdLoja->hideColumns({"idLoja", "codUF", "desativado", "certificadoSerie", "certificadoSenha", "porcentagemPIS", "porcentagemCOFINS", "custoTransporteTon", "custoTransporte1", "custoTransporte2",
                       "custoFuncionario"});

  sdLoja->setHeaderData("descricao", "Descrição");
  sdLoja->setHeaderData("nomeFantasia", "Nome Fantasia");
  sdLoja->setHeaderData("razaoSocial", "Razão Social");
  sdLoja->setHeaderData("tel", "Tel.");
  sdLoja->setHeaderData("tel2", "Tel. 2");
  sdLoja->setHeaderData("inscEstadual", "Insc. Est.");
  sdLoja->setHeaderData("sigla", "Sigla");
  sdLoja->setHeaderData("cnpj", "CNPJ");
  sdLoja->setHeaderData("porcentagemFrete", "Frete");
  sdLoja->setHeaderData("valorMinimoFrete", "Mínimo Frete");

  sdLoja->ui->lineEditBusca->setPlaceholderText("Descrição/Nome Fantasia/Razão Social");

  return sdLoja;
}

SearchDialog *SearchDialog::nfe(QWidget *parent) {
  SearchDialog *sdNFe = new SearchDialog("Buscar NFe", "view_nfe_baixada", "idNFe", {"chaveAcesso"}, "", "", parent);

  sdNFe->ui->lineEditBusca->show();

  sdNFe->ui->table->setAutoResize(false);

  sdNFe->hideColumns({"idNFe", "infCpl", "xml"});
  sdNFe->ui->table->showColumn("created");

  sdNFe->setHeaderData("numeroNFe", "NFe");
  sdNFe->setHeaderData("status", "Status");
  sdNFe->setHeaderData("chaveAcesso", "Chave Acesso");
  sdNFe->setHeaderData("infCpl", "Inf. Comp.");
  sdNFe->setHeaderData("emitente", "Emitente");
  sdNFe->setHeaderData("created", "Data");

  sdNFe->ui->treeView->show();

  sdNFe->ui->lineEditBusca->setPlaceholderText("Número NFe/Chave Acesso/XML");

  sdNFe->resize(1172, 783);

  return sdNFe;
}

SearchDialog *SearchDialog::produto(const bool permitirDescontinuados, const bool silent, const bool showAllProdutos, const bool compraAvulsa, QWidget *parent) {
  // TODO: 3nao mostrar promocao vencida no descontinuado
  SearchDialog *sdProd = new SearchDialog("Buscar Produto", "view_produto", "idProduto", {"descricao"}, "fornecedor, descricao, colecao, codcomercial", "idProduto = 0", parent);

  sdProd->permitirDescontinuados = permitirDescontinuados;
  sdProd->silent = silent;
  sdProd->showAllProdutos = showAllProdutos;
  sdProd->compraAvulsa = compraAvulsa;

  if (compraAvulsa) { sdProd->hideColumns({"statusEstoque", "estoqueRestante", "lote"}); }

  sdProd->hideColumns({"desativado", "descontinuado", "estoque", "promocao", "idProduto", "representacao"});

  for (int column = 0, columnCount = sdProd->model.columnCount(); column < columnCount; ++column) {
    if (sdProd->model.record().fieldName(column).endsWith("Upd")) { sdProd->ui->table->setColumnHidden(column, true); }
  }

  sdProd->setHeaderData("fornecedor", "Fornecedor");
  sdProd->setHeaderData("statusEstoque", "Estoque");
  sdProd->setHeaderData("descricao", "Descrição");
  sdProd->setHeaderData("estoqueRestante", "Estoque Disp.");
  sdProd->setHeaderData("estoqueCaixa", "Estoque Cx.");
  sdProd->setHeaderData("lote", "Lote");
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
  sdProd->ui->pushButtonModelo3d->show();
  sdProd->ui->radioButtonProdAtivos->setChecked(true);

  sdProd->ui->lineEditBusca->setPlaceholderText("Fornecedor/Descrição/Coleção/Código Comercial");

  return sdProd;
}

SearchDialog *SearchDialog::fornecedor(QWidget *parent) {
  SearchDialog *sdFornecedor =
      new SearchDialog("Buscar Fornecedor", "fornecedor", "idFornecedor", {"nomeFantasia", "razaoSocial"}, "razaoSocial, nomeFantasia, contatoCPF, cnpj", "desativado = FALSE", parent);

  sdFornecedor->hideColumns({"aliquotaSt", "comissao1", "comissao2", "comissaoLoja",  "desativado",    "email", "idFornecedor", "idNextel", "inscEstadual", "nextel",    "representacao", "st",
                             "tel",        "telCel",    "telCom",    "especialidade", "fretePagoLoja", "banco", "agencia",      "cc",       "poupanca",     "nomeBanco", "cnpjBanco"});

  sdFornecedor->setHeaderData("razaoSocial", "Razão Social");
  sdFornecedor->setHeaderData("nomeFantasia", "Nome Fantasia");
  sdFornecedor->setHeaderData("contatoNome", "Nome do Contato");
  sdFornecedor->setHeaderData("cnpj", "CNPJ");
  sdFornecedor->setHeaderData("contatoCPF", "CPF do Contato");
  sdFornecedor->setHeaderData("contatoApelido", "Apelido do Contato");
  sdFornecedor->setHeaderData("contatoRG", "RG do Contato");
  sdFornecedor->setHeaderData("validadeProdutos", "Validade Produtos");

  sdFornecedor->ui->lineEditBusca->setPlaceholderText("Razão Social/Nome Fantasia/CNPJ/CPF do Contato");

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

  sdTransportadora->ui->lineEditBusca->setPlaceholderText("Razão Social/Nome Fantasia");

  return sdTransportadora;
}

SearchDialog *SearchDialog::veiculo(QWidget *parent) {
  SearchDialog *sdVeiculo = new SearchDialog("Buscar Veículo", "view_busca_veiculo", "idVeiculo", {"razaoSocial", "modelo", "placa"}, "modelo, placa", "desativado = FALSE", parent);

  sdVeiculo->hideColumns({"idVeiculo", "desativado"});

  sdVeiculo->setHeaderData("razaoSocial", "Transportadora");
  sdVeiculo->setHeaderData("modelo", "Modelo");
  sdVeiculo->setHeaderData("capacidade", "Carga");
  sdVeiculo->setHeaderData("placa", "Placa");

  sdVeiculo->ui->lineEditBusca->setPlaceholderText("Transportadora/Modelo/Placa");

  return sdVeiculo;
}

SearchDialog *SearchDialog::usuario(QWidget *parent) {
  SearchDialog *sdUsuario = new SearchDialog("Buscar Usuário", "view_usuario", "idUsuario", {"nome"}, "nome, tipo", "desativado = FALSE", parent);

  sdUsuario->hideColumns({"idLoja", "idUsuario", "user", "passwd", "especialidade", "desativado"});

  sdUsuario->setHeaderData("descricao", "Loja");
  sdUsuario->setHeaderData("tipo", "Função");
  sdUsuario->setHeaderData("nome", "Nome");
  sdUsuario->setHeaderData("email", "E-mail");

  sdUsuario->ui->lineEditBusca->setPlaceholderText("Função/Nome");

  return sdUsuario;
}

SearchDialog *SearchDialog::vendedor(QWidget *parent) {
  const int idLoja = UserSession::idLoja;
  const bool specialSeller = (UserSession::tipoUsuario == "VENDEDOR ESPECIAL");

  const QString filtro = "desativado = FALSE AND tipo IN ('VENDEDOR', 'VENDEDOR ESPECIAL')";
  const QString filtroLoja = (idLoja == 1 or specialSeller) ? "" : " AND idLoja = " + QString::number(idLoja);
  const QString tipoUsuario = UserSession::tipoUsuario;
  const QString filtroAdmin = (tipoUsuario == "ADMINISTRADOR" or tipoUsuario == "ADMINISTRATIVO") ? "" : " AND nome != 'REPOSIÇÂO'";

  SearchDialog *sdVendedor = new SearchDialog("Buscar Vendedor", "usuario", "idUsuario", {"nome"}, "nome, tipo", filtro + filtroLoja + filtroAdmin, parent);

  sdVendedor->model.setSort("nome");

  sdVendedor->hideColumns({"idUsuario", "idLoja", "user", "passwd", "especialidade", "desativado"});

  sdVendedor->setHeaderData("tipo", "Função");
  sdVendedor->setHeaderData("nome", "Nome");
  sdVendedor->setHeaderData("email", "E-mail");

  sdVendedor->ui->lineEditBusca->setPlaceholderText("Função/Nome");

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

  sdProfissional->ui->lineEditBusca->setPlaceholderText("Profissional/Profissão");

  return sdProfissional;
}

void SearchDialog::setFornecedorRep(const QString &newFornecedorRep) { fornecedorRep = newFornecedorRep.isEmpty() ? "" : " AND fornecedor = '" + newFornecedorRep + "'"; }

QString SearchDialog::getFilter() const { return filter; }

void SearchDialog::setRepresentacao(const bool isRepresentacao) { this->isRepresentacao = isRepresentacao; }

void SearchDialog::on_radioButtonProdAtivos_toggled(const bool) { on_lineEditBusca_textChanged(QString()); }

void SearchDialog::on_radioButtonProdDesc_toggled(const bool) { on_lineEditBusca_textChanged(QString()); }

void SearchDialog::on_pushButtonModelo3d_clicked() {
  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!"); }

  const int row = selection.first().row();

  const QString ip = qApp->getWebDavIp();
  const QString fornecedor = model.data(row, "fornecedor").toString();
  const QString codComercial = model.data(row, "codComercial").toString();

  const QString url = "http://" + ip + "/webdav/METAIS_VIVIANE/MODELOS 3D/" + fornecedor + "/" + codComercial + ".skp";

  auto *manager = new QNetworkAccessManager(this);
  auto request = QNetworkRequest(QUrl(url));
  request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
  auto reply = manager->get(request);

  connect(reply, &QNetworkReply::finished, [=] {
    if (reply->error() != QNetworkReply::NoError) {
      if (reply->error() == QNetworkReply::ContentNotFoundError) { throw RuntimeError("Produto não possui modelo 3D!"); }

      throw RuntimeException("Erro ao baixar arquivo: " + reply->errorString(), this);
    }

    const QString filename = QDir::currentPath() + "/arquivos/" + url.split("/").last();

    File file(filename);

    if (not file.open(QFile::WriteOnly)) { throw RuntimeException("Erro abrindo arquivo para escrita: " + file.errorString(), this); }

    file.write(reply->readAll());

    file.close();

    if (not QDesktopServices::openUrl(QUrl::fromLocalFile(filename))) { throw RuntimeException("Não foi possível abrir o arquivo 3D!"); }
  });
}

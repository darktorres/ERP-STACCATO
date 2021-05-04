#include "inputdialogconfirmacao.h"
#include "ui_inputdialogconfirmacao.h"

#include "application.h"
#include "file.h"
#include "orcamento.h"
#include "sortfilterproxymodel.h"
#include "sqlquery.h"
#include "usersession.h"

#include <QAuthenticator>
#include <QDebug>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSqlError>

InputDialogConfirmacao::InputDialogConfirmacao(const Tipo tipo, QWidget *parent) : QDialog(parent), tipo(tipo), ui(new Ui::InputDialogConfirmacao) {
  ui->setupUi(this);

  setWindowFlags(Qt::Window);

  setupTables();

  ui->dateEditEvento->setDate(qApp->serverDate());
  ui->dateEditProximo->setDate(qApp->serverDate());

  if (tipo == Tipo::Recebimento) {
    ui->labelProximoEvento->hide();
    ui->dateEditProximo->hide();

    ui->labelEvento->setText("Data do recebimento:");

    ui->labelFoto->hide();
    ui->lineEditFoto->hide();
    ui->pushButtonFoto->hide();

    ui->labelEntregou->hide();
    ui->lineEditEntregou->hide();

    ui->pushButtonQuebradoEntrega->hide();
  }

  if (tipo == Tipo::Entrega) {
    ui->labelProximoEvento->hide();
    ui->dateEditProximo->hide();

    ui->labelEvento->setText("Data entrega:");

    ui->pushButtonQuebradoReceb->hide();
  }

  if (tipo == Tipo::Representacao) {
    ui->labelAviso->hide();

    ui->labelProximoEvento->hide();
    ui->dateEditProximo->hide();

    ui->tableLogistica->hide();

    ui->labelFoto->hide();
    ui->lineEditFoto->hide();
    ui->pushButtonFoto->hide();

    ui->labelEntregou->hide();
    ui->lineEditEntregou->hide();

    ui->frameQuebrado->hide();

    adjustSize();
  }

  setConnections();

  show();
}

InputDialogConfirmacao::~InputDialogConfirmacao() { delete ui; }

void InputDialogConfirmacao::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->dateEditEvento, &QDateEdit::dateChanged, this, &InputDialogConfirmacao::on_dateEditEvento_dateChanged, connectionType);
  connect(ui->pushButtonQuebradoReceb, &QPushButton::clicked, this, &InputDialogConfirmacao::on_pushButtonQuebradoReceb_clicked, connectionType);
  connect(ui->pushButtonQuebradoEntrega, &QPushButton::clicked, this, &InputDialogConfirmacao::on_pushButtonQuebradoEntrega_clicked, connectionType);
  connect(ui->pushButtonSalvar, &QPushButton::clicked, this, &InputDialogConfirmacao::on_pushButtonSalvar_clicked, connectionType);
  connect(ui->pushButtonFoto, &QPushButton::clicked, this, &InputDialogConfirmacao::on_pushButtonFoto_clicked, connectionType);
}

QDate InputDialogConfirmacao::getDate() const { return ui->dateEditEvento->date(); }

QDate InputDialogConfirmacao::getNextDateTime() const { return ui->dateEditProximo->date(); }

QString InputDialogConfirmacao::getRecebeu() const { return ui->lineEditRecebeu->text(); }

QString InputDialogConfirmacao::getEntregou() const { return ui->lineEditEntregou->text(); }

void InputDialogConfirmacao::cadastrar() {
  if (tipo == Tipo::Recebimento) { modelEstoque.submitAll(); }

  if (tipo == Tipo::Entrega) { modelVeiculo.submitAll(); }
}

void InputDialogConfirmacao::on_pushButtonSalvar_clicked() {
  if ((tipo == Tipo::Recebimento or tipo == Tipo::Entrega)) {
    if (ui->lineEditRecebeu->text().isEmpty()) { throw RuntimeError("Faltou preencher quem recebeu!", this); }
  }

  if (tipo == Tipo::Entrega) {
    if (ui->lineEditEntregou->text().isEmpty()) { throw RuntimeError("Faltou preencher quem entregou!", this); }
  }

  if (tipo != Tipo::Representacao) {
    qApp->startTransaction("InputDialogConfirmacao::on_pushButtonSalvar");

    cadastrar();

    qApp->endTransaction();
  }

  QDialog::accept();
  close();
}

void InputDialogConfirmacao::on_dateEditEvento_dateChanged(const QDate &date) {
  if (ui->dateEditProximo->date() < date) { ui->dateEditProximo->setDate(date); }
}

void InputDialogConfirmacao::setupTables() {
  if (tipo == Tipo::Recebimento) {
    modelEstoque.setTable("estoque");

    modelEstoque.setHeaderData("status", "Status");
    modelEstoque.setHeaderData("local", "Local");
    modelEstoque.setHeaderData("fornecedor", "Fornecedor");
    modelEstoque.setHeaderData("descricao", "Produto");
    modelEstoque.setHeaderData("observacao", "Obs.");
    modelEstoque.setHeaderData("quant", "Quant.");
    modelEstoque.setHeaderData("un", "Un.");
    modelEstoque.setHeaderData("caixas", "Cx.");
    modelEstoque.setHeaderData("codComercial", "Cód. Com.");
    modelEstoque.setHeaderData("lote", "Lote");
    modelEstoque.setHeaderData("bloco", "Bloco");

    modelEstoque.proxyModel = new SortFilterProxyModel(&modelEstoque, this);

    ui->tableLogistica->setModel(&modelEstoque);

    ui->tableLogistica->hideColumn("restante");
    ui->tableLogistica->hideColumn("ajuste");
    ui->tableLogistica->hideColumn("vBCIPI");
    ui->tableLogistica->hideColumn("pIPI");
    ui->tableLogistica->hideColumn("vIPI");
    ui->tableLogistica->hideColumn("idEstoque");
    ui->tableLogistica->hideColumn("idNFe");
    ui->tableLogistica->hideColumn("recebidoPor");
    ui->tableLogistica->hideColumn("idProduto");
    ui->tableLogistica->hideColumn("quantUpd");
    ui->tableLogistica->hideColumn("codBarras");
    ui->tableLogistica->hideColumn("ncm");
    ui->tableLogistica->hideColumn("nve");
    ui->tableLogistica->hideColumn("extipi");
    ui->tableLogistica->hideColumn("cest");
    ui->tableLogistica->hideColumn("cfop");
    ui->tableLogistica->hideColumn("valor");
    ui->tableLogistica->hideColumn("valorUnid");
    ui->tableLogistica->hideColumn("codBarrasTrib");
    ui->tableLogistica->hideColumn("unTrib");
    ui->tableLogistica->hideColumn("quantTrib");
    ui->tableLogistica->hideColumn("valorUnidTrib");
    ui->tableLogistica->hideColumn("frete");
    ui->tableLogistica->hideColumn("seguro");
    ui->tableLogistica->hideColumn("desconto");
    ui->tableLogistica->hideColumn("outros");
    ui->tableLogistica->hideColumn("compoeTotal");
    ui->tableLogistica->hideColumn("numeroPedido");
    ui->tableLogistica->hideColumn("itemPedido");
    ui->tableLogistica->hideColumn("tipoICMS");
    ui->tableLogistica->hideColumn("orig");
    ui->tableLogistica->hideColumn("cstICMS");
    ui->tableLogistica->hideColumn("modBC");
    ui->tableLogistica->hideColumn("vBC");
    ui->tableLogistica->hideColumn("pICMS");
    ui->tableLogistica->hideColumn("vICMS");
    ui->tableLogistica->hideColumn("modBCST");
    ui->tableLogistica->hideColumn("pMVAST");
    ui->tableLogistica->hideColumn("vBCST");
    ui->tableLogistica->hideColumn("pICMSST");
    ui->tableLogistica->hideColumn("vICMSST");
    ui->tableLogistica->hideColumn("cEnq");
    ui->tableLogistica->hideColumn("cstIPI");
    ui->tableLogistica->hideColumn("cstPIS");
    ui->tableLogistica->hideColumn("vBCPIS");
    ui->tableLogistica->hideColumn("pPIS");
    ui->tableLogistica->hideColumn("vPIS");
    ui->tableLogistica->hideColumn("cstCOFINS");
    ui->tableLogistica->hideColumn("vBCCOFINS");
    ui->tableLogistica->hideColumn("pCOFINS");
    ui->tableLogistica->hideColumn("vCOFINS");
    ui->tableLogistica->hideColumn("valorGare");
  }

  if (tipo == Tipo::Entrega) {
    modelVeiculo.setTable("veiculo_has_produto");

    modelVeiculo.setHeaderData("idVenda", "Venda");
    modelVeiculo.setHeaderData("status", "Status");
    modelVeiculo.setHeaderData("fornecedor", "Fornecedor");
    modelVeiculo.setHeaderData("produto", "Produto");
    modelVeiculo.setHeaderData("obs", "Obs.");
    modelVeiculo.setHeaderData("caixas", "Cx.");
    modelVeiculo.setHeaderData("kg", "Kg.");
    modelVeiculo.setHeaderData("quant", "Quant.");
    modelVeiculo.setHeaderData("un", "Un.");
    modelVeiculo.setHeaderData("quantCaixa", "Quant./Cx.");
    modelVeiculo.setHeaderData("codComercial", "Cód. Com.");
    modelVeiculo.setHeaderData("formComercial", "Form. Com.");

    modelVeiculo.proxyModel = new SortFilterProxyModel(&modelVeiculo, this);

    ui->tableLogistica->setModel(&modelVeiculo);

    ui->tableLogistica->hideColumn("fotoEntrega");
    ui->tableLogistica->hideColumn("id");
    ui->tableLogistica->hideColumn("data");
    ui->tableLogistica->hideColumn("idEvento");
    ui->tableLogistica->hideColumn("idVeiculo");
    ui->tableLogistica->hideColumn("idEstoque");
    ui->tableLogistica->hideColumn("idVendaProduto1");
    ui->tableLogistica->hideColumn("idVendaProduto2");
    ui->tableLogistica->hideColumn("idCompra");
    ui->tableLogistica->hideColumn("idNFeSaida");
    ui->tableLogistica->hideColumn("idLoja");
    ui->tableLogistica->hideColumn("idProduto");
  }
}

void InputDialogConfirmacao::setFilterEntrega(const QString &id, const QString &idEvento) { // entrega
  if (id.isEmpty()) { throw RuntimeException("IdsCompra vazio!"); }

  const QString filter = "idVenda = '" + id + "' AND idEvento = " + idEvento;

  modelVeiculo.setFilter(filter);

  modelVeiculo.select();

  ui->dateEditEvento->setDateTime(modelVeiculo.data(0, "data").toDateTime());

  setWindowTitle("Venda: " + id);
}

void InputDialogConfirmacao::setFilterRecebe(const QStringList &ids) { // recebimento
  if (ids.isEmpty()) { throw RuntimeException("IdsCompra vazio!"); }

  const QString filter = "idEstoque = " + ids.join(" OR idEstoque = ");

  modelEstoque.setFilter(filter);

  modelEstoque.select();

  setWindowTitle("Estoque: " + ids.join(", "));

  ui->pushButtonQuebradoReceb->setDisabled(true); // TODO: remove this after it's fixed
}

void InputDialogConfirmacao::on_pushButtonQuebradoReceb_clicked() {
  // 1. quebrar a linha em 2
  // 2. a parte quebrada fica com status 'quebrado' no limbo
  // 3. a parte que veio prossegue para estoque
  // 4. verificar se precisa desfazer algum consumo caso a quant. nao seja suficiente

  const auto list = ui->tableLogistica->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  const int row = list.first().row();

  // -------------------------------------------------------------------------

  SqlQuery query;
  query.prepare("SELECT quantCaixa FROM produto WHERE idProduto = :idProduto");
  query.bindValue(":idProduto", modelEstoque.data(row, "idProduto"));

  if (not query.exec() or not query.first()) { throw RuntimeException("Erro buscando dados do produto: " + query.lastError().text(), this); }

  const double quantCaixa = query.value("quantCaixa").toDouble();

  const double caixasDefeito = getCaixasDefeito(row);

  // -------------------------------------------------------------------------

  qApp->startTransaction("InputDialogConfirmacao::on_pushButtonQuebrado");

  dividirRecebimento(row, caixasDefeito, quantCaixa);

  qApp->endTransaction();

  qApp->enqueueInformation("Operação realizada com sucesso!", this);
}

void InputDialogConfirmacao::on_pushButtonQuebradoEntrega_clicked() {
  // 1. quebrar a linha em 2
  // 2. a parte quebrada fica com status 'quebrado' no limbo
  // 3. a parte que veio prossegue para estoque
  // 4. verificar se precisa desfazer algum consumo caso a quant. nao seja suficiente

  const auto list = ui->tableLogistica->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  // -------------------------------------------------------------------------

  const int row = list.first().row();
  QString obs;

  QMessageBox msgBox(QMessageBox::Question, "Atenção!", "Criar reposição ou gerar crédito?", QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, this);
  msgBox.setButtonText(QMessageBox::Yes, "Criar reposição");
  msgBox.setButtonText(QMessageBox::No, "Gerar crédito");
  msgBox.setButtonText(QMessageBox::Cancel, "Cancelar");

  const int choice = msgBox.exec();

  if (choice == QMessageBox::Cancel) { return; }
  if (choice == QMessageBox::Yes) { obs = QInputDialog::getText(this, "Observacao", "Observacao: "); }

  const int novoIdVendaProduto2 = qApp->reservarIdVendaProduto2();

  const double caixasDefeito = getCaixasDefeito(row);

  // -------------------------------------------------------------------------

  qApp->startTransaction("InputDialogConfirmacao::on_pushButtonQuebrado");

  dividirEntrega(row, choice, caixasDefeito, obs, novoIdVendaProduto2);

  qApp->endTransaction();

  qApp->enqueueInformation("Operação realizada com sucesso!", this);
}

void InputDialogConfirmacao::criarConsumoQuebrado(const int idEstoque, const double caixasDefeito, const double quantCaixa) {
  SqlQuery query;
  query.prepare("INSERT INTO estoque_has_consumo (idEstoque, status, local, quant, caixas) VALUES (:idEstoque, 'QUEBRADO', 'TEMP', :quant, :caixas)");
  query.bindValue(":idEstoque", idEstoque);
  query.bindValue(":quant", caixasDefeito * quantCaixa);
  query.bindValue(":caixas", caixasDefeito);

  if (not query.exec()) { throw RuntimeException("Erro criando consumo quebrado: " + query.lastError().text()); }
}

void InputDialogConfirmacao::dividirRecebimento(const int row, const double caixasDefeito, const double quantCaixa) {
  // TODO: fazer no recebimento o mesmo fluxo da entrega (criar nova linha, etc)
  // TODO: ao dividir linha fazer prepend '(REPO. ENTREGA/RECEB.)' na observacao do produto
  // TODO: nao dividir linha do estoque, apenas criar um consumo 'quebrado' para inutilizar a parte que foi quebrada (a mesma coisa para faltando)

  // TODO: 0finish this part

  const int idEstoque = modelEstoque.data(row, "idEstoque").toInt();
  const double caixas = modelEstoque.data(row, "caixas").toDouble();
  Q_UNUSED(caixas)
  Q_UNUSED(quantCaixa)

  desfazerConsumo(idEstoque, caixasDefeito);
  criarConsumoQuebrado(idEstoque, caixasDefeito, quantCaixa);

  // ****

  // criar consumo 'quebra' (consumindo sobra) na tabela estoque_has_consumo e tambem em pedido_fornecedor
  // assim nao terei divergencia em relacao a quant. disponivel de estoque
  // como agora eu faço consumo compra e nao estoque, devo diminuir a quantidade em pedido_fornecedor
  // ou a quant. do estoque?

  // se sobra nao era suficiente para acomodar quebra: pegar consumo com maior prazo e desvincular
  // produto<>consumo
  // tem uma funcao pronta em 'devolucao' mas basicamente é apagar em estoque_has_consumo e dessacioar em
  // pedido_fornecedor

  // ****
  // 10 peças, 5 consumo, 3 quebradas
  // verificar se quant quebrada < quant consumo manter consumo senao ajustar

  // 1. criar linha consumo 'QUEBRADO'
  // 2. na venda subtrair do produto a quant quebrada, recalcular valores
  // 3. copiar linha com quant quebrada e status 'pendente', obs 'reposicao' (para ir para tela de
  //  pendentes_compra)
  // -
  // recalcular valores
}

void InputDialogConfirmacao::dividirEntrega(const int row, const int choice, const double caixasDefeito, const QString obs, const int novoIdVendaProduto2) {
  // NOTE: na tabela veiculo_has_produto é separado a linha em 2:
  // -linha original mantem a quant. entregue
  // -linha nova mostra a quant. quebrada

  // na tabela venda_has_produto é separado a linha em:
  // -linha original mantem a quant. entregue
  // -linha nova mostar a quant. quebrada
  // -caso tenha reposicao é criada uma terceira linha 'repo.' (mesma quant. da 'quebrada')

  const double caixas = modelVeiculo.data(row, "caixas").toDouble();
  const QString idVendaProduto2 = modelVeiculo.data(row, "idVendaProduto2").toString();
  const double quantCaixa = modelVeiculo.data(row, "quantCaixa").toDouble();

  SqlTableModel modelVendaProduto;
  modelVendaProduto.setTable("venda_has_produto2");

  modelVendaProduto.setFilter("idVendaProduto2 = " + modelVeiculo.data(row, "idVendaProduto2").toString());

  modelVendaProduto.select();

  if (modelVendaProduto.rowCount() == 0) { return; }

  // -------------------------------------------------------------------------

  dividirVenda(modelVendaProduto, caixas, caixasDefeito, quantCaixa, novoIdVendaProduto2);

  // -------------------------------------------------------------------------

  (choice == QMessageBox::Yes) ? criarReposicaoCliente(modelVendaProduto, caixasDefeito, quantCaixa, obs, novoIdVendaProduto2) : gerarCreditoCliente(modelVendaProduto, caixasDefeito, quantCaixa);

  modelVendaProduto.submitAll();

  // -------------------------------------------------------------------------

  dividirVeiculo(row, caixas, caixasDefeito, quantCaixa, novoIdVendaProduto2);

  // -------------------------------------------------------------------------

  dividirConsumo(caixas, caixasDefeito, quantCaixa, novoIdVendaProduto2, idVendaProduto2);

  // -------------------------------------------------------------------------

  dividirCompra(caixas, caixasDefeito, quantCaixa, novoIdVendaProduto2, idVendaProduto2);
}

void InputDialogConfirmacao::gerarCreditoCliente(const SqlTableModel &modelVendaProduto, const double caixasDefeito, const double quantCaixa) {
  const QString idVenda = modelVendaProduto.data(0, "idVenda").toString();
  const double descUnitario = modelVendaProduto.data(0, "descUnitario").toDouble();

  const double credito = caixasDefeito * quantCaixa * descUnitario;

  qApp->enqueueInformation("Gerado crédito no valor de R$ " + QLocale(QLocale::Portuguese).toString(credito), this);

  SqlTableModel modelCliente;
  modelCliente.setTable("cliente");

  // TODO: since this is inside a transaction simplify using a UPDATE query: credito = credito + novoCredito
  SqlQuery query;
  query.prepare("SELECT idCliente FROM venda WHERE idVenda = :idVenda");
  query.bindValue(":idVenda", idVenda);

  if (not query.exec() or not query.first()) { throw RuntimeException("Erro buscando cliente: " + query.lastError().text()); }

  modelCliente.setFilter("idCliente = " + query.value("idCliente").toString());

  modelCliente.select();

  const double creditoAntigo = modelCliente.data(0, "credito").toDouble();

  modelCliente.setData(0, "credito", credito + creditoAntigo);

  modelCliente.submitAll();
}

void InputDialogConfirmacao::criarReposicaoCliente(SqlTableModel &modelVendaProduto, const double caixasDefeito, const double quantCaixa, const QString obs, const int novoIdVendaProduto2) {
  const int newRow = modelVendaProduto.insertRowAtEnd();
  // NOTE: *quebralinha venda_produto2

  // copiar linha com quantidade quebrada
  for (int col = 0; col < modelVendaProduto.columnCount(); ++col) {
    if (modelVendaProduto.fieldIndex("idVendaProduto2") == col) { continue; }
    if (modelVendaProduto.fieldIndex("entregou") == col) { continue; }
    if (modelVendaProduto.fieldIndex("idCompra") == col) { continue; }
    if (modelVendaProduto.fieldIndex("idNFeSaida") == col) { continue; }
    if (modelVendaProduto.fieldIndex("dataPrevCompra") == col) { continue; }
    if (modelVendaProduto.fieldIndex("dataRealCompra") == col) { continue; }
    if (modelVendaProduto.fieldIndex("dataPrevConf") == col) { continue; }
    if (modelVendaProduto.fieldIndex("dataRealConf") == col) { continue; }
    if (modelVendaProduto.fieldIndex("dataPrevFat") == col) { continue; }
    if (modelVendaProduto.fieldIndex("dataRealFat") == col) { continue; }
    if (modelVendaProduto.fieldIndex("dataPrevColeta") == col) { continue; }
    if (modelVendaProduto.fieldIndex("dataRealColeta") == col) { continue; }
    if (modelVendaProduto.fieldIndex("dataPrevReceb") == col) { continue; }
    if (modelVendaProduto.fieldIndex("dataRealReceb") == col) { continue; }
    if (modelVendaProduto.fieldIndex("dataPrevEnt") == col) { continue; }
    if (modelVendaProduto.fieldIndex("created") == col) { continue; }
    if (modelVendaProduto.fieldIndex("lastUpdated") == col) { continue; }

    const QVariant value = modelVendaProduto.data(0, col);

    if (value.isNull()) { continue; }

    modelVendaProduto.setData(newRow, col, value);
  }

  modelVendaProduto.setData(newRow, "idRelacionado", novoIdVendaProduto2);
  modelVendaProduto.setData(newRow, "quant", caixasDefeito * quantCaixa);
  modelVendaProduto.setData(newRow, "caixas", caixasDefeito);
  modelVendaProduto.setData(newRow, "prcUnitario", 0);
  modelVendaProduto.setData(newRow, "descUnitario", 0);
  modelVendaProduto.setData(newRow, "parcial", 0);
  modelVendaProduto.setData(newRow, "desconto", 0);
  modelVendaProduto.setData(newRow, "parcialDesc", 0);
  modelVendaProduto.setData(newRow, "descGlobal", 0);
  modelVendaProduto.setData(newRow, "total", 0);
  modelVendaProduto.setData(newRow, "status", "REPO. ENTREGA");
  modelVendaProduto.setData(newRow, "reposicaoEntrega", true);

  modelVendaProduto.setData(newRow, "obs", "(REPO. ENTREGA) " + obs);
}

void InputDialogConfirmacao::desfazerConsumo(const int idEstoque, const double caixasDefeito) {
  // TODO: pass this responsability to Estoque class
  // NOTE: verificar WidgetCompraConsumos::desfazerConsumo

  SqlQuery query;
  query.prepare("SELECT restante FROM estoque WHERE idEstoque = :idEstoque");
  query.bindValue(":idEstoque", idEstoque);

  if (not query.exec() or not query.first()) { throw RuntimeException("Erro buscando sobra estoque: " + query.lastError().text()); }

  double restante = query.value("restante").toDouble(); // TODO: divide this by quantCaixa
  qDebug() << "sobra: " << restante;
  qDebug() << "caixasDefeito: " << caixasDefeito;

  if (restante < 0) { // faltando pecas para consumo, desfazer os consumos com prazo maior
    SqlQuery querySelect;
    querySelect.prepare(
        "SELECT CAST((`v`.`data` + INTERVAL `v`.`prazoEntrega` DAY) AS DATE) AS `prazoEntrega`, ehc.* FROM estoque_has_consumo ehc LEFT JOIN venda_has_produto2 vp2 ON ehc.idVendaProduto2 = "
        "vp2.idVendaProduto2 LEFT JOIN venda v ON vp2.idVenda = v.idVenda WHERE ehc.idEstoque = :idEstoque ORDER BY prazoEntrega DESC");
    querySelect.bindValue(":idEstoque", idEstoque);

    if (not querySelect.exec()) { throw RuntimeException("Erro buscando consumo estoque: " + querySelect.lastError().text()); }

    SqlQuery queryDelete;
    // TODO: 0se a parte não quebrada for suficiente nao desfazer consumos (tomar cuidado para usar o idEstoque do restante e nao do quebrado)
    queryDelete.prepare("DELETE FROM estoque_has_consumo WHERE idConsumo = :idConsumo");

    SqlQuery queryVenda;
    // TODO: should set flag 'reposicao' (where is this flag unset?)
    queryVenda.prepare("UPDATE venda_has_produto2 SET status = 'REPO. RECEB.', dataPrevEnt = NULL WHERE idVendaProduto2 = :idVendaProduto2 AND status NOT IN ('CANCELADO', 'DEVOLVIDO')");

    while (querySelect.next()) {
      const int caixas = querySelect.value("caixas").toInt();

      queryDelete.bindValue(":idConsumo", querySelect.value("idConsumo"));

      if (not queryDelete.exec()) { throw RuntimeException("Erro removendo consumo: " + queryDelete.lastError().text()); }

      queryVenda.bindValue(":idVendaProduto2", querySelect.value("idVendaProduto2"));

      if (not queryVenda.exec()) { throw RuntimeException("Erro voltando produto para pendente: " + queryVenda.lastError().text()); }

      restante += caixas;
      if (restante >= 0) { break; }
    }
  }
}

void InputDialogConfirmacao::on_pushButtonFoto_clicked() {
  const QString filePath = QFileDialog::getOpenFileName(this, "Imagens", "", "(*.jpg *.jpeg *.png *.tif *.bmp *.pdf)");

  if (filePath.isEmpty()) { return; }

  File file(filePath);

  if (not file.open(QFile::ReadOnly)) { throw RuntimeException("Erro lendo arquivo: " + file.errorString(), this); }

  auto *manager = new QNetworkAccessManager(this);

  connect(manager, &QNetworkAccessManager::authenticationRequired, this, [&](QNetworkReply *reply, QAuthenticator *authenticator) {
    Q_UNUSED(reply)

    authenticator->setUser(UserSession::_user);
    authenticator->setPassword(UserSession::_password);
  });

  const QString ip = qApp->getWebDavIp();
  const QString idVenda = modelVeiculo.data(0, "idVenda").toString();
  const QString idEvento = modelVeiculo.data(0, "idEvento").toString();

  QFileInfo info(file);

  const QString extension = info.suffix();

  const QString url = "http://" + ip + "/webdav/FOTOS ENTREGAS/" + idVenda + " - " + idEvento + "." + extension;

  const auto fileContent = file.readAll();

  manager->put(QNetworkRequest(QUrl(url)), fileContent);

  ui->lineEditFoto->setText("Enviando...");

  connect(manager, &QNetworkAccessManager::finished, this, [=](QNetworkReply *reply) {
    const QUrl redirect = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();

    if (redirect.isValid()) {
      manager->put(QNetworkRequest(redirect), fileContent);
      return;
    }

    if (reply->error() != QNetworkReply::NoError) {
      ui->lineEditFoto->setStyleSheet("background-color: rgb(255, 0, 0); color: rgb(0, 0, 0);");
      throw RuntimeException("Erro enviando foto: " + reply->errorString());
    }

    ui->lineEditFoto->setText(reply->url().toString());
    ui->lineEditFoto->setStyleSheet("background-color: rgb(0, 255, 0); color: rgb(0, 0, 0);");

    for (int row = 0; row < modelVeiculo.rowCount(); ++row) { modelVeiculo.setData(row, "fotoEntrega", reply->url().toString()); }
  });
}

double InputDialogConfirmacao::getCaixasDefeito(const int row) {
  QString produto;
  double caixas = 0;

  if (tipo == Tipo::Recebimento) {
    produto = modelEstoque.data(row, "descricao").toString();
    caixas = modelEstoque.data(row, "caixas").toDouble();
  }

  if (tipo == Tipo::Entrega) {
    produto = modelVeiculo.data(row, "produto").toString();
    caixas = modelVeiculo.data(row, "caixas").toDouble();
  }

  bool ok;

  const double caixasDefeito = QInputDialog::getDouble(this, produto, "Caixas quebradas: ", caixas, 0, caixas, 1, &ok);

  if (not ok or qFuzzyIsNull(caixasDefeito)) { throw std::exception(); }

  return caixasDefeito;
}

void InputDialogConfirmacao::dividirVenda(SqlTableModel &modelVendaProduto, const double caixas, const double caixasDefeito, const double quantCaixa, const int novoIdVendaProduto2) {
  const double caixasRestante = caixas - caixasDefeito;
  const double quantRestante = caixasRestante * quantCaixa;

  modelVendaProduto.setData(0, "caixas", caixasRestante);
  modelVendaProduto.setData(0, "quant", quantRestante);

  const double prcUnitario = modelVendaProduto.data(0, "prcUnitario").toDouble();
  const double descUnitario = modelVendaProduto.data(0, "descUnitario").toDouble();
  const double descGlobal = modelVendaProduto.data(0, "descGlobal").toDouble() / 100;

  modelVendaProduto.setData(0, "parcial", quantRestante * prcUnitario);
  modelVendaProduto.setData(0, "parcialDesc", quantRestante * descUnitario);
  modelVendaProduto.setData(0, "total", quantRestante * descUnitario * (1 - descGlobal));

  // -------------------------------------------------------------------------

  const int rowQuebrado2 = modelVendaProduto.insertRowAtEnd();
  // NOTE: *quebralinha venda_produto2

  for (int col = 0; col < modelVendaProduto.columnCount(); ++col) {
    if (modelVendaProduto.fieldIndex("idVendaProduto2") == col) { continue; }
    //    if (modelVendaProduto.fieldIndex("entregou") == col) { continue; }
    if (modelVendaProduto.fieldIndex("idCompra") == col) { continue; }
    //    if (modelVendaProduto.fieldIndex("idNFeSaida") == col) { continue; }
    if (modelVendaProduto.fieldIndex("dataPrevCompra") == col) { continue; }
    if (modelVendaProduto.fieldIndex("dataRealCompra") == col) { continue; }
    if (modelVendaProduto.fieldIndex("dataPrevConf") == col) { continue; }
    if (modelVendaProduto.fieldIndex("dataRealConf") == col) { continue; }
    if (modelVendaProduto.fieldIndex("dataPrevFat") == col) { continue; }
    if (modelVendaProduto.fieldIndex("dataRealFat") == col) { continue; }
    if (modelVendaProduto.fieldIndex("dataPrevColeta") == col) { continue; }
    if (modelVendaProduto.fieldIndex("dataRealColeta") == col) { continue; }
    if (modelVendaProduto.fieldIndex("dataPrevReceb") == col) { continue; }
    if (modelVendaProduto.fieldIndex("dataRealReceb") == col) { continue; }
    if (modelVendaProduto.fieldIndex("dataPrevEnt") == col) { continue; }
    if (modelVendaProduto.fieldIndex("created") == col) { continue; }
    if (modelVendaProduto.fieldIndex("lastUpdated") == col) { continue; }

    const QVariant value = modelVendaProduto.data(0, col);

    if (value.isNull()) { continue; }

    modelVendaProduto.setData(rowQuebrado2, col, value);
  }

  const double quantDefeito = caixasDefeito * quantCaixa;

  modelVendaProduto.setData(rowQuebrado2, "idVendaProduto2", novoIdVendaProduto2);
  modelVendaProduto.setData(rowQuebrado2, "idRelacionado", modelVendaProduto.data(0, "idVendaProduto2"));
  modelVendaProduto.setData(rowQuebrado2, "caixas", caixasDefeito);
  modelVendaProduto.setData(rowQuebrado2, "quant", quantDefeito);
  modelVendaProduto.setData(rowQuebrado2, "status", "QUEBRADO");

  modelVendaProduto.setData(rowQuebrado2, "parcial", quantDefeito * prcUnitario);
  modelVendaProduto.setData(rowQuebrado2, "parcialDesc", quantDefeito * descUnitario);
  modelVendaProduto.setData(rowQuebrado2, "total", quantDefeito * descUnitario * (1 - descGlobal));
}

void InputDialogConfirmacao::dividirVeiculo(const int row, const double caixas, const double caixasDefeito, const double quantCaixa, const int novoIdVendaProduto2) {
  // diminuir quantidade da linha selecionada

  // recalcular kg? (posso usar proporcao para nao precisar puxar kgcx)
  modelVeiculo.setData(row, "caixas", caixas - caixasDefeito);
  modelVeiculo.setData(row, "quant", (caixas - caixasDefeito) * quantCaixa);

  // copiar linha com quantDefeito

  const int rowQuebrado = modelVeiculo.insertRowAtEnd();

  for (int col = 0; col < modelVeiculo.columnCount(); ++col) {
    if (modelVeiculo.fieldIndex("id") == col) { continue; }
    if (modelVeiculo.fieldIndex("created") == col) { continue; }
    if (modelVeiculo.fieldIndex("lastUpdated") == col) { continue; }

    const QVariant value = modelVeiculo.data(row, col);

    if (value.isNull()) { continue; }

    modelVeiculo.setData(rowQuebrado, col, value);
  }

  // recalcular kg? (posso usar proporcao para nao precisar puxar kgcx)
  modelVeiculo.setData(rowQuebrado, "idVendaProduto2", novoIdVendaProduto2);
  modelVeiculo.setData(rowQuebrado, "caixas", caixasDefeito);
  modelVeiculo.setData(rowQuebrado, "quant", caixasDefeito * quantCaixa);
  modelVeiculo.setData(rowQuebrado, "status", "QUEBRADO");

  modelVeiculo.submitAll();
}

void InputDialogConfirmacao::dividirConsumo(const double caixas, const double caixasDefeito, const double quantCaixa, const int novoIdVendaProduto2, const QString idVendaProduto2) {
  SqlTableModel modelConsumo;
  modelConsumo.setTable("estoque_has_consumo");

  modelConsumo.setFilter("idVendaProduto2 = " + idVendaProduto2);

  modelConsumo.select();

  if (modelConsumo.rowCount() == 0) { return; }

  // -------------------------------------------------------------------------
  // NOTE: *quebralinha estoque_consumo

  const double caixasRestante = caixas - caixasDefeito;
  const double quantRestante = caixasRestante * quantCaixa;
  const double valorConsumo = modelConsumo.data(0, "valor").toDouble();

  const double quantTrib = modelConsumo.data(0, "quantTrib").toDouble();
  const double desconto = modelConsumo.data(0, "desconto").toDouble();
  const double vBC = modelConsumo.data(0, "vBC").toDouble();
  const double vICMS = modelConsumo.data(0, "vICMS").toDouble();
  const double vBCST = modelConsumo.data(0, "vBCST").toDouble();
  const double vICMSST = modelConsumo.data(0, "vICMSST").toDouble();
  const double vBCPIS = modelConsumo.data(0, "vBCPIS").toDouble();
  const double vPIS = modelConsumo.data(0, "vPIS").toDouble();
  const double vBCCOFINS = modelConsumo.data(0, "vBCCOFINS").toDouble();
  const double vCOFINS = modelConsumo.data(0, "vCOFINS").toDouble();

  const double proporcao = caixasRestante / caixas;

  modelConsumo.setData(0, "quant", quantRestante * -1);
  modelConsumo.setData(0, "caixas", caixasRestante);
  modelConsumo.setData(0, "valor", valorConsumo * proporcao);

  // impostos
  modelConsumo.setData(0, "quantTrib", quantTrib * proporcao);
  modelConsumo.setData(0, "desconto", desconto * proporcao);
  modelConsumo.setData(0, "vBC", vBC * proporcao);
  modelConsumo.setData(0, "vICMS", vICMS * proporcao);
  modelConsumo.setData(0, "vBCST", vBCST * proporcao);
  modelConsumo.setData(0, "vICMSST", vICMSST * proporcao);
  modelConsumo.setData(0, "vBCPIS", vBCPIS * proporcao);
  modelConsumo.setData(0, "vPIS", vPIS * proporcao);
  modelConsumo.setData(0, "vBCCOFINS", vBCCOFINS * proporcao);
  modelConsumo.setData(0, "vCOFINS", vCOFINS * proporcao);

  // -------------------------------------------------------------------------

  // copiar linha
  const int newRow = modelConsumo.insertRowAtEnd();

  for (int column = 0, columnCount = modelConsumo.columnCount(); column < columnCount; ++column) {
    if (column == modelConsumo.fieldIndex("idConsumo")) { continue; }
    if (column == modelConsumo.fieldIndex("idVendaProduto2")) { continue; }
    if (column == modelConsumo.fieldIndex("created")) { continue; }
    if (column == modelConsumo.fieldIndex("lastUpdated")) { continue; }

    const QVariant value = modelConsumo.data(0, column);

    if (value.isNull()) { continue; }

    modelConsumo.setData(newRow, column, value);
  }

  // -------------------------------------------------------------------------

  const double proporcaoNovo = caixasDefeito / caixas;

  modelConsumo.setData(newRow, "idVendaProduto2", novoIdVendaProduto2);
  modelConsumo.setData(newRow, "status", "QUEBRADO");
  modelConsumo.setData(newRow, "quant", caixasDefeito * quantCaixa * -1);
  modelConsumo.setData(newRow, "caixas", caixasDefeito);
  modelConsumo.setData(newRow, "valor", valorConsumo * proporcaoNovo);

  // impostos
  modelConsumo.setData(newRow, "quantTrib", quantTrib * proporcaoNovo);
  modelConsumo.setData(newRow, "desconto", desconto * proporcaoNovo);
  modelConsumo.setData(newRow, "vBC", vBC * proporcaoNovo);
  modelConsumo.setData(newRow, "vICMS", vICMS * proporcaoNovo);
  modelConsumo.setData(newRow, "vBCST", vBCST * proporcaoNovo);
  modelConsumo.setData(newRow, "vICMSST", vICMSST * proporcaoNovo);
  modelConsumo.setData(newRow, "vBCPIS", vBCPIS * proporcaoNovo);
  modelConsumo.setData(newRow, "vPIS", vPIS * proporcaoNovo);
  modelConsumo.setData(newRow, "vBCCOFINS", vBCCOFINS * proporcaoNovo);
  modelConsumo.setData(newRow, "vCOFINS", vCOFINS * proporcaoNovo);

  // -------------------------------------------------------------------------

  modelConsumo.submitAll();
}

void InputDialogConfirmacao::dividirCompra(const double caixas, const double caixasDefeito, const double quantCaixa, const int novoIdVendaProduto2, const QString idVendaProduto2) {
  SqlTableModel modelCompra;
  modelCompra.setTable("pedido_fornecedor_has_produto2");

  modelCompra.setFilter("idVendaProduto2 = " + idVendaProduto2);

  modelCompra.select();

  if (modelCompra.rowCount() == 0) { return; }

  // -------------------------------------------------------------------------

  const double prcUnitario = modelCompra.data(0, "prcUnitario").toDouble();
  const double caixasRestante = caixas - caixasDefeito;
  const double quantRestante = caixasRestante * quantCaixa;

  modelCompra.setData(0, "quant", quantRestante);
  modelCompra.setData(0, "caixas", caixasRestante);
  modelCompra.setData(0, "preco", quantRestante * prcUnitario);

  // -------------------------------------------------------------------------

  // NOTE: *quebralinha pedido_fornecedor2
  const int newRow = modelCompra.insertRowAtEnd();

  for (int column = 0, columnCount = modelCompra.columnCount(); column < columnCount; ++column) {
    if (column == modelCompra.fieldIndex("idPedido2")) { continue; }
    if (column == modelCompra.fieldIndex("created")) { continue; }
    if (column == modelCompra.fieldIndex("lastUpdated")) { continue; }

    const QVariant value = modelCompra.data(0, column);

    if (value.isNull()) { continue; }

    modelCompra.setData(newRow, column, value);
  }

  // -------------------------------------------------------------------------

  modelCompra.setData(newRow, "idRelacionado", modelCompra.data(0, "idPedido2"));
  modelCompra.setData(newRow, "idVendaProduto2", novoIdVendaProduto2);
  modelCompra.setData(newRow, "quant", caixasDefeito * quantCaixa);
  modelCompra.setData(newRow, "caixas", caixasDefeito);
  modelCompra.setData(newRow, "preco", caixasDefeito * quantCaixa * prcUnitario);

  // -------------------------------------------------------------------------

  modelCompra.submitAll();
}

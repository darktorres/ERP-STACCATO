#include "inputdialogconfirmacao.h"
#include "ui_inputdialogconfirmacao.h"

#include "application.h"
#include "orcamento.h"
#include "sortfilterproxymodel.h"

#include <QDebug>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSqlError>
#include <QSqlQuery>

InputDialogConfirmacao::InputDialogConfirmacao(const Tipo tipo, QWidget *parent) : QDialog(parent), tipo(tipo), ui(new Ui::InputDialogConfirmacao) {
  ui->setupUi(this);

  connect(ui->dateEditEvento, &QDateEdit::dateChanged, this, &InputDialogConfirmacao::on_dateEditEvento_dateChanged);
  connect(ui->pushButtonQuebradoReceb, &QPushButton::clicked, this, &InputDialogConfirmacao::on_pushButtonQuebradoReceb_clicked);
  connect(ui->pushButtonQuebradoEntrega, &QPushButton::clicked, this, &InputDialogConfirmacao::on_pushButtonQuebradoEntrega_clicked);
  connect(ui->pushButtonSalvar, &QPushButton::clicked, this, &InputDialogConfirmacao::on_pushButtonSalvar_clicked);
  connect(ui->pushButtonFoto, &QPushButton::clicked, this, &InputDialogConfirmacao::on_pushButtonFoto_clicked);

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

  show();
}

InputDialogConfirmacao::~InputDialogConfirmacao() { delete ui; }

QDate InputDialogConfirmacao::getDate() const { return ui->dateEditEvento->date(); }

QDate InputDialogConfirmacao::getNextDateTime() const { return ui->dateEditProximo->date(); }

QString InputDialogConfirmacao::getRecebeu() const { return ui->lineEditRecebeu->text(); }

QString InputDialogConfirmacao::getEntregou() const { return ui->lineEditEntregou->text(); }

bool InputDialogConfirmacao::cadastrar() {
  if (tipo == Tipo::Recebimento) {
    if (not modelEstoque.submitAll()) { return false; }
  }

  if (tipo == Tipo::Entrega) {
    if (not modelVeiculo.submitAll()) { return false; }
  }

  return true;
}

void InputDialogConfirmacao::on_pushButtonSalvar_clicked() {
  if ((tipo == Tipo::Recebimento or tipo == Tipo::Entrega)) {
    if (ui->lineEditRecebeu->text().isEmpty()) { return qApp->enqueueError("Faltou preencher quem recebeu!", this); }
  }

  if (tipo == Tipo::Entrega) {
    if (ui->lineEditEntregou->text().isEmpty()) { return qApp->enqueueError("Faltou preencher quem entregou!", this); }
  }

  if (tipo != Tipo::Representacao) {
    if (not qApp->startTransaction("InputDialogConfirmacao::on_pushButtonSalvar")) { return; }

    if (not cadastrar()) { return qApp->rollbackTransaction(); }

    if (not qApp->endTransaction()) { return; }
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
    ui->tableLogistica->hideColumn("valorGare");
    ui->tableLogistica->hideColumn("idEstoque");
    ui->tableLogistica->hideColumn("idNFe");
    ui->tableLogistica->hideColumn("recebidoPor");
    ui->tableLogistica->hideColumn("idProduto");
    ui->tableLogistica->hideColumn("quantUpd");
    ui->tableLogistica->hideColumn("codBarras");
    ui->tableLogistica->hideColumn("ncm");
    ui->tableLogistica->hideColumn("cfop");
    ui->tableLogistica->hideColumn("valor");
    ui->tableLogistica->hideColumn("valorUnid");
    ui->tableLogistica->hideColumn("codBarrasTrib");
    ui->tableLogistica->hideColumn("unTrib");
    ui->tableLogistica->hideColumn("quantTrib");
    ui->tableLogistica->hideColumn("valorTrib");
    ui->tableLogistica->hideColumn("desconto");
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

bool InputDialogConfirmacao::setFilterEntrega(const QString &id, const QString &idEvento) { // entrega
  if (id.isEmpty()) { return qApp->enqueueError(false, "IdsCompra vazio!", this); }

  const QString filter = "idVenda = '" + id + "' AND idEvento = " + idEvento;

  modelVeiculo.setFilter(filter);

  if (not modelVeiculo.select()) { return false; }

  ui->dateEditEvento->setDateTime(modelVeiculo.data(0, "data").toDateTime());

  setWindowTitle("Venda: " + id);

  return true;
}

bool InputDialogConfirmacao::setFilterRecebe(const QStringList &ids) { // recebimento
  if (ids.isEmpty()) { return qApp->enqueueError(false, "IdsCompra vazio!", this); }

  const QString filter = "idEstoque = " + ids.join(" OR idEstoque = ");

  modelEstoque.setFilter(filter);

  if (not modelEstoque.select()) { return false; }

  setWindowTitle("Estoque: " + ids.join(", "));

  ui->pushButtonQuebradoReceb->setDisabled(true); // TODO: remove this after it's fixed

  return true;
}

void InputDialogConfirmacao::on_pushButtonQuebradoReceb_clicked() {
  // 1. quebrar a linha em 2
  // 2. a parte quebrada fica com status 'quebrado' no limbo
  // 3. a parte que veio prossegue para estoque
  // 4. verificar se precisa desfazer algum consumo caso a quant. nao seja suficiente

  const auto list = ui->tableLogistica->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhum item selecionado!", this); }

  const int row = list.first().row();

  // -------------------------------------------------------------------------

  QSqlQuery query;
  query.prepare("SELECT quantCaixa FROM produto WHERE idProduto = :idProduto");
  query.bindValue(":idProduto", modelEstoque.data(row, "idProduto"));

  if (not query.exec() or not query.first()) { return qApp->enqueueError("Erro buscando dados do produto: " + query.lastError().text(), this); }

  const double quantCaixa = query.value("quantCaixa").toDouble();

  const auto caixasDefeito = getCaixasDefeito(row);

  if (not caixasDefeito) { return; }

  // -------------------------------------------------------------------------

  if (not qApp->startTransaction("InputDialogConfirmacao::on_pushButtonQuebrado")) { return; }

  if (not dividirRecebimento(row, *caixasDefeito, quantCaixa)) { return qApp->rollbackTransaction(); }

  if (not qApp->endTransaction()) { return; }

  qApp->enqueueInformation("Operação realizada com sucesso!", this);
}

void InputDialogConfirmacao::on_pushButtonQuebradoEntrega_clicked() {
  // 1. quebrar a linha em 2
  // 2. a parte quebrada fica com status 'quebrado' no limbo
  // 3. a parte que veio prossegue para estoque
  // 4. verificar se precisa desfazer algum consumo caso a quant. nao seja suficiente

  const auto list = ui->tableLogistica->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhum item selecionado!", this); }

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

  const auto novoIdVendaProduto2 = qApp->reservarIdVendaProduto2();

  if (not novoIdVendaProduto2) { return; }

  const auto caixasDefeito = getCaixasDefeito(row);

  if (not caixasDefeito) { return; }

  // -------------------------------------------------------------------------

  if (not qApp->startTransaction("InputDialogConfirmacao::on_pushButtonQuebrado")) { return; }

  if (not dividirEntrega(row, choice, *caixasDefeito, obs, *novoIdVendaProduto2)) { return qApp->rollbackTransaction(); }

  if (not qApp->endTransaction()) { return; }

  qApp->enqueueInformation("Operação realizada com sucesso!", this);
}

bool InputDialogConfirmacao::criarConsumoQuebrado(const int idEstoque, const double caixasDefeito, const double quantCaixa) {
  QSqlQuery query;
  query.prepare("INSERT INTO estoque_has_consumo (idEstoque, status, local, quant, caixas) VALUES (:idEstoque, 'QUEBRADO', 'TEMP', :quant, :caixas)");
  query.bindValue(":idEstoque", idEstoque);
  query.bindValue(":quant", caixasDefeito * quantCaixa);
  query.bindValue(":caixas", caixasDefeito);

  if (not query.exec()) { return qApp->enqueueError(false, "Erro criando consumo quebrado: " + query.lastError().text(), this); }

  return true;
}

bool InputDialogConfirmacao::dividirRecebimento(const int row, const double caixasDefeito, const double quantCaixa) {
  // TODO: fazer no recebimento o mesmo fluxo da entrega (criar nova linha, etc)
  // TODO: ao dividir linha fazer prepend '(REPO. ENTREGA/RECEB.)' na observacao do produto
  // TODO: nao dividir linha do estoque, apenas criar um consumo 'quebrado' para inutilizar a parte que foi quebrada (a mesma coisa para faltando)

  // REFAC: 0finish this part

  const int idEstoque = modelEstoque.data(row, "idEstoque").toInt();
  const double caixas = modelEstoque.data(row, "caixas").toDouble();
  Q_UNUSED(caixas)
  Q_UNUSED(quantCaixa)

  if (not desfazerConsumo(idEstoque, caixasDefeito)) { return false; }
  if (not criarConsumoQuebrado(idEstoque, caixasDefeito, quantCaixa)) { return false; }

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

  return true;
}

bool InputDialogConfirmacao::dividirEntrega(const int row, const int choice, const double caixasDefeito, const QString obs, const int novoIdVendaProduto2) {
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

  if (not modelVendaProduto.select()) { return false; }

  // -------------------------------------------------------------------------

  if (not dividirVenda(modelVendaProduto, caixas, caixasDefeito, quantCaixa, novoIdVendaProduto2)) { return false; }

  // -------------------------------------------------------------------------

  choice == (QMessageBox::Yes) ? criarReposicaoCliente(modelVendaProduto, caixasDefeito, quantCaixa, obs, novoIdVendaProduto2) : gerarCreditoCliente(modelVendaProduto, caixasDefeito, quantCaixa);

  if (not modelVendaProduto.submitAll()) { return false; }

  // -------------------------------------------------------------------------

  if (not dividirVeiculo(row, caixas, caixasDefeito, quantCaixa, novoIdVendaProduto2)) { return false; }

  // -------------------------------------------------------------------------

  if (not dividirConsumo(caixas, caixasDefeito, quantCaixa, novoIdVendaProduto2, idVendaProduto2)) { return false; }

  // -------------------------------------------------------------------------

  if (not dividirCompra(caixas, caixasDefeito, quantCaixa, novoIdVendaProduto2, idVendaProduto2)) { return false; }

  // -------------------------------------------------------------------------

  return true;
}

bool InputDialogConfirmacao::gerarCreditoCliente(const SqlTableModel &modelVendaProduto, const double caixasDefeito, const double quantCaixa) {
  const QString idVenda = modelVendaProduto.data(0, "idVenda").toString();
  const double descUnitario = modelVendaProduto.data(0, "descUnitario").toDouble();

  const double credito = caixasDefeito * quantCaixa * descUnitario;

  qApp->enqueueInformation("Gerado crédito no valor de R$ " + QLocale(QLocale::Portuguese).toString(credito), this);

  SqlTableModel modelCliente;
  modelCliente.setTable("cliente");

  // TODO: since this is inside a transaction simplify using a UPDATE query: credito = credito + novoCredito
  QSqlQuery query;
  query.prepare("SELECT idCliente FROM venda WHERE idVenda = :idVenda");
  query.bindValue(":idVenda", idVenda);

  if (not query.exec() or not query.first()) { return qApp->enqueueError(false, "Erro buscando cliente: " + query.lastError().text(), this); }

  modelCliente.setFilter("idCliente = " + query.value("idCliente").toString());

  if (not modelCliente.select()) { return false; }

  const double creditoAntigo = modelCliente.data(0, "credito").toDouble();

  if (not modelCliente.setData(0, "credito", credito + creditoAntigo)) { return false; }

  if (not modelCliente.submitAll()) { return false; }

  return true;
}

bool InputDialogConfirmacao::criarReposicaoCliente(SqlTableModel &modelVendaProduto, const double caixasDefeito, const double quantCaixa, const QString obs, const int novoIdVendaProduto2) {
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

    if (not modelVendaProduto.setData(newRow, col, value)) { return false; }
  }

  if (not modelVendaProduto.setData(newRow, "idRelacionado", novoIdVendaProduto2)) { return false; }
  if (not modelVendaProduto.setData(newRow, "quant", caixasDefeito * quantCaixa)) { return false; }
  if (not modelVendaProduto.setData(newRow, "caixas", caixasDefeito)) { return false; }
  if (not modelVendaProduto.setData(newRow, "prcUnitario", 0)) { return false; }
  if (not modelVendaProduto.setData(newRow, "descUnitario", 0)) { return false; }
  if (not modelVendaProduto.setData(newRow, "parcial", 0)) { return false; }
  if (not modelVendaProduto.setData(newRow, "desconto", 0)) { return false; }
  if (not modelVendaProduto.setData(newRow, "parcialDesc", 0)) { return false; }
  if (not modelVendaProduto.setData(newRow, "descGlobal", 0)) { return false; }
  if (not modelVendaProduto.setData(newRow, "total", 0)) { return false; }
  if (not modelVendaProduto.setData(newRow, "status", "REPO. ENTREGA")) { return false; }
  if (not modelVendaProduto.setData(newRow, "reposicaoEntrega", true)) { return false; }

  return modelVendaProduto.setData(newRow, "obs", "(REPO. ENTREGA) " + obs);
}

bool InputDialogConfirmacao::desfazerConsumo(const int idEstoque, const double caixasDefeito) {
  // REFAC: pass this responsability to Estoque class
  // NOTE: verificar WidgetCompraConsumos::desfazerConsumo

  QSqlQuery query;
  query.prepare("SELECT restante FROM estoque WHERE idEstoque = :idEstoque");
  query.bindValue(":idEstoque", idEstoque);

  if (not query.exec() or not query.first()) { return qApp->enqueueError(false, "Erro buscando sobra estoque: " + query.lastError().text(), this); }

  double restante = query.value("restante").toDouble(); // TODO: divide this by quantCaixa
  qDebug() << "sobra: " << restante;
  qDebug() << "caixasDefeito: " << caixasDefeito;

  if (restante < 0) { // faltando pecas para consumo, desfazer os consumos com prazo maior
    QSqlQuery querySelect;
    querySelect.prepare(
        "SELECT CAST((`v`.`data` + INTERVAL `v`.`prazoEntrega` DAY) AS DATE) AS `prazoEntrega`, ehc.* FROM estoque_has_consumo ehc LEFT JOIN venda_has_produto2 vp2 ON ehc.idVendaProduto2 = "
        "vp2.idVendaProduto2 LEFT JOIN venda v ON vp2.idVenda = v.idVenda WHERE ehc.idEstoque = :idEstoque ORDER BY prazoEntrega DESC");
    querySelect.bindValue(":idEstoque", idEstoque);

    if (not querySelect.exec()) { return qApp->enqueueError(false, "Erro buscando consumo estoque: " + querySelect.lastError().text(), this); }

    QSqlQuery queryDelete;
    // TODO: 0se a parte não quebrada for suficiente nao desfazer consumos (tomar cuidado para usar o idEstoque do restante e nao do quebrado)
    queryDelete.prepare("DELETE FROM estoque_has_consumo WHERE idConsumo = :idConsumo");

    QSqlQuery queryVenda;
    // TODO: should set flag 'reposicao' (where is this flag unset?)
    queryVenda.prepare("UPDATE venda_has_produto2 SET status = 'REPO. RECEB.', dataPrevEnt = NULL WHERE idVendaProduto2 = :idVendaProduto2 AND status NOT IN ('CANCELADO', 'DEVOLVIDO')");

    while (querySelect.next()) {
      const int caixas = querySelect.value("caixas").toInt();

      queryDelete.bindValue(":idConsumo", querySelect.value("idConsumo"));

      if (not queryDelete.exec()) { return qApp->enqueueError(false, "Erro removendo consumo: " + queryDelete.lastError().text(), this); }

      queryVenda.bindValue(":idVendaProduto2", querySelect.value("idVendaProduto2"));

      if (not queryVenda.exec()) { return qApp->enqueueError(false, "Erro voltando produto para pendente: " + queryVenda.lastError().text(), this); }

      restante += caixas;
      if (restante >= 0) { break; }
    }
  }

  return true;
}

void InputDialogConfirmacao::on_pushButtonFoto_clicked() {
  const QString filePath = QFileDialog::getOpenFileName(this, "Imagens", QDir::currentPath(), "(*.jpg *.jpeg *.png *.tif *.bmp *.pdf)");

  if (filePath.isEmpty()) { return; }

  QFile *file = new QFile(filePath);

  if (not file->open(QFile::ReadOnly)) { return qApp->enqueueError("Erro lendo arquivo: " + file->errorString(), this); }

  QNetworkAccessManager *manager = new QNetworkAccessManager(this);

  const QString ip = qApp->getWebDavIp();
  const QString idVenda = modelVeiculo.data(0, "idVenda").toString();
  const QString idEvento = modelVeiculo.data(0, "idEvento").toString();

  QFileInfo info(*file);

  const QString extension = info.suffix();

  const QString url = "http://" + ip + "/webdav/FOTOS ENTREGAS/" + idVenda + " - " + idEvento + "." + extension;

  auto reply = manager->put(QNetworkRequest(QUrl(url)), file);

  ui->lineEditFoto->setText("Enviando...");

  connect(reply, &QNetworkReply::finished, [=] {
    ui->lineEditFoto->setText((reply->error() == QNetworkReply::NoError) ? url : "Erro enviando foto: " + reply->errorString());

    if (reply->error() == QNetworkReply::NoError) {
      ui->lineEditFoto->setText(url);
      ui->lineEditFoto->setStyleSheet("background-color: rgb(0, 255, 0); color: rgb(0, 0, 0);");

      for (int row = 0; row < modelVeiculo.rowCount(); ++row) {
        if (not modelVeiculo.setData(row, "fotoEntrega", url)) { return; }
      }

    } else {
      ui->lineEditFoto->setText("Erro enviando foto: " + reply->errorString());
      ui->lineEditFoto->setStyleSheet("background-color: rgb(255, 0, 0); color: rgb(0, 0, 0);");
    }
  });
}

std::optional<double> InputDialogConfirmacao::getCaixasDefeito(const int row) {
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

  if (not ok or qFuzzyIsNull(caixasDefeito)) { return {}; }

  return caixasDefeito;
}

bool InputDialogConfirmacao::dividirVenda(SqlTableModel &modelVendaProduto, const double caixas, const double caixasDefeito, const double quantCaixa, const int novoIdVendaProduto2) {
  const double caixasRestante = caixas - caixasDefeito;
  const double quantRestante = caixasRestante * quantCaixa;

  if (not modelVendaProduto.setData(0, "caixas", caixasRestante)) { return false; }
  if (not modelVendaProduto.setData(0, "quant", quantRestante)) { return false; }

  const double prcUnitario = modelVendaProduto.data(0, "prcUnitario").toDouble();
  const double descUnitario = modelVendaProduto.data(0, "descUnitario").toDouble();
  const double descGlobal = modelVendaProduto.data(0, "descGlobal").toDouble() / 100;

  if (not modelVendaProduto.setData(0, "parcial", quantRestante * prcUnitario)) { return false; }
  if (not modelVendaProduto.setData(0, "parcialDesc", quantRestante * descUnitario)) { return false; }
  if (not modelVendaProduto.setData(0, "total", quantRestante * descUnitario * (1 - descGlobal))) { return false; }

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

    if (not modelVendaProduto.setData(rowQuebrado2, col, value)) { return false; }
  }

  const double quantDefeito = caixasDefeito * quantCaixa;

  if (not modelVendaProduto.setData(rowQuebrado2, "idVendaProduto2", novoIdVendaProduto2)) { return false; }
  if (not modelVendaProduto.setData(rowQuebrado2, "idRelacionado", modelVendaProduto.data(0, "idVendaProduto2"))) { return false; }
  if (not modelVendaProduto.setData(rowQuebrado2, "caixas", caixasDefeito)) { return false; }
  if (not modelVendaProduto.setData(rowQuebrado2, "quant", quantDefeito)) { return false; }
  if (not modelVendaProduto.setData(rowQuebrado2, "status", "QUEBRADO")) { return false; }

  if (not modelVendaProduto.setData(rowQuebrado2, "parcial", quantDefeito * prcUnitario)) { return false; }
  if (not modelVendaProduto.setData(rowQuebrado2, "parcialDesc", quantDefeito * descUnitario)) { return false; }
  if (not modelVendaProduto.setData(rowQuebrado2, "total", quantDefeito * descUnitario * (1 - descGlobal))) { return false; }

  return true;
}

bool InputDialogConfirmacao::dividirVeiculo(const int row, const double caixas, const double caixasDefeito, const double quantCaixa, const int novoIdVendaProduto2) {
  // diminuir quantidade da linha selecionada

  // recalcular kg? (posso usar proporcao para nao precisar puxar kgcx)
  if (not modelVeiculo.setData(row, "caixas", caixas - caixasDefeito)) { return false; }
  if (not modelVeiculo.setData(row, "quant", (caixas - caixasDefeito) * quantCaixa)) { return false; }

  // copiar linha com quantDefeito

  const int rowQuebrado = modelVeiculo.insertRowAtEnd();

  for (int col = 0; col < modelVeiculo.columnCount(); ++col) {
    if (modelVeiculo.fieldIndex("id") == col) { continue; }
    if (modelVeiculo.fieldIndex("created") == col) { continue; }
    if (modelVeiculo.fieldIndex("lastUpdated") == col) { continue; }

    const QVariant value = modelVeiculo.data(row, col);

    if (not modelVeiculo.setData(rowQuebrado, col, value)) { return false; }
  }

  // recalcular kg? (posso usar proporcao para nao precisar puxar kgcx)
  if (not modelVeiculo.setData(rowQuebrado, "idVendaProduto2", novoIdVendaProduto2)) { return false; }
  if (not modelVeiculo.setData(rowQuebrado, "caixas", caixasDefeito)) { return false; }
  if (not modelVeiculo.setData(rowQuebrado, "quant", caixasDefeito * quantCaixa)) { return false; }
  if (not modelVeiculo.setData(rowQuebrado, "status", "QUEBRADO")) { return false; }

  if (not modelVeiculo.submitAll()) { return false; }

  return true;
}

bool InputDialogConfirmacao::dividirConsumo(const double caixas, const double caixasDefeito, const double quantCaixa, const int novoIdVendaProduto2, const QString idVendaProduto2) {
  SqlTableModel modelConsumo;
  modelConsumo.setTable("estoque_has_consumo");

  modelConsumo.setFilter("idVendaProduto2 = " + idVendaProduto2);

  if (not modelConsumo.select()) { return false; }
  // TODO: verificar se rowCount > 0?

  const double caixasRestante = caixas - caixasDefeito;
  const double quantRestante = caixasRestante * quantCaixa;
  const double valorConsumo = modelConsumo.data(0, "valor").toDouble();

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

  if (not modelConsumo.setData(0, "quant", quantRestante)) { return false; }
  if (not modelConsumo.setData(0, "caixas", caixasRestante)) { return false; }
  if (not modelConsumo.setData(0, "valor", valorConsumo * proporcao)) { return false; }
  if (not modelConsumo.setData(0, "desconto", desconto * proporcao)) { return false; }
  if (not modelConsumo.setData(0, "vBC", vBC * proporcao)) { return false; }
  if (not modelConsumo.setData(0, "vICMS", vICMS * proporcao)) { return false; }
  if (not modelConsumo.setData(0, "vBCST", vBCST * proporcao)) { return false; }
  if (not modelConsumo.setData(0, "vICMSST", vICMSST * proporcao)) { return false; }
  if (not modelConsumo.setData(0, "vBCPIS", vBCPIS * proporcao)) { return false; }
  if (not modelConsumo.setData(0, "vPIS", vPIS * proporcao)) { return false; }
  if (not modelConsumo.setData(0, "vBCCOFINS", vBCCOFINS * proporcao)) { return false; }
  if (not modelConsumo.setData(0, "vCOFINS", vCOFINS * proporcao)) { return false; }

  // copiar linha
  const int newRow = modelConsumo.insertRowAtEnd();

  for (int column = 0, columnCount = modelConsumo.columnCount(); column < columnCount; ++column) {
    if (column == modelConsumo.fieldIndex("idConsumo")) { continue; }
    if (column == modelConsumo.fieldIndex("idVendaProduto2")) { continue; }
    if (column == modelConsumo.fieldIndex("created")) { continue; }
    if (column == modelConsumo.fieldIndex("lastUpdated")) { continue; }

    const QVariant value = modelConsumo.data(0, column);

    if (not modelConsumo.setData(newRow, column, value)) { return false; }
  }

  const double proporcaoNovo = caixasDefeito / caixas;

  if (not modelConsumo.setData(newRow, "idVendaProduto2", novoIdVendaProduto2)) { return false; }
  if (not modelConsumo.setData(newRow, "status", "QUEBRADO")) { return false; }
  if (not modelConsumo.setData(newRow, "quant", caixasDefeito * quantCaixa)) { return false; }
  if (not modelConsumo.setData(newRow, "caixas", caixasDefeito)) { return false; }
  if (not modelConsumo.setData(newRow, "valor", valorConsumo * proporcaoNovo)) { return false; }
  if (not modelConsumo.setData(newRow, "desconto", desconto * proporcaoNovo)) { return false; }
  if (not modelConsumo.setData(newRow, "vBC", vBC * proporcaoNovo)) { return false; }
  if (not modelConsumo.setData(newRow, "vICMS", vICMS * proporcaoNovo)) { return false; }
  if (not modelConsumo.setData(newRow, "vBCST", vBCST * proporcaoNovo)) { return false; }
  if (not modelConsumo.setData(newRow, "vICMSST", vICMSST * proporcaoNovo)) { return false; }
  if (not modelConsumo.setData(newRow, "vBCPIS", vBCPIS * proporcaoNovo)) { return false; }
  if (not modelConsumo.setData(newRow, "vPIS", vPIS * proporcaoNovo)) { return false; }
  if (not modelConsumo.setData(newRow, "vBCCOFINS", vBCCOFINS * proporcaoNovo)) { return false; }
  if (not modelConsumo.setData(newRow, "vCOFINS", vCOFINS * proporcaoNovo)) { return false; }

  if (not modelConsumo.submitAll()) { return false; }

  return true;
}

bool InputDialogConfirmacao::dividirCompra(const double caixas, const double caixasDefeito, const double quantCaixa, const int novoIdVendaProduto2, const QString idVendaProduto2) {
  SqlTableModel modelCompra;
  modelCompra.setTable("pedido_fornecedor_has_produto2");

  modelCompra.setFilter("idVendaProduto2 = " + idVendaProduto2);

  if (not modelCompra.select()) { return false; }
  // TODO: verificar se rowCount > 0?

  const double prcUnitario = modelCompra.data(0, "prcUnitario").toDouble();
  const double caixasRestante = caixas - caixasDefeito;
  const double quantRestante = caixasRestante * quantCaixa;

  if (not modelCompra.setData(0, "quant", quantRestante)) { return false; }
  if (not modelCompra.setData(0, "caixas", caixasRestante)) { return false; }
  if (not modelCompra.setData(0, "preco", quantRestante * prcUnitario)) { return false; }

  // copiar linha
  const int newRow = modelCompra.insertRowAtEnd();

  for (int column = 0, columnCount = modelCompra.columnCount(); column < columnCount; ++column) {
    if (column == modelCompra.fieldIndex("idPedido2")) { continue; }
    if (column == modelCompra.fieldIndex("created")) { continue; }
    if (column == modelCompra.fieldIndex("lastUpdated")) { continue; }

    const QVariant value = modelCompra.data(0, column);

    if (not modelCompra.setData(newRow, column, value)) { return false; }
  }

  if (not modelCompra.setData(newRow, "idRelacionado", modelCompra.data(0, "idPedido2"))) { return false; }
  if (not modelCompra.setData(newRow, "idVendaProduto2", novoIdVendaProduto2)) { return false; }
  if (not modelCompra.setData(newRow, "quant", caixasDefeito * quantCaixa)) { return false; }
  if (not modelCompra.setData(newRow, "caixas", caixasDefeito)) { return false; }
  if (not modelCompra.setData(newRow, "preco", caixasDefeito * quantCaixa * prcUnitario)) { return false; }

  if (not modelCompra.submitAll()) { return false; }

  return true;
}

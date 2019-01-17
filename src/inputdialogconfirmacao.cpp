#include <QDebug>
#include <QInputDialog>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>

#include "application.h"
#include "inputdialogconfirmacao.h"
#include "orcamento.h"
#include "sortfilterproxymodel.h"
#include "ui_inputdialogconfirmacao.h"

InputDialogConfirmacao::InputDialogConfirmacao(const Tipo tipo, QWidget *parent) : QDialog(parent), tipo(tipo), ui(new Ui::InputDialogConfirmacao) {
  ui->setupUi(this);

  connect(ui->dateEditEvento, &QDateTimeEdit::dateChanged, this, &InputDialogConfirmacao::on_dateEditEvento_dateChanged);
  connect(ui->pushButtonFaltando, &QPushButton::clicked, this, &InputDialogConfirmacao::on_pushButtonFaltando_clicked);
  connect(ui->pushButtonQuebrado, &QPushButton::clicked, this, &InputDialogConfirmacao::on_pushButtonQuebrado_clicked);
  connect(ui->pushButtonSalvar, &QPushButton::clicked, this, &InputDialogConfirmacao::on_pushButtonSalvar_clicked);

  setWindowFlags(Qt::Window);

  setupTables();

  ui->dateEditEvento->setDateTime(QDateTime::currentDateTime());
  ui->dateEditProximo->setDateTime(QDateTime::currentDateTime());

  if (tipo == Tipo::Recebimento) {
    ui->labelProximoEvento->hide();
    ui->dateEditProximo->hide();

    ui->labelEvento->setText("Data do recebimento:");

    ui->labelEntregou->hide();
    ui->lineEditEntregou->hide();
  }

  if (tipo == Tipo::Entrega) {
    ui->labelProximoEvento->hide();
    ui->dateEditProximo->hide();

    ui->labelEvento->setText("Data entrega:");
  }

  if (tipo == Tipo::Representacao) {
    ui->labelAviso->hide();

    ui->labelProximoEvento->hide();
    ui->dateEditProximo->hide();

    ui->tableLogistica->hide();

    ui->labelEntregou->hide();
    ui->lineEditEntregou->hide();

    ui->frameQuebrado->hide();

    adjustSize();
  }

  show();
}

InputDialogConfirmacao::~InputDialogConfirmacao() { delete ui; }

QDateTime InputDialogConfirmacao::getDateTime() const { return ui->dateEditEvento->dateTime(); }

QDateTime InputDialogConfirmacao::getNextDateTime() const { return ui->dateEditProximo->dateTime(); }

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
  if ((tipo == Tipo::Recebimento or tipo == Tipo::Entrega) and ui->lineEditRecebeu->text().isEmpty()) { return qApp->enqueueError("Faltou preencher quem recebeu!", this); }

  if (tipo == Tipo::Entrega and ui->lineEditEntregou->text().isEmpty()) { return qApp->enqueueError("Faltou preencher quem entregou!", this); }

  if (tipo != Tipo::Representacao) {
    if (not qApp->startTransaction()) { return; }

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

    ui->tableLogistica->setModel(new SortFilterProxyModel(&modelEstoque, this));

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
    modelVeiculo.setHeaderData("unCaixa", "Un./Cx.");
    modelVeiculo.setHeaderData("codComercial", "Cód. Com.");
    modelVeiculo.setHeaderData("formComercial", "Form. Com.");

    ui->tableLogistica->setModel(new SortFilterProxyModel(&modelVeiculo, this));

    ui->tableLogistica->hideColumn("id");
    ui->tableLogistica->hideColumn("data");
    ui->tableLogistica->hideColumn("idEvento");
    ui->tableLogistica->hideColumn("idVeiculo");
    ui->tableLogistica->hideColumn("idEstoque");
    ui->tableLogistica->hideColumn("idVendaProduto");
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

  return true;
}

bool InputDialogConfirmacao::setFilterRecebe(const QStringList &ids) { // recebimento
  if (ids.isEmpty()) { return qApp->enqueueError(false, "IdsCompra vazio!", this); }

  const QString filter = "idEstoque = " + ids.join(" OR idEstoque = ");

  modelEstoque.setFilter(filter);

  if (not modelEstoque.select()) { return false; }

  setWindowTitle("Estoque: " + ids.join(", "));

  ui->pushButtonQuebrado->setDisabled(true); // TODO: remove this after it's fixed

  return true;
}

void InputDialogConfirmacao::on_pushButtonQuebrado_clicked() {
  // 1. quebrar a linha em 2
  // 2. a parte quebrada fica com status 'quebrado' no limbo
  // 3. a parte que veio prossegue para estoque
  // 4. verificar se precisa desfazer algum consumo caso a quant. nao seja suficiente

  const auto list = ui->tableLogistica->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhum item selecionado!", this); }

  const int row = list.first().row();

  // -------------------------------------------------------------------------

  QString produto;
  double unCaixa = 0;
  double caixas = 0;

  if (tipo == Tipo::Recebimento) {
    produto = modelEstoque.data(row, "descricao").toString();
    caixas = modelEstoque.data(row, "caixas").toDouble();

    QSqlQuery query;
    query.prepare("SELECT UPPER(un) AS un, m2cx, pccx FROM produto WHERE idProduto = :idProduto");
    query.bindValue(":idProduto", modelEstoque.data(row, "idProduto"));

    if (not query.exec() or not query.first()) { return qApp->enqueueError("Erro buscando dados do produto: " + query.lastError().text(), this); }

    const QString un = query.value("un").toString();
    const double m2cx = query.value("m2cx").toDouble();
    const double pccx = query.value("pccx").toDouble();

    unCaixa = (un == "M2" or un == "M²" or un == "ML" ? m2cx : pccx);
  }

  int choice = -1;

  if (tipo == Tipo::Entrega) {
    produto = modelVeiculo.data(row, "produto").toString();
    unCaixa = modelVeiculo.data(row, "unCaixa").toDouble(); // *
    caixas = modelVeiculo.data(row, "caixas").toDouble();

    QMessageBox msgBox(QMessageBox::Question, "Atenção!", "Criar reposição ou gerar crédito?", QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, this);
    msgBox.setButtonText(QMessageBox::Yes, "Criar reposição");
    msgBox.setButtonText(QMessageBox::No, "Gerar crédito");
    msgBox.setButtonText(QMessageBox::Cancel, "Cancelar");

    choice = msgBox.exec();
  }

  bool ok;
  // TODO: verify if this needs to be changed to use step after &ok
  const double caixasDefeito = QInputDialog::getDouble(this, produto, "Caixas quebradas: ", caixas, 0, caixas, 1, &ok);

  if (not ok or qFuzzyIsNull(caixasDefeito)) { return; }

  if (not qApp->startTransaction()) { return; }

  if (not processarQuebra(row, choice, caixasDefeito, unCaixa)) { return qApp->rollbackTransaction(); }

  if (not qApp->endTransaction()) { return; }

  qApp->enqueueInformation("Operação realizada com sucesso!", this);
}

bool InputDialogConfirmacao::processarQuebra(const int row, const int choice, const double caixasDefeito, const double unCaixa) {
  // TODO: ao quebrar linha fazer prepend '(REPO. ENTREGA/RECEB.)' na observacao do produto
  // TODO: fazer no recebimento o mesmo fluxo da entrega (criar nova linha, etc)
  if (tipo == Tipo::Recebimento and not quebrarRecebimento(row, caixasDefeito, unCaixa)) { return false; }
  if (tipo == Tipo::Entrega and not quebrarEntrega(row, choice, caixasDefeito, unCaixa)) { return false; }

  return true;
}

bool InputDialogConfirmacao::quebrarRecebimento(const int row, const double caixasDefeito, const double unCaixa) {
  // REFAC: 0finish this part

  // model is estoque
  // perguntar quant. quebrada - QInputDialog

  //  const QString produto = model.data(row, "descricao").toString();
  const int idEstoque = modelEstoque.data(row, "idEstoque").toInt();
  const int caixas = modelEstoque.data(row, "caixas").toInt();

  //  QSqlQuery query;
  //  query.prepare("SELECT UPPER(un) AS un, m2cx, pccx FROM produto WHERE idProduto = :idProduto");
  //  query.bindValue(":idProduto", model.data(row, "idProduto"));

  //  if (not query.exec() or not query.first()) { return qApp->enqueueError(false, "Erro buscando dados do produto: " + query.lastError().text()); }

  //  const QString un = query.value("un").toString();
  //  const double m2cx = query.value("m2cx").toDouble();
  //  const double pccx = query.value("pccx").toDouble();

  //  unCaixa = un == "M2" or un == "M²" or un == "ML" ? m2cx : pccx;

  //  qDebug() << "unCaixa: " << unCaixa;

  //  bool ok = false;
  //  // TODO: 0put this outside transaction
  //  caixasDefeito = QInputDialog::getInt(this, produto, "Caixas quebradas/faltando: ", caixas, 0, caixas, 1, &ok);

  //  if (not ok or caixasDefeito == 0) { return false; }

  // TODO: inline this
  if (not quebrarLinhaRecebimento(row, caixas, caixasDefeito, unCaixa)) { return false; }
  //  if (not criarConsumo(row)) { return false; }
  if (not desfazerConsumo(idEstoque, caixasDefeito)) { return false; }

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

bool InputDialogConfirmacao::quebrarEntrega(const int row, const int choice, const double caixasDefeito, const double unCaixa) {
  // NOTE: na tabela veiculo_has_produto é separado a linha em 2:
  // -linha original mantem a quant. entregue
  // -linha nova mostra a quant. quebrada

  // na tabela venda_has_produto é separado a linha em:
  // -linha original mantem a quant. entregue
  // -linha nova mostar a quant. quebrada
  // -caso tenha reposicao é criada uma terceira linha 'repo.' (mesma quant. da 'quebrada')

  const double caixas = modelVeiculo.data(row, "caixas").toDouble();

  // diminuir quantidade da linha selecionada

  // recalcular kg? (posso usar proporcao para nao precisar puxar kgcx)
  if (not modelVeiculo.setData(row, "caixas", caixas - caixasDefeito)) { return false; }
  if (not modelVeiculo.setData(row, "quant", (caixas - caixasDefeito) * unCaixa)) { return false; }

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
  if (not modelVeiculo.setData(rowQuebrado, "caixas", caixasDefeito)) { return false; }
  if (not modelVeiculo.setData(rowQuebrado, "quant", caixasDefeito * unCaixa)) { return false; }
  if (not modelVeiculo.setData(rowQuebrado, "status", "QUEBRADO")) { return false; }

  //  // perguntar se gerar credito ou reposicao

  if (choice == QMessageBox::Cancel) { return false; }

  // TODO: marcar idRelacionado

  SqlRelationalTableModel modelVendaProduto;
  modelVendaProduto.setTable("venda_has_produto");

  modelVendaProduto.setFilter("idVendaProduto = " + modelVeiculo.data(row, "idVendaProduto").toString());

  if (not modelVendaProduto.select()) { return false; }

  if (not modelVendaProduto.setData(0, "caixas", caixas - caixasDefeito)) { return false; }
  if (not modelVendaProduto.setData(0, "quant", (caixas - caixasDefeito) * unCaixa)) { return false; }

  const int rowQuebrado2 = modelVendaProduto.insertRowAtEnd();
  // NOTE: *quebralinha venda_produto/pedido_fornecedor

  for (int col = 0; col < modelVendaProduto.columnCount(); ++col) {
    if (modelVendaProduto.fieldIndex("idVendaProduto") == col) { continue; }
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

    if (not modelVendaProduto.setData(rowQuebrado2, col, value)) { return false; }
  }

  if (not modelVendaProduto.setData(rowQuebrado2, "caixas", caixasDefeito)) { return false; }
  if (not modelVendaProduto.setData(rowQuebrado2, "quant", caixasDefeito * unCaixa)) { return false; }
  if (not modelVendaProduto.setData(rowQuebrado2, "status", "QUEBRADO")) { return false; }

  // -------------------------------------------------------------------------

  choice == QMessageBox::Yes ? criarReposicaoCliente(modelVendaProduto, caixasDefeito, unCaixa) : gerarCreditoCliente(modelVendaProduto, caixasDefeito, unCaixa);

  return modelVendaProduto.submitAll();
}

bool InputDialogConfirmacao::gerarCreditoCliente(const SqlRelationalTableModel &modelVendaProduto, const double caixasDefeito, const double unCaixa) {
  const QString idVenda = modelVendaProduto.data(0, "idVenda").toString();
  const double descUnitario = modelVendaProduto.data(0, "descUnitario").toDouble();

  const double credito = caixasDefeito * unCaixa * descUnitario;

  qApp->enqueueInformation("Gerado crédito no valor de R$ " + QLocale(QLocale::Portuguese).toString(credito), this);

  SqlRelationalTableModel modelCliente;
  modelCliente.setTable("cliente");

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

bool InputDialogConfirmacao::criarReposicaoCliente(SqlRelationalTableModel &modelVendaProduto, const double caixasDefeito, const double unCaixa) {
  const int newRow = modelVendaProduto.insertRowAtEnd();
  // NOTE: *quebralinha venda_produto/pedido_fornecedor

  // copiar linha com quantidade quebrada
  for (int col = 0; col < modelVendaProduto.columnCount(); ++col) {
    if (modelVendaProduto.fieldIndex("idVendaProduto") == col) { continue; }
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

  if (not modelVendaProduto.setData(newRow, "quant", caixasDefeito * unCaixa)) { return false; }
  if (not modelVendaProduto.setData(newRow, "caixas", caixasDefeito)) { return false; }
  if (not modelVendaProduto.setData(newRow, "parcial", 0)) { return false; }
  if (not modelVendaProduto.setData(newRow, "desconto", 0)) { return false; }
  if (not modelVendaProduto.setData(newRow, "parcialDesc", 0)) { return false; }
  if (not modelVendaProduto.setData(newRow, "descGlobal", 0)) { return false; }
  if (not modelVendaProduto.setData(newRow, "total", 0)) { return false; }
  if (not modelVendaProduto.setData(newRow, "status", "REPO. ENTREGA")) { return false; }
  if (not modelVendaProduto.setData(newRow, "reposicaoEntrega", true)) { return false; }

  // REFAC: this blocks the transaction
  const QString obs = QInputDialog::getText(this, "Observacao", "Observacao: ");

  return modelVendaProduto.setData(newRow, "obs", "(REPO. ENTREGA) " + obs);
}

bool InputDialogConfirmacao::quebrarLinhaRecebimento(const int row, const int caixas, const double caixasDefeito, const double unCaixa) {
  // TODO: 5ao marcar caixas quebradas e só houver uma nao dividir em duas linhas (para nao ficar linha zerado)
  // diminuir quant. da linha selecionada

  if (not modelEstoque.setData(row, "caixas", caixas - caixasDefeito)) { return false; }
  if (not modelEstoque.setData(row, "quant", (caixas - caixasDefeito) * unCaixa)) { return false; }

  // copiar linha com defeito

  const int rowQuebrado = modelEstoque.insertRowAtEnd();

  for (int col = 0; col < modelEstoque.columnCount(); ++col) {
    if (modelEstoque.fieldIndex("idEstoque") == col) { continue; }
    if (modelEstoque.fieldIndex("created") == col) { continue; }
    if (modelEstoque.fieldIndex("lastUpdated") == col) { continue; }

    const QVariant value = modelEstoque.data(row, col);

    if (not modelEstoque.setData(rowQuebrado, col, value)) { return false; }
  }

  const QString obs = "Estoque: " + modelEstoque.data(row, "idEstoque").toString() + " - " + QInputDialog::getText(this, "Observacao", "Observacao: ");
  qDebug() << "obs: " << obs;

  if (not modelEstoque.setData(rowQuebrado, "observacao", obs)) { return false; }
  if (not modelEstoque.setData(rowQuebrado, "caixas", caixasDefeito)) { return false; }
  if (not modelEstoque.setData(rowQuebrado, "quant", caixasDefeito * unCaixa)) { return false; }
  if (not modelEstoque.setData(rowQuebrado, "status", "QUEBRADO")) { return false; }
  // TODO: recalcular proporcional dos valores

  if (not modelEstoque.submitAll()) { return false; }

  return true;
}

bool InputDialogConfirmacao::desfazerConsumo(const int idEstoque, const double caixasDefeito) {
  // REFAC: pass this responsability to Estoque class
  // NOTE: verificar WidgetCompraOC::desfazerConsumo

  QSqlQuery query;
  query.prepare("SELECT COALESCE(e.caixas - SUM(ehc.caixas), 0) AS sobra FROM estoque_has_consumo ehc LEFT JOIN estoque e ON ehc.idEstoque = e.idEstoque WHERE ehc.idEstoque = :idEstoque");
  query.bindValue(":idEstoque", idEstoque);

  if (not query.exec() or not query.first()) { return qApp->enqueueError(false, "Erro buscando sobra estoque: " + query.lastError().text(), this); }

  // REFAC: why this int is stored in a double?
  double sobra = query.value("sobra").toInt();
  qDebug() << "sobra: " << sobra;
  qDebug() << "caixasDefeito: " << caixasDefeito;

  if (sobra < 0) {
    // faltando pecas para consumo, desfazer os consumos com prazo maior

    QSqlQuery querySelect;
    querySelect.prepare(
        "SELECT CAST((`v`.`data` + INTERVAL `v`.`prazoEntrega` DAY) AS DATE) AS `prazoEntrega`, ehc.* FROM estoque_has_consumo ehc LEFT JOIN venda_has_produto vp ON ehc.idVendaProduto = "
        "vp.idVendaProduto LEFT JOIN venda v ON vp.idVenda = v.idVenda WHERE ehc.idEstoque = :idEstoque ORDER BY prazoEntrega DESC");
    querySelect.bindValue(":idEstoque", idEstoque);

    if (not querySelect.exec()) { return qApp->enqueueError(false, "Erro buscando consumo estoque: " + querySelect.lastError().text(), this); }

    QSqlQuery queryDelete;
    // TODO: 0se a parte não quebrada for suficiente nao desfazer consumos (tomar cuidado para usar o idEstoque do restante e nao do quebrado)
    queryDelete.prepare("DELETE FROM estoque_has_consumo WHERE idConsumo = :idConsumo");

    QSqlQuery queryVenda;
    // TODO: should set flag 'reposicao'
    queryVenda.prepare("UPDATE venda_has_produto SET status = 'REPO. RECEB.', dataPrevEnt = NULL WHERE idVendaProduto = :idVendaProduto AND status NOT IN ('CANCELADO', 'DEVOLVIDO')");

    while (querySelect.next()) {
      const int caixas = querySelect.value("caixas").toInt();

      queryDelete.bindValue(":idConsumo", querySelect.value("idConsumo"));

      if (not queryDelete.exec()) { return qApp->enqueueError(false, "Erro removendo consumo: " + queryDelete.lastError().text(), this); }

      queryVenda.bindValue(":idVendaProduto", querySelect.value("idVendaProduto"));

      if (not queryVenda.exec()) { return qApp->enqueueError(false, "Erro voltando produto para pendente: " + queryVenda.lastError().text(), this); }

      sobra += caixas;
      if (sobra >= 0) { break; }
    }
  }

  return true;
}

void InputDialogConfirmacao::on_pushButtonFaltando_clicked() {
  // 1. quebrar a linha em 2 (tanto no veiculo_has_produto quanto no venda_has_produto) (talvez no pf tambem)?
  // 2. a parte que está faltando mantém em recebimento
  // 3. a parte que veio prossegue para estoque
  // 4. verificar se precisa desfazer algum consumo caso a quant. nao seja suficiente

  // recebimento

  // entrega
  // 1. confirmando apenas a linha já selecionada (visivel na tela) a linha duplicada irá se manter na entrega
}

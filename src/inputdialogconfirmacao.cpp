#include <QDebug>
#include <QInputDialog>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>

#include "inputdialogconfirmacao.h"
#include "orcamento.h"
#include "ui_inputdialogconfirmacao.h"

InputDialogConfirmacao::InputDialogConfirmacao(const Tipo &tipo, QWidget *parent) : QDialog(parent), tipo(tipo), ui(new Ui::InputDialogConfirmacao) {
  ui->setupUi(this);

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

// REFAC: 5should be QDate?
QDateTime InputDialogConfirmacao::getDate() const { return ui->dateEditEvento->dateTime(); }

QDateTime InputDialogConfirmacao::getNextDate() const { return ui->dateEditProximo->dateTime(); }

QString InputDialogConfirmacao::getRecebeu() const { return ui->lineEditRecebeu->text(); }

QString InputDialogConfirmacao::getEntregou() const { return ui->lineEditEntregou->text(); }

bool InputDialogConfirmacao::cadastrar() {
  if (not model.submitAll()) {
    emit errorSignal("Erro salvando dados na tabela: " + model.lastError().text());
    return false;
  }

  if (not modelVenda.submitAll()) {
    emit errorSignal("Erro salvando produto venda: " + modelVenda.lastError().text());
    return false;
  }

  if (not modelCliente.submitAll()) {
    emit errorSignal("Erro salvando crédito: " + modelCliente.lastError().text());
    return false;
  }

  return true;
}

void InputDialogConfirmacao::on_pushButtonSalvar_clicked() {
  if ((tipo == Tipo::Recebimento or tipo == Tipo::Entrega) and ui->lineEditRecebeu->text().isEmpty()) {
    QMessageBox::critical(this, "Erro!", "Faltou preencher quem recebeu!");
    return;
  }

  if (tipo == Tipo::Entrega and ui->lineEditEntregou->text().isEmpty()) {
    QMessageBox::critical(this, "Erro!", "Faltou preencher quem entregou!");
    return;
  }

  emit transactionStarted();

  if (tipo != Tipo::Representacao) {
    QSqlQuery("SET SESSION TRANSACTION ISOLATION LEVEL SERIALIZABLE").exec();
    QSqlQuery("START TRANSACTION").exec();

    if (not cadastrar()) {
      QSqlQuery("ROLLBACK").exec();
      emit transactionEnded();
      return;
    }

    QSqlQuery("COMMIT").exec();

    emit transactionEnded();
  }

  QDialog::accept();
  close();
}

void InputDialogConfirmacao::on_dateEditEvento_dateChanged(const QDate &date) {
  if (ui->dateEditProximo->date() < date) ui->dateEditProximo->setDate(date);
}

void InputDialogConfirmacao::setupTables() {
  model.setEditStrategy(QSqlTableModel::OnManualSubmit);

  if (tipo == Tipo::Recebimento) {
    model.setTable("estoque");
    model.setHeaderData("status", "Status");
    model.setHeaderData("local", "Local");
    model.setHeaderData("fornecedor", "Fornecedor");
    model.setHeaderData("descricao", "Produto");
    model.setHeaderData("quant", "Quant.");
    model.setHeaderData("un", "Un.");
    model.setHeaderData("caixas", "Cx.");
    model.setHeaderData("codComercial", "Cód. Com.");
    model.setHeaderData("lote", "Lote");
    model.setHeaderData("bloco", "Bloco");
  }

  if (tipo == Tipo::Entrega) {
    model.setTable("veiculo_has_produto");
    model.setHeaderData("idVenda", "Venda");
    model.setHeaderData("status", "Status");
    model.setHeaderData("fornecedor", "Fornecedor");
    model.setHeaderData("produto", "Produto");
    model.setHeaderData("obs", "Obs.");
    model.setHeaderData("caixas", "Cx.");
    model.setHeaderData("kg", "Kg.");
    model.setHeaderData("quant", "Quant.");
    model.setHeaderData("un", "Un.");
    model.setHeaderData("unCaixa", "Un./Cx.");
    model.setHeaderData("codComercial", "Cód. Com.");
    model.setHeaderData("formComercial", "Form. Com.");
  }

  if (tipo != Tipo::Representacao) {
    model.setFilter("0");

    if (not model.select()) {
      QMessageBox::critical(this, "Erro!", "Erro lendo tabela pedido_fornecedor_has_produto: " + model.lastError().text());
      return;
    }

    ui->tableLogistica->setModel(&model);
  }

  if (tipo == Tipo::Recebimento) {
    ui->tableLogistica->hideColumn("idEstoque");
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

bool InputDialogConfirmacao::setFilter(const QString &id, const QString &idEvento) { // entrega
  if (id.isEmpty()) {
    model.setFilter("0");
    QMessageBox::critical(this, "Erro!", "IdsCompra vazio!");
    return false;
  }

  const QString filter = "idVenda = '" + id + "' AND idEvento = " + idEvento;

  model.setFilter(filter);

  if (not model.select()) {
    QMessageBox::critical(this, "Erro!", "Erro lendo tabela pedido_fornecedor_has_produto: " + model.lastError().text());
    return false;
  }

  ui->tableLogistica->resizeColumnsToContents();

  ui->dateEditEvento->setDateTime(model.data(0, "data").toDateTime());

  return true;
}

bool InputDialogConfirmacao::setFilter(const QStringList &ids) { // recebimento
  if (ids.isEmpty()) {
    model.setFilter("0");
    QMessageBox::critical(this, "Erro!", "IdsCompra vazio!");
    return false;
  }

  const QString filter = "idEstoque = " + ids.join(" OR idEstoque = ");

  model.setFilter(filter);

  if (not model.select()) {
    QMessageBox::critical(this, "Erro!", "Erro lendo tabela pedido_fornecedor_has_produto: " + model.lastError().text());
    return false;
  }

  ui->tableLogistica->resizeColumnsToContents();

  setWindowTitle("Estoque: " + ids.join(", "));

  return true;
}

void InputDialogConfirmacao::on_pushButtonQuebradoFaltando_clicked() {
  const auto list = ui->tableLogistica->selectionModel()->selectedRows();

  if (list.isEmpty()) {
    QMessageBox::critical(this, "Erro!", "Nenhum item selecionado!");
    return;
  }

  const int row = list.first().row();

  emit transactionStarted();

  QSqlQuery("SET SESSION TRANSACTION ISOLATION LEVEL SERIALIZABLE").exec();
  QSqlQuery("START TRANSACTION").exec();

  if (not processarQuebra(row)) {
    QSqlQuery("ROLLBACK").exec();
    emit transactionEnded();
    return;
  }

  QSqlQuery("COMMIT").exec();

  emit transactionEnded();

  QMessageBox::information(this, "Aviso!", "Operação realizada com sucesso!");
}

bool InputDialogConfirmacao::processarQuebra(const int row) {
  // TODO: ao quebrar linha fazer prepend '(REPO. ENTREGA/RECEB.)' na observacao do produto
  if (tipo == Tipo::Recebimento and not quebraRecebimento(row)) return false;
  if (tipo == Tipo::Entrega and not quebraEntrega(row)) return false;

  return true;
}

bool InputDialogConfirmacao::quebraRecebimento(const int row) {
  // TODO: 0finish this part

  // model is estoque
  // perguntar quant. quebrada - QInputDialog

  const QString produto = model.data(row, "descricao").toString();
  const int idEstoque = model.data(row, "idEstoque").toInt();
  const int caixas = model.data(row, "caixas").toInt();

  QSqlQuery query;
  query.prepare("SELECT UPPER(un) AS un, m2cx, pccx FROM produto WHERE idProduto = :idProduto");
  query.bindValue(":idProduto", model.data(row, "idProduto"));

  if (not query.exec() or not query.first()) {
    emit errorSignal("Erro buscando dados do produto: " + query.lastError().text());
    return false;
  }

  const QString un = query.value("un").toString();
  const double m2cx = query.value("m2cx").toDouble();
  const double pccx = query.value("pccx").toDouble();

  unCaixa = un == "M2" or un == "M²" or un == "ML" ? m2cx : pccx;

  qDebug() << "unCaixa: " << unCaixa;

  QInputDialog input;
  bool ok = false;
  // TODO: 0put this outside transaction
  caixasDefeito = input.getInt(this, produto, "Caixas quebradas: ", caixas, 0, caixas, 1, &ok);

  if (not ok or caixasDefeito == 0) return false;

  if (not quebrarLinha(row, caixas)) return false;
  //  if (not criarConsumo(row)) return false;
  if (not desfazerConsumo(idEstoque)) return false;

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

bool InputDialogConfirmacao::quebraEntrega(const int row) {
  // model is veiculo_has_produto

  const QString produto = model.data(row, "produto").toString();
  const double caixas = model.data(row, "caixas").toDouble();
  unCaixa = model.data(row, "unCaixa").toDouble(); // *

  QInputDialog input;
  bool ok = false;
  // TODO: 0put this outside transaction
  caixasDefeito = input.getInt(this, produto, "Caixas quebradas: ", 0, 0, caixas, 1, &ok); // *

  if (not ok or caixasDefeito == 0) return false;

  // diminuir quantidade da linha selecionada

  // recalcular kg? (posso usar proporcao para nao precisar puxar kgcx)
  if (not model.setData(row, "caixas", caixas - caixasDefeito)) return false;
  if (not model.setData(row, "quant", (caixas - caixasDefeito) * unCaixa)) return false;

  // copiar linha com quantDefeito

  const int rowQuebrado = model.rowCount();
  model.insertRow(rowQuebrado);

  for (int col = 0; col < model.columnCount(); ++col) {
    if (model.fieldIndex("id") == col) continue;
    if (model.fieldIndex("created") == col) continue;
    if (model.fieldIndex("lastUpdated") == col) continue;

    if (not model.setData(rowQuebrado, col, model.data(row, col))) return false;
  }

  // recalcular kg? (posso usar proporcao para nao precisar puxar kgcx)
  if (not model.setData(rowQuebrado, "caixas", caixasDefeito)) return false;
  if (not model.setData(rowQuebrado, "quant", caixasDefeito * unCaixa)) return false;
  if (not model.setData(rowQuebrado, "status", "QUEBRADO")) return false;

  // perguntar se gerar credito ou reposicao

  QMessageBox msgBox(QMessageBox::Question, "Atenção!", "Criar reposição ou gerar crédito?", QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, this);
  msgBox.setButtonText(QMessageBox::Yes, "Criar reposição");
  msgBox.setButtonText(QMessageBox::No, "Gerar crédito");
  msgBox.setButtonText(QMessageBox::Cancel, "Cancelar");

  const int choice = msgBox.exec();

  if (choice == QMessageBox::Cancel) return false;

  //
  modelVenda.setTable("venda_has_produto");
  modelVenda.setEditStrategy(QSqlTableModel::OnManualSubmit);

  modelVenda.setFilter("idVendaProduto = " + model.data(row, "idVendaProduto").toString());

  if (not modelVenda.select()) {
    emit errorSignal("Erro lendo tabela produto venda: " + modelVenda.lastError().text());
    return false;
  }
  //

  choice == QMessageBox::Yes ? criarReposicaoCliente() : gerarCreditoCliente();

  return true;
}

bool InputDialogConfirmacao::gerarCreditoCliente() {
  const QString idVenda = modelVenda.data(0, "idVenda").toString();
  const double descUnitario = modelVenda.data(0, "descUnitario").toDouble();

  const double credito = caixasDefeito * unCaixa * descUnitario;

  QMessageBox::information(this, "Crédito", "Gerado crédito no valor de R$ " + QLocale(QLocale::Portuguese).toString(credito));

  modelCliente.setTable("cliente");
  modelCliente.setEditStrategy(QSqlTableModel::OnManualSubmit);

  QSqlQuery query;
  query.prepare("SELECT idCliente FROM venda WHERE idVenda = :idVenda");
  query.bindValue(":idVenda", idVenda);

  if (not query.exec() or not query.first()) {
    emit errorSignal("Erro buscando cliente: " + query.lastError().text());
    return false;
  }

  modelCliente.setFilter("idCliente = " + query.value("idCliente").toString());

  if (not modelCliente.select()) {
    emit errorSignal("Erro lendo tabela cliente: " + modelCliente.lastError().text());
    return false;
  }

  const double creditoAntigo = modelCliente.data(0, "credito").toDouble();

  modelCliente.setData(0, "credito", credito + creditoAntigo);

  return true;
}

bool InputDialogConfirmacao::criarReposicaoCliente() {
  const int newRow = modelVenda.rowCount();
  modelVenda.insertRow(newRow);

  // copiar linha com quantidade quebrada
  for (int col = 0; col < modelVenda.columnCount(); ++col) {
    if (modelVenda.fieldIndex("idVendaProduto") == col) continue;
    if (modelVenda.fieldIndex("entregou") == col) continue;
    if (modelVenda.fieldIndex("idCompra") == col) continue;
    if (modelVenda.fieldIndex("idNFeSaida") == col) continue;
    if (modelVenda.fieldIndex("dataPrevCompra") == col) continue;
    if (modelVenda.fieldIndex("dataRealCompra") == col) continue;
    if (modelVenda.fieldIndex("dataPrevConf") == col) continue;
    if (modelVenda.fieldIndex("dataRealConf") == col) continue;
    if (modelVenda.fieldIndex("dataPrevFat") == col) continue;
    if (modelVenda.fieldIndex("dataRealFat") == col) continue;
    if (modelVenda.fieldIndex("dataPrevColeta") == col) continue;
    if (modelVenda.fieldIndex("dataRealColeta") == col) continue;
    if (modelVenda.fieldIndex("dataPrevReceb") == col) continue;
    if (modelVenda.fieldIndex("dataRealReceb") == col) continue;
    if (modelVenda.fieldIndex("dataPrevEnt") == col) continue;
    if (modelVenda.fieldIndex("created") == col) continue;
    if (modelVenda.fieldIndex("lastUpdated") == col) continue;

    if (not modelVenda.setData(newRow, col, modelVenda.data(0, col))) return false;
  }

  if (not modelVenda.setData(newRow, "quant", caixasDefeito * unCaixa)) return false;
  if (not modelVenda.setData(newRow, "caixas", caixasDefeito)) return false;
  if (not modelVenda.setData(newRow, "parcial", 0)) return false;
  if (not modelVenda.setData(newRow, "desconto", 0)) return false;
  if (not modelVenda.setData(newRow, "parcialDesc", 0)) return false;
  if (not modelVenda.setData(newRow, "descGlobal", 0)) return false;
  if (not modelVenda.setData(newRow, "total", 0)) return false;
  if (not modelVenda.setData(newRow, "status", "REPO. ENTREGA")) return false;
  if (not modelVenda.setData(newRow, "reposicao", true)) return false;

  const QString obs = QInputDialog::getText(this, "Observacao", "Observacao: ");
  if (not modelVenda.setData(newRow, "obs", "(REPO. ENTREGA) " + obs)) return false;

  return true;
}

bool InputDialogConfirmacao::quebrarLinha(const int row, const int caixas) {
  // TODO: 5ao marcar caixas quebradas e só houver uma nao dividir em duas linhas (para nao ficar linha zerado)
  // diminuir quant. da linha selecionada

  if (not model.setData(row, "caixas", caixas - caixasDefeito)) return false;
  if (not model.setData(row, "quant", (caixas - caixasDefeito) * unCaixa)) return false;

  // copiar linha com defeito

  const int rowQuebrado = model.rowCount();
  model.insertRow(rowQuebrado);

  for (int col = 0; col < model.columnCount(); ++col) {
    if (model.fieldIndex("idEstoque") == col) continue;
    if (model.fieldIndex("created") == col) continue;
    if (model.fieldIndex("lastUpdated") == col) continue;

    model.setData(rowQuebrado, col, model.data(row, col));
  }

  const QString obs = QInputDialog::getText(this, "Observacao", "Observacao: ");

  model.setData(rowQuebrado, "observacao", "(REPO. RECEB.) " + obs);
  model.setData(rowQuebrado, "caixas", caixasDefeito);
  model.setData(rowQuebrado, "quant", caixasDefeito * unCaixa);
  model.setData(rowQuebrado, "status", "QUEBRADO");
  model.setData(rowQuebrado, "lote", "Estoque: " + model.data(row, "idEstoque").toString());
  // recalcular proporcional dos valores

  if (not model.submitAll()) {
    emit errorSignal("Erro dividindo linhas: " + model.lastError().text());
    return false;
  }

  return true;
}

bool InputDialogConfirmacao::criarConsumo(const int row) {
  SqlRelationalTableModel modelConsumo;
  modelConsumo.setTable("estoque_has_consumo");
  modelConsumo.setEditStrategy(QSqlTableModel::OnManualSubmit);

  modelConsumo.setFilter("idEstoque = " + model.data(row, "idEstoque").toString());

  if (not modelConsumo.select()) {
    emit errorSignal("Erro lendo tabela consumo: " + modelConsumo.lastError().text());
    return false;
  }

  // criar consumo 'quebrado'

  const int rowConsumo = modelConsumo.rowCount();
  modelConsumo.insertRow(rowConsumo);

  if (not modelConsumo.setData(rowConsumo, "idEstoque", model.data(row, "idEstoque"))) return false;
  if (not modelConsumo.setData(rowConsumo, "idVendaProduto", 0)) return false;
  if (not modelConsumo.setData(rowConsumo, "status", "QUEBRADO")) return false;
  if (not modelConsumo.setData(rowConsumo, "local", "TEMP")) return false;
  if (not modelConsumo.setData(rowConsumo, "idProduto", model.data(row, "idProduto"))) return false;
  if (not modelConsumo.setData(rowConsumo, "fornecedor", model.data(row, "fornecedor"))) return false;
  if (not modelConsumo.setData(rowConsumo, "descricao", model.data(row, "descricao"))) return false;
  if (not modelConsumo.setData(rowConsumo, "quant", caixasDefeito * unCaixa * -1)) return false;
  if (not modelConsumo.setData(rowConsumo, "quantUpd", 5)) return false;
  if (not modelConsumo.setData(rowConsumo, "caixas", caixasDefeito)) return false;

  qDebug() << "quant quebrado: " << caixasDefeito * unCaixa * -1;
  qDebug() << "caixas quebrado: " << caixasDefeito;

  if (not modelConsumo.submitAll()) {
    emit errorSignal("Erro atualizando consumo: " + modelConsumo.lastError().text());
    return false;
  }

  return true;
}

bool InputDialogConfirmacao::desfazerConsumo(const int idEstoque) {
  QSqlQuery query;
  query.prepare("SELECT COALESCE(e.caixas - SUM(ehc.caixas), 0) AS sobra FROM estoque_has_consumo ehc LEFT JOIN estoque e ON ehc.idEstoque = e.idEstoque WHERE ehc.idEstoque = :idEstoque");
  query.bindValue(":idEstoque", idEstoque);

  if (not query.exec() or not query.first()) {
    emit errorSignal("Erro buscando sobra estoque: " + query.lastError().text());
    return false;
  }

  double sobra = query.value("sobra").toInt();
  qDebug() << "sobra: " << sobra;
  qDebug() << "caixasDefeito: " << caixasDefeito;

  if (sobra < 0) {
    // faltando pecas para consumo, desfazer os consumos com prazo maior

    query.prepare("SELECT CAST((`v`.`data` + INTERVAL `v`.`prazoEntrega` DAY) AS DATE) AS `prazoEntrega`, ehc.* FROM estoque_has_consumo ehc LEFT JOIN venda_has_produto vp ON ehc.idVendaProduto = "
                  "vp.idVendaProduto LEFT JOIN venda v ON vp.idVenda = v.idVenda WHERE ehc.idEstoque = :idEstoque ORDER BY prazoEntrega DESC");
    query.bindValue(":idEstoque", idEstoque);

    if (not query.exec()) {
      emit errorSignal("Erro buscando consumo estoque: " + query.lastError().text());
      return false;
    }

    while (query.next()) {
      const int caixas = query.value("caixas").toInt();

      QSqlQuery query2;
      // TODO: 0se a parte não quebrada for suficiente nao desfazer consumos (tomar cuidado para usar o idEstoque do restante e nao do quebrado)
      query2.prepare("DELETE FROM estoque_has_consumo WHERE idConsumo = :idConsumo");
      query2.bindValue(":idConsumo", query.value("idConsumo"));

      if (not query2.exec()) {
        emit errorSignal("Erro removendo consumo: " + query2.lastError().text());
        return false;
      }

      query2.prepare("UPDATE venda_has_produto SET status = 'REPO. RECEB.' WHERE idVendaProduto = :idVendaProduto");
      query2.bindValue(":idVendaProduto", query.value("idVendaProduto"));

      if (not query2.exec()) {
        emit errorSignal("Erro voltando produto para pendente: " + query2.lastError().text());
        return false;
      }

      sobra += caixas;
      if (sobra >= 0) break;
    }
  }

  return true;
}

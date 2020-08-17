#include "nfedistribuicao.h"
#include "ui_nfedistribuicao.h"

#include "acbr.h"
#include "application.h"
#include "reaisdelegate.h"
#include "usersession.h"
#include "xml_viewer.h"

#include <QFile>
#include <QInputDialog>
#include <QSqlError>
#include <QTimer>

NFeDistribuicao::NFeDistribuicao(QWidget *parent) : QWidget(parent), ui(new Ui::NFeDistribuicao) {
  ui->setupUi(this);

  if (UserSession::getSetting("User/monitorarNFe").value_or(false).toBool()) {
    updateTables();

    const QDateTime today = QDateTime::currentDateTime();
    const QDateTime tomorrow = QDateTime(QDate::currentDate().addDays(1), QTime(0, 0, 0));

    auto timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [=] {
      const QDateTime today2 = QDateTime::currentDateTime();
      const QDateTime tomorrow2 = QDateTime(QDate::currentDate().addDays(1), QTime(0, 0, 0));

      timer->start(today2.msecsTo(tomorrow2));

      qApp->setSilent(true);
      on_pushButtonPesquisar_clicked();
      qApp->setSilent(false);
    });
    timer->start(today.msecsTo(tomorrow));
  }
}

NFeDistribuicao::~NFeDistribuicao() { delete ui; }

void NFeDistribuicao::resetTables() { modelIsSet = false; }

void NFeDistribuicao::updateTables() {
  if (not isSet) {
    setConnections();
    isSet = true;
  }

  if (not modelIsSet) {
    setupTables();
    modelIsSet = true;
  }

  buscarNSU();

  if (not model.select()) { return; }
}

void NFeDistribuicao::buscarNSU() {
  const auto idLoja = UserSession::getSetting("User/lojaACBr");

  if (not idLoja) { return qApp->enqueueError("Não está configurado qual loja usar no ACBr! Escolher em Configurações!"); }

  QSqlQuery queryLoja;

  if (not queryLoja.exec("SELECT cnpj, ultimoNSU, maximoNSU FROM loja WHERE idLoja = " + idLoja->toString()) or not queryLoja.first()) {
    return qApp->enqueueException("Erro buscando CNPJ da loja: " + queryLoja.lastError().text(), this);
  }

  ui->spinBoxMaxNSU->setValue(queryLoja.value("maximoNSU").toInt());
  ui->spinBoxUltNSU->setValue(queryLoja.value("ultimoNSU").toInt());
  ui->lineEditCNPJ->setText(queryLoja.value("cnpj").toString().remove(".").remove("/").remove("-"));

  ui->lineEditCNPJ->setVisible(false);
}

void NFeDistribuicao::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->pushButtonCiencia, &QPushButton::clicked, this, &NFeDistribuicao::on_pushButtonCiencia_clicked, connectionType);
  connect(ui->pushButtonConfirmacao, &QPushButton::clicked, this, &NFeDistribuicao::on_pushButtonConfirmacao_clicked, connectionType);
  connect(ui->pushButtonDesconhecimento, &QPushButton::clicked, this, &NFeDistribuicao::on_pushButtonDesconhecimento_clicked, connectionType);
  connect(ui->pushButtonNaoRealizada, &QPushButton::clicked, this, &NFeDistribuicao::on_pushButtonNaoRealizada_clicked, connectionType);
  connect(ui->pushButtonPesquisar, &QPushButton::clicked, this, &NFeDistribuicao::on_pushButtonPesquisar_clicked, connectionType);
  connect(ui->table, &TableView::activated, this, &NFeDistribuicao::on_table_activated, connectionType);
}

void NFeDistribuicao::setupTables() {
  model.setTable("nfe");

  model.setFilter("nsu IS NOT NULL");

  model.setSort("nsu", Qt::DescendingOrder);

  model.setHeaderData("numeroNFe", "NFe");
  model.setHeaderData("tipo", "Tipo");
  model.setHeaderData("status", "Status");
  model.setHeaderData("emitente", "Emitente");
  model.setHeaderData("cnpjDest", "CNPJ Dest.");
  model.setHeaderData("cnpjOrig", "CNPJ Orig.");
  model.setHeaderData("chaveAcesso", "Chave Acesso");
  model.setHeaderData("valor", "R$");
  model.setHeaderData("nsu", "NSU");
  model.setHeaderData("statusDistribuicao", "Distribuição");
  model.setHeaderData("dataDistribuicao", "Data Evento");

  ui->table->setModel(&model);

  ui->table->hideColumn("idNFe");
  ui->table->hideColumn("idVenda");
  ui->table->hideColumn("xml");
  ui->table->hideColumn("obs");
  ui->table->hideColumn("transportadora");
  ui->table->hideColumn("gare");
  ui->table->hideColumn("gareData");
  ui->table->hideColumn("infCpl");
  ui->table->hideColumn("utilizada");
  ui->table->hideColumn("confirmar");

  ui->table->setItemDelegateForColumn("valor", new ReaisDelegate(2, true, this));
}

void NFeDistribuicao::on_pushButtonPesquisar_clicked() {
  // 1. chamar funcao DistribuicaoDFePorUltNSU no ACBr
  // 2. guardar resumo e eventos das NFes retornadas
  // 3. enquanto maxNSU != ultNSU repetir os passos
  // 4. fazer evento de ciencia para cada nota
  // 5. repetir passo 1 para pegar os xmls das notas

  const auto idLoja = UserSession::getSetting("User/lojaACBr");

  if (not idLoja) { return qApp->enqueueError("Não está configurado qual loja usar no ACBr! Escolher em Configurações!"); }

  //----------------------------------------------------------

  QFile file("LOG_DFe.txt");

  if (not file.open(QFile::ReadOnly)) { return; }

  const QString resposta = file.readAll();

  file.close();

  qDebug() << "size: " << resposta.length();

  if (resposta.contains("ERRO: ")) { return qApp->enqueueException(resposta, this); }

  //----------------------------------------------------------

  //  ACBr acbrLocal;
  //  const auto respostaOptional = acbrLocal.enviarComando("NFe.DistribuicaoDFePorUltNSU(\"35\", \"" + ui->lineEditCNPJ->text() + "\", " + QString::number(ui->spinBoxUltNSU->value()) + ")", true);

  //  qDebug() << "resposta: " << respostaOptional.value_or("erro");

  //  if (not respostaOptional) { return; }

  //  const QString resposta = respostaOptional.value();

  //  if (resposta.contains("ERRO: ")) { return qApp->enqueueException(resposta, this); }

  //----------------------------------------------------------

  if (not qApp->startTransaction("NFeDistribuicao::on_pushButtonPesquisar")) { return; }

  if (not pesquisarNFes(resposta, idLoja->toString())) { return qApp->rollbackTransaction(); }

  if (not qApp->endTransaction()) { return; }

  //----------------------------------------------------------

  if (not model.select()) { return; }

  //  if (ui->spinBoxUltNSU->value() < ui->spinBoxMaxNSU->value()) { return on_pushButtonPesquisar_clicked(); }

  //  if (darCiencia()) { return on_pushButtonPesquisar_clicked(); }

  //  confirmar();

  QSqlQuery queryMaintenance;

  if (not queryMaintenance.exec("UPDATE maintenance SET lastDistribuicao = NOW()")) { return qApp->enqueueException("Erro guardando lastDistribuicao:" + queryMaintenance.lastError().text(), this); }

  qApp->enqueueInformation("Operação realizada com sucesso!", this);
}

void NFeDistribuicao::confirmar() {
  auto match = model.multiMatch({{"confirmar", true}, {"statusDistribuicao", "CIÊNCIA"}});

  while (match.size() > 0) {
    // select up to 20 rows

    const auto selection = match.mid(0, 20);

    for (auto row : selection) { ui->table->selectRow(row); }

    if (not enviarEvento("210200", "CONFIRMAÇÃO")) { return; }

    match = model.multiMatch({{"confirmar", true}, {"statusDistribuicao", "CIÊNCIA"}});
  }
}

bool NFeDistribuicao::darCiencia() {
  auto match = model.multiMatch({{"statusDistribuicao", "DESCONHECIDO"}});

  bool pesquisar = false;

  while (match.size() > 0) {
    // select up to 20 rows
    const auto selection = match.mid(0, 20);

    for (auto row : selection) { ui->table->selectRow(row); }

    if (not enviarEvento("210210", "CIÊNCIA")) { return false; }

    match = model.multiMatch({{"statusDistribuicao", "DESCONHECIDO"}});

    pesquisar = true;
  }

  return pesquisar;
}

bool NFeDistribuicao::pesquisarNFes(const QString &resposta, const QString &idLoja) {
  const QStringList eventos = resposta.split("\r\n\r\n", Qt::SkipEmptyParts);
  const QString cnpjDest = ui->lineEditCNPJ->text();

  for (const auto &evento : eventos) {
    if (evento.contains("[DistribuicaoDFe]")) {
      const int indexMaxNSU = evento.indexOf("\r\nmaxNSU=");
      const int indexUltNSU = evento.indexOf("\r\nultNSU=");

      // TODO: calcular quantidade de eventos retornados (ultNSU - NSUenviado) e verificar se na resposta veio todos
      // erro de socket resulta em uma resposta cortada
      // levar em conta que alguns eventos sao duplos, como  ProEve/InfEve e devem ser contados como um unico evento

      if (indexMaxNSU == -1 or indexUltNSU == -1) { return qApp->enqueueException(false, "Não encontrou o campo 'maxNSU/ultNSU'!", this); }

      const QString maxNSU = evento.mid(indexMaxNSU + 9).split("\r\n").first();
      const QString ultNSU = evento.mid(indexUltNSU + 9).split("\r\n").first();

      ui->spinBoxMaxNSU->setValue(maxNSU.toInt());
      ui->spinBoxUltNSU->setValue(ultNSU.toInt());

      QSqlQuery queryLoja2;

      if (not queryLoja2.exec("UPDATE loja SET ultimoNSU = " + ultNSU + ", maximoNSU = " + maxNSU + " WHERE idLoja = " + idLoja)) {
        return qApp->enqueueException(false, "Erro guardando NSU: " + queryLoja2.lastError().text(), this);
      }
    }

    if (evento.contains("[ResNFe")) {
      const int indexChave = evento.indexOf("\r\nchNFe=");

      if (indexChave == -1) { return qApp->enqueueException(false, "Não encontrou o campo 'chNFe'!", this); }

      const QString chaveAcesso = evento.mid(indexChave + 8).split("\r\n").first();
      const QString numeroNFe = chaveAcesso.mid(25, 9);

      //----------------------------------------------------------

      const int indexCnpj = evento.indexOf("\r\nCNPJCPF=");

      if (indexCnpj == -1) { return qApp->enqueueException(false, "Não encontrou o campo 'CNPJCPF'!", this); }

      const QString cnpjOrig = evento.mid(indexCnpj + 10).split("\r\n").first();

      //----------------------------------------------------------

      const int indexNome = evento.indexOf("\r\nxNome=");

      if (indexNome == -1) { return qApp->enqueueException(false, "Não encontrou o campo 'xNome'!", this); }

      const QString nomeEmitente = evento.mid(indexNome + 8).split("\r\n").first();

      //----------------------------------------------------------

      const int indexValor = evento.indexOf("\r\nvNF=");

      if (indexValor == -1) { return qApp->enqueueException(false, "Não encontrou o campo 'vNF'!", this); }

      const QString valor = evento.mid(indexValor + 6).split("\r\n").first().replace(",", ".");

      //----------------------------------------------------------

      const int indexNsu = evento.indexOf("\r\nNSU=");

      if (indexNsu == -1) { return qApp->enqueueException(false, "Não encontrou o campo 'NSU'!", this); }

      const QString nsu = evento.mid(indexNsu + 6).split("\r\n").first();

      //----------------------------------------------------------

      const int indexXML = evento.indexOf("\r\nXML=");
      const int indexArquivo = evento.indexOf("\r\narquivo=");

      if (indexXML == -1 or indexArquivo == -1) { return qApp->enqueueException(false, "Não encontrou o campo 'XML'!", this); }

      const QString xml = evento.mid(indexXML + 6, indexArquivo - indexXML - 6);

      //----------------------------------------------------------

      const int indexSchema = evento.indexOf("\r\nschema=");

      if (indexSchema == -1) { return qApp->enqueueException(false, "Não encontrou o campo 'schema'!", this); }

      const QString schema = evento.mid(indexSchema + 9).split("\r\n").first();

      //----------------------------------------------------------

      QSqlQuery queryExiste;

      if (not queryExiste.exec("SELECT status FROM nfe WHERE chaveAcesso = '" + chaveAcesso + "'")) {
        return qApp->enqueueException(false, "Erro verificando se NFe já cadastrada: " + queryExiste.lastError().text(), this);
      }

      const bool existe = queryExiste.first();

      //----------------------------------------------------------

      if (not existe) {
        const QString status = (schema == "procNFe") ? "AUTORIZADO" : "RESUMO";

        QSqlQuery queryCadastrar;

        if (not queryCadastrar.exec("INSERT INTO nfe (numeroNFe, tipo, xml, status, emitente, cnpjDest, cnpjOrig, chaveAcesso, transportadora, valor, infCpl, nsu, statusDistribuicao) VALUES ('" +
                                    numeroNFe + "', 'ENTRADA', '" + xml + "', '" + status + "', '" + nomeEmitente + "', '" + cnpjDest + "', '" + cnpjOrig + "', '" + chaveAcesso + "', '" +
                                    encontraTransportadora(xml) + "', '" + valor + "', '" + encontraInfCpl(xml) + "', '" + nsu + "', 'DESCONHECIDO')")) {
          return qApp->enqueueException(false, "Erro cadastrando resumo da NFe: " + queryCadastrar.lastError().text(), this);
        }
      }

      if (existe and schema == "procNFe") {
        if (queryExiste.value("status").toString() == "AUTORIZADO") { continue; }

        QSqlQuery queryAtualizar;

        if (not queryAtualizar.exec("UPDATE nfe SET status = 'AUTORIZADO', xml = '" + xml + "', transportadora = '" + encontraTransportadora(xml) + "', infCpl = '" + encontraInfCpl(xml) +
                                    "' WHERE chaveAcesso = '" + chaveAcesso + "' AND status = 'RESUMO'")) {
          return qApp->enqueueException(false, "Erro atualizando xml: " + queryAtualizar.lastError().text(), this);
        }
      }
    }

    if (evento.contains("[InfEve")) {
      const int indexTipo = evento.indexOf("\r\nxEvento=");

      if (indexTipo == -1) { return qApp->enqueueException(false, "Não encontrou o campo 'xEvento'!", this); }

      const QString eventoTipo = evento.mid(indexTipo + 10).split("\r\n").first();

      //----------------------------------------------------------

      const int indexChave = evento.indexOf("\r\nchNFe=");

      if (indexChave == -1) { return qApp->enqueueException(false, "Não encontrou o campo 'chNFe'!", this); }

      const QString chaveAcesso = evento.mid(indexChave + 8).split("\r\n").first();

      //----------------------------------------------------------

      QSqlQuery queryExiste;

      if (not queryExiste.exec("SELECT statusDistribuicao FROM nfe WHERE chaveAcesso = '" + chaveAcesso + "'")) {
        return qApp->enqueueException(false, "Erro verificando se NFe já cadastrada: " + queryExiste.lastError().text(), this);
      }

      if (queryExiste.first()) {
        const QString statusDistribuicao = queryExiste.value("statusDistribuicao").toString();

        if (eventoTipo == "Ciencia da Operacao" and statusDistribuicao == "DESCONHECIDO") {
          QSqlQuery queryAtualiza;

          if (not queryAtualiza.exec("UPDATE nfe SET statusDistribuicao = 'CIÊNCIA' WHERE chaveAcesso = '" + chaveAcesso + "'")) {
            return qApp->enqueueException(false, "Erro atualizando statusDistribuicao: " + queryAtualiza.lastError().text());
          }
        }

        if (eventoTipo == "Confirmacao da Operacao" and (statusDistribuicao == "DESCONHECIDO" or statusDistribuicao == "CIÊNCIA")) {
          QSqlQuery queryAtualiza;

          if (not queryAtualiza.exec("UPDATE nfe SET statusDistribuicao = 'CONFIRMAÇÃO' WHERE chaveAcesso = '" + chaveAcesso + "'")) {
            return qApp->enqueueException(false, "Erro atualizando statusDistribuicao: " + queryAtualiza.lastError().text());
          }
        }

        if (eventoTipo == "CANCELAMENTO" and statusDistribuicao != "CANCELADA") {
          QSqlQuery queryAtualiza;

          if (not queryAtualiza.exec("UPDATE nfe SET status = 'CANCELADA', statusDistribuicao = 'CANCELADA' WHERE chaveAcesso = '" + chaveAcesso + "'")) {
            return qApp->enqueueException(false, "Erro atualizando statusDistribuicao: " + queryAtualiza.lastError().text());
          }
        }

        // TODO: implement
        //        if (eventoTipo == "DESCONHECIMENTO" and statusDistribuicao == "DESCONHECIDO") {}
        //        if (eventoTipo == "NÃO REALIZADA" and statusDistribuicao == "DESCONHECIDO") {}
      }
    }
  }

  return true;
}

void NFeDistribuicao::on_pushButtonCiencia_clicked() {
  if (not enviarEvento("210210", "CIÊNCIA")) { return; }

  qApp->enqueueInformation("Operação realizada com sucesso!", this);
}

void NFeDistribuicao::on_pushButtonConfirmacao_clicked() {
  if (not enviarEvento("210200", "CONFIRMAÇÃO")) { return; }

  qApp->enqueueInformation("Operação realizada com sucesso!", this);
}

void NFeDistribuicao::on_pushButtonDesconhecimento_clicked() {
  if (not enviarEvento("210220", "DESCONHECIMENTO")) { return; }

  qApp->enqueueInformation("Operação realizada com sucesso!", this);
}

void NFeDistribuicao::on_pushButtonNaoRealizada_clicked() {
  if (not enviarEvento("210240", "NÃO REALIZADA")) { return; }

  qApp->enqueueInformation("Operação realizada com sucesso!", this);
}

bool NFeDistribuicao::enviarEvento(const QString &codigoEvento, const QString &operacao) {
  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { return qApp->enqueueError(false, "Nenhuma linha selecionada!", this); }

  if (selection.size() > 20) { return qApp->enqueueError(false, "Deve selecionar no máximo 20 linhas!", this); }

  //----------------------------------------------------------

  const QString cnpj = ui->lineEditCNPJ->text();
  const QString dataHora = qApp->serverDateTime().toString("dd/MM/yyyy HH:mm");

  QString justificativa;

  if (operacao == "NÃO REALIZADA") {
    justificativa = QInputDialog::getText(this, "Justificativa", "Entre 15 e 255 caracteres: ");
    if (justificativa.size() < 15 or justificativa.size() > 255) { return qApp->enqueueError(false, "Justificativa fora do tamanho!", this); }
  }

  QString comando;
  comando += "NFE.EnviarEvento(\"[EVENTO]\r\n";
  comando += "idLote = 1\r\n";

  int count = 0;

  for (const auto &index : selection) {
    const QString statusDistribuicao = model.data(index.row(), "statusDistribuicao").toString();

    if (statusDistribuicao == "CANCELADA") { continue; }

    if (operacao == "CIÊNCIA" and statusDistribuicao != "DESCONHECIDO") { continue; }

    const QString chaveAcesso = model.data(index.row(), "chaveAcesso").toString();
    const QString numEvento = QString("%1").arg(++count, 3, 10, QChar('0')); // padding with zeros

    comando += "[EVENTO" + numEvento + "]\r\n";
    comando += "cOrgao = 91\r\n";
    comando += "CNPJ = " + cnpj + "\r\n";
    comando += "chNFe = " + chaveAcesso + "\r\n";
    comando += "dhEvento = " + dataHora + "\r\n";
    comando += "tpEvento = " + codigoEvento + "\r\n";
    comando += "nSeqEvento = 1\r\n";
    comando += "versaoEvento = 1.00\r\n";
    if (operacao == "NÃO REALIZADA") { comando += "xJust = " + justificativa + "\r\n"; }
  }

  comando += "\")";

  //----------------------------------------------------------

  ACBr acbrLocal;
  const auto respostaOptional = acbrLocal.enviarComando(comando, true);

  //  qDebug() << "resposta evento: " << respostaOptional.value_or("erro");

  if (not respostaOptional) { return false; }

  const QString resposta = respostaOptional.value();

  if (resposta.contains("ERRO: ")) { return qApp->enqueueException(false, resposta, this); }

  //----------------------------------------------------------

  const QStringList eventos = resposta.split("\r\n\r\n", Qt::SkipEmptyParts);

  if (not qApp->startTransaction("NFeDistribuicao::on_pushButtonCiencia")) { return false; }

  for (const auto &evento : eventos) {
    if (evento.contains("XMotivo=Rejeicao: Evento de Ciencia da Operacao para NFe cancelada ou denegada")) {
      const int indexChave = evento.indexOf("\r\nchNFe=");

      if (indexChave == -1) {
        qApp->enqueueException("Não encontrou o campo 'chNFe'!", this);
        return qApp->rollbackTransaction(false);
      }

      const QString chaveAcesso = evento.mid(indexChave + 8).split("\r\n").first();

      QSqlQuery query;

      if (not query.exec("UPDATE nfe SET status = 'CANCELADA', statusDistribuicao = 'CANCELADA' WHERE chaveAcesso = '" + chaveAcesso + "'")) {
        qApp->enqueueException("Erro atualizando status da NFe: " + query.lastError().text(), this);
        return qApp->rollbackTransaction(false);
      }
    }

    if (evento.contains("XMotivo=Evento registrado e vinculado a NF-e") or evento.contains("XMotivo=Rejeicao: Duplicidade de evento")) {
      const int indexChave = evento.indexOf("\r\nchNFe=");

      if (indexChave == -1) {
        qApp->enqueueException("Não encontrou o campo 'chNFe'!", this);
        return qApp->rollbackTransaction(false);
      }

      const QString chaveAcesso = evento.mid(indexChave + 8).split("\r\n").first();

      QSqlQuery query;

      if (not query.exec("UPDATE nfe SET statusDistribuicao = '" + operacao + "', dataDistribuicao = NOW() WHERE chaveAcesso = '" + chaveAcesso + "'")) {
        qApp->enqueueException("Erro atualizando status da NFe: " + query.lastError().text(), this);
        return qApp->rollbackTransaction(false);
      }
    }
  }

  if (not model.submitAll()) { return qApp->rollbackTransaction(false); }

  if (not qApp->endTransaction()) { return false; }

  return true;
}

void NFeDistribuicao::on_table_activated(const QModelIndex &index) {
  const QByteArray xml = model.data(index.row(), "xml").toByteArray();

  if (xml.isEmpty()) { return qApp->enqueueException("XML vazio!", this); }

  auto *viewer = new XML_Viewer(xml, this);
  viewer->setAttribute(Qt::WA_DeleteOnClose);
}

QString NFeDistribuicao::encontraInfCpl(const QString &xml) {
  const int indexInfCpl1 = xml.indexOf("<infCpl>");
  const int indexInfCpl2 = xml.indexOf("</infCpl>");

  if (indexInfCpl1 != -1 and indexInfCpl2 != -1) { return xml.mid(indexInfCpl1, indexInfCpl2 - indexInfCpl1).remove("<infCpl>"); }

  return "";
}

QString NFeDistribuicao::encontraTransportadora(const QString &xml) {
  const int indexTransportadora1 = xml.indexOf("<transporta>");
  const int indexTransportadora2 = xml.indexOf("</transporta>");

  if (indexTransportadora1 != -1 and indexTransportadora2 != -1) {
    const QString transportaTemp = xml.mid(indexTransportadora1, indexTransportadora2 - indexTransportadora1).remove("<transporta>");

    const int indexTransportaNome1 = transportaTemp.indexOf("<xNome>");
    const int indexTransportaNome2 = transportaTemp.indexOf("</xNome>");

    if (indexTransportaNome1 != -1 and indexTransportaNome2 != -1) { return transportaTemp.mid(indexTransportaNome1, indexTransportaNome2 - indexTransportaNome1).remove("<xNome>"); }
  }

  return "";
}

// TODO: pintar linhas de amarelo/vermelho a medida que aproximar do prazo para realizar uma operacao final

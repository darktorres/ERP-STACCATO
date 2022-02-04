#include "widgetnfedistribuicao.h"
#include "ui_widgetnfedistribuicao.h"

#include "acbrlib.h"
#include "application.h"
#include "nfeproxymodel.h"
#include "reaisdelegate.h"
#include "user.h"

#include <QInputDialog>
#include <QSqlError>
#include <QTimer>

WidgetNFeDistribuicao::WidgetNFeDistribuicao(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetNFeDistribuicao) {
  ui->setupUi(this);

  connect(&timer, &QTimer::timeout, this, &WidgetNFeDistribuicao::downloadAutomatico);
  timer.setTimerType(Qt::VeryCoarseTimer);
  timer.start(tempoTimer);
}

WidgetNFeDistribuicao::~WidgetNFeDistribuicao() { delete ui; }

void WidgetNFeDistribuicao::downloadAutomatico() {
  if (not User::getSetting("User/monitorarNFe").toBool()) { return; }

  timer.stop();

  // TODO: is this still needed?
  updateTables();

  qDebug() << "download Automatico";
  qApp->setSilent(true);

  try {
    consultarSefaz();
  } catch (std::exception &) {
    qApp->setSilent(false);
    timer.start(tempoTimer);
    throw;
  }

  qApp->setSilent(false);
  timer.start(tempoTimer);
}

void WidgetNFeDistribuicao::resetTables() { modelIsSet = false; }

void WidgetNFeDistribuicao::updateTables() {
  if (not isSet) {
    setConnections();
    ui->itemBoxLoja->setSearchDialog(SearchDialog::loja(this));
    isSet = true;
  }

  if (not modelIsSet) {
    setupTables();
    montaFiltro();
    modelIsSet = true;
  }

  model.select();
}

void WidgetNFeDistribuicao::buscarNSU() {
  const QString idLoja = ui->itemBoxLoja->getId().toString();

  if (idLoja.isEmpty()) { return; }

  SqlQuery queryLoja;

  if (not queryLoja.exec("SELECT cnpj, ultimoNSU, maximoNSU FROM loja WHERE idLoja = " + idLoja)) { throw RuntimeException("Erro buscando CNPJ da loja: " + queryLoja.lastError().text(), this); }

  if (not queryLoja.first()) { throw RuntimeException("Dados não encontrados para loja com id: '" + idLoja + "'", this); }

  maximoNSU = queryLoja.value("maximoNSU").toInt();
  ultimoNSU = queryLoja.value("ultimoNSU").toInt();
  cnpjDest = queryLoja.value("cnpj").toString().remove(".").remove("/").remove("-");
}

void WidgetNFeDistribuicao::setConnections() {
  if (not blockingSignals.isEmpty()) { blockingSignals.pop(); } // avoid crashing on first setConnections

  if (not blockingSignals.isEmpty()) { return; } // delay setting connections until last unset/set block

  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->checkBoxCancelada, &QCheckBox::toggled, this, &WidgetNFeDistribuicao::montaFiltro, connectionType);
  connect(ui->checkBoxCiencia, &QCheckBox::toggled, this, &WidgetNFeDistribuicao::montaFiltro, connectionType);
  connect(ui->checkBoxConfirmacao, &QCheckBox::toggled, this, &WidgetNFeDistribuicao::montaFiltro, connectionType);
  connect(ui->checkBoxDesconhecido, &QCheckBox::toggled, this, &WidgetNFeDistribuicao::montaFiltro, connectionType);
  connect(ui->checkBoxDesconhecimento, &QCheckBox::toggled, this, &WidgetNFeDistribuicao::montaFiltro, connectionType);
  connect(ui->checkBoxNaoRealizada, &QCheckBox::toggled, this, &WidgetNFeDistribuicao::montaFiltro, connectionType);
  connect(ui->groupBoxLoja, &QGroupBox::toggled, this, &WidgetNFeDistribuicao::montaFiltro, connectionType);
  connect(ui->groupBoxStatus, &QGroupBox::toggled, this, &WidgetNFeDistribuicao::on_groupBoxStatus_toggled, connectionType);
  connect(ui->itemBoxLoja, &ItemBox::textChanged, this, &WidgetNFeDistribuicao::buscarNSU, connectionType);
  connect(ui->itemBoxLoja, &ItemBox::textChanged, this, &WidgetNFeDistribuicao::montaFiltro, connectionType);
  connect(ui->pushButtonCiencia, &QPushButton::clicked, this, &WidgetNFeDistribuicao::on_pushButtonCiencia_clicked, connectionType);
  connect(ui->pushButtonConfirmacao, &QPushButton::clicked, this, &WidgetNFeDistribuicao::on_pushButtonConfirmacao_clicked, connectionType);
  connect(ui->pushButtonDesconhecimento, &QPushButton::clicked, this, &WidgetNFeDistribuicao::on_pushButtonDesconhecimento_clicked, connectionType);
  connect(ui->pushButtonNaoRealizada, &QPushButton::clicked, this, &WidgetNFeDistribuicao::on_pushButtonNaoRealizada_clicked, connectionType);
  connect(ui->pushButtonPesquisar, &QPushButton::clicked, this, &WidgetNFeDistribuicao::on_pushButtonPesquisar_clicked, connectionType);
  connect(ui->table, &TableView::activated, this, &WidgetNFeDistribuicao::on_table_activated, connectionType);
}

void WidgetNFeDistribuicao::unsetConnections() {
  blockingSignals.push(0);

  disconnect(ui->checkBoxCancelada, &QCheckBox::toggled, this, &WidgetNFeDistribuicao::montaFiltro);
  disconnect(ui->checkBoxCiencia, &QCheckBox::toggled, this, &WidgetNFeDistribuicao::montaFiltro);
  disconnect(ui->checkBoxConfirmacao, &QCheckBox::toggled, this, &WidgetNFeDistribuicao::montaFiltro);
  disconnect(ui->checkBoxDesconhecido, &QCheckBox::toggled, this, &WidgetNFeDistribuicao::montaFiltro);
  disconnect(ui->checkBoxDesconhecimento, &QCheckBox::toggled, this, &WidgetNFeDistribuicao::montaFiltro);
  disconnect(ui->checkBoxNaoRealizada, &QCheckBox::toggled, this, &WidgetNFeDistribuicao::montaFiltro);
  disconnect(ui->groupBoxLoja, &QGroupBox::toggled, this, &WidgetNFeDistribuicao::montaFiltro);
  disconnect(ui->groupBoxStatus, &QGroupBox::toggled, this, &WidgetNFeDistribuicao::on_groupBoxStatus_toggled);
  disconnect(ui->itemBoxLoja, &ItemBox::textChanged, this, &WidgetNFeDistribuicao::buscarNSU);
  disconnect(ui->itemBoxLoja, &ItemBox::textChanged, this, &WidgetNFeDistribuicao::montaFiltro);
  disconnect(ui->pushButtonCiencia, &QPushButton::clicked, this, &WidgetNFeDistribuicao::on_pushButtonCiencia_clicked);
  disconnect(ui->pushButtonConfirmacao, &QPushButton::clicked, this, &WidgetNFeDistribuicao::on_pushButtonConfirmacao_clicked);
  disconnect(ui->pushButtonDesconhecimento, &QPushButton::clicked, this, &WidgetNFeDistribuicao::on_pushButtonDesconhecimento_clicked);
  disconnect(ui->pushButtonNaoRealizada, &QPushButton::clicked, this, &WidgetNFeDistribuicao::on_pushButtonNaoRealizada_clicked);
  disconnect(ui->pushButtonPesquisar, &QPushButton::clicked, this, &WidgetNFeDistribuicao::on_pushButtonPesquisar_clicked);
  disconnect(ui->table, &TableView::activated, this, &WidgetNFeDistribuicao::on_table_activated);
}

void WidgetNFeDistribuicao::setupTables() {
  model.setTable("view_nfe_distribuicao");

  model.setHeaderData("numeroNFe", "NF-e");
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

  model.proxyModel = new NFeProxyModel(&model, this);

  ui->table->setModel(&model);

  ui->table->hideColumn("idNFe");
  ui->table->hideColumn("idVenda");
  ui->table->hideColumn("dataHoraEmissao");
  ui->table->hideColumn("emitente");
  ui->table->hideColumn("obs");
  ui->table->hideColumn("transportadora");
  ui->table->hideColumn("gare");
  ui->table->hideColumn("gareData");
  ui->table->hideColumn("infCpl");
  ui->table->hideColumn("utilizada");
  ui->table->hideColumn("ciencia");
  ui->table->hideColumn("confirmar");
  ui->table->hideColumn("desconhecer");
  ui->table->hideColumn("naoRealizar");

  ui->table->setItemDelegateForColumn("valor", new ReaisDelegate(2, true, this));
}

void WidgetNFeDistribuicao::on_pushButtonPesquisar_clicked() {
  timer.stop();

  try {
    consultarSefaz();
    qApp->enqueueInformation("Operação realizada com sucesso!", this);
  } catch (std::exception &) {
    timer.start(tempoTimer);
    throw;
  }

  timer.start(tempoTimer);
}

void WidgetNFeDistribuicao::enviarComando(ACBr &acbr) {
  qDebug() << "pesquisar cnpj - nsu: " << cnpjDest << " - " << ultimoNSU;

  // TODO: parametrizar o código do estado em vez de usar 35
  const QString resposta = acbr.enviarComando(R"(NFe.DistribuicaoDFePorUltNSU("35", ")" + cnpjDest + R"(", )" + QString::number(ultimoNSU) + ")", "Consultando NF-es do CNPJ " + cnpjDest + "...");

  // TODO: se essas mensagens são silenciosas como o usuario vai arrumar quando der erro?

  if (resposta.contains("Consumo Indevido", Qt::CaseInsensitive)) { tempoTimer = 1h; }
  if (resposta.contains("ERRO: Rejeicao: CNPJ-Base consultado difere do CNPJ-Base do Certificado Digital")) { throw RuntimeError("Certificado plugado é de outro CNPJ!"); }
  if (resposta.contains("ERRO: ", Qt::CaseInsensitive)) { throw RuntimeException(resposta, this); }

  tempoTimer = (resposta.contains("XMotivo=Nenhum documento localizado", Qt::CaseInsensitive)) ? 1h : 15min;

  //----------------------------------------------------------

  qApp->startTransaction("NFeDistribuicao::enviarComando");

  processarResposta(resposta, idLoja);

  qApp->endTransaction();

  model.select();
}

void WidgetNFeDistribuicao::buscarNFes(const QString &cnpjRaiz, const QString &servidor, const QString &porta) {
  // 1. chamar funcao DistribuicaoDFePorUltNSU no ACBr
  // 2. guardar resumo e eventos das NF-es retornadas
  // 3. enquanto maxNSU != ultNSU repetir os passos
  // 4. fazer evento de ciencia para cada nota
  // 5. repetir passo 1 para pegar os xmls das notas

  //----------------------------------------------------------

  //  File file("LOG_DFe.txt");

  //  if (not file.open(QFile::ReadOnly)) { return; }

  //  const QString resposta = file.readAll();

  //  file.close();

  //  qDebug() << "size: " << resposta.length();

  //  if (resposta.contains("ERRO: ")) { throw RuntimeException(resposta, this); }

  //----------------------------------------------------------

  // TODO: verificar se cnpjRaiz é valido

  SqlQuery queryCnpj;

  if (not queryCnpj.exec("SELECT idLoja, cnpj, ultimoNSU, maximoNSU FROM loja WHERE cnpj LIKE '" + cnpjRaiz + "%' AND desativado = FALSE")) {
    throw RuntimeException("Erro buscando filiais: " + queryCnpj.lastError().text());
  }

  if (queryCnpj.size() == 0) { throw RuntimeException("Nenhuma filial encontrada para CNPJ: " + cnpjRaiz); }

  ACBr acbr;
  acbr.setarServidor(servidor, porta);

  while (queryCnpj.next()) {
    idLoja = queryCnpj.value("idLoja").toString();
    cnpjDest = queryCnpj.value("cnpj").toString().remove(".").remove("/").remove("-");
    ultimoNSU = queryCnpj.value("ultimoNSU").toInt();
    maximoNSU = queryCnpj.value("maximoNSU").toInt();

    if (houveConsultaEmOutroPc()) { continue; }

    enviarComando(acbr);

    //----------------------------------------------------------

    while (ultimoNSU < maximoNSU) {
      qDebug() << "pesquisar novamente";
      enviarComando(acbr);
    }

    qDebug() << "darCiencia";
    darCiencia(acbr, true);
    qDebug() << "confirmar";
    confirmar(acbr, true);
    qDebug() << "desconhecer";
    desconhecer(acbr, true);
    qDebug() << "naoRealizar";
    naoRealizar(acbr, true);

    //----------------------------------------------------------

    qDebug() << "maintenance";
    SqlQuery queryMaintenance;

    if (not queryMaintenance.exec("UPDATE loja SET lastDistribuicao = NOW() WHERE idLoja = " + idLoja)) {
      throw RuntimeException("Erro guardando lastDistribuicao:" + queryMaintenance.lastError().text(), this);
    }
  }

  qDebug() << "end pesquisa: " << cnpjRaiz;
}

void WidgetNFeDistribuicao::confirmar(ACBr &acbr, const bool silent) {
  auto match = model.multiMatch({{"confirmar", true}});

  while (not match.isEmpty()) { // processar 20 linhas por vez
    const auto selection = match.mid(0, 20);

    if (not enviarEvento(acbr, "CONFIRMAÇÃO", selection)) { return; }

    match = model.multiMatch({{"confirmar", true}});
  }

  if (not silent) { qApp->enqueueInformation("Operação realizada com sucesso!", this); }
}

void WidgetNFeDistribuicao::desconhecer(ACBr &acbr, const bool silent) {
  auto match = model.multiMatch({{"desconhecer", true}});

  while (not match.isEmpty()) { // processar 20 linhas por vez
    const auto selection = match.mid(0, 20);

    if (not enviarEvento(acbr, "DESCONHECIMENTO", selection)) { return; }

    match = model.multiMatch({{"desconhecer", true}});
  }

  if (not silent) { qApp->enqueueInformation("Operação realizada com sucesso!", this); }
}

void WidgetNFeDistribuicao::naoRealizar(ACBr &acbr, const bool silent) {
  auto match = model.multiMatch({{"naoRealizar", true}});

  while (not match.isEmpty()) { // processar 20 linhas por vez
    const auto selection = match.mid(0, 20);

    if (not enviarEvento(acbr, "NÃO REALIZADA", selection)) { return; }

    match = model.multiMatch({{"naoRealizar", true}});
  }

  if (not silent) { qApp->enqueueInformation("Operação realizada com sucesso!", this); }
}

void WidgetNFeDistribuicao::darCiencia(ACBr &acbr, const bool silent) {
  auto match = model.multiMatch({{"ciencia", true}});

  while (not match.isEmpty()) { // processar 20 linhas por vez
    const auto selection = match.mid(0, 20);

    if (not enviarEvento(acbr, "CIÊNCIA", selection)) { return; }

    match = model.multiMatch({{"ciencia", true}});
  }

  if (not silent) { qApp->enqueueInformation("Operação realizada com sucesso!", this); }
}

void WidgetNFeDistribuicao::processarResposta(const QString &resposta, const QString &idLoja) {
  const QStringList eventos = resposta.split("\r\n\r\n", Qt::SkipEmptyParts);

  for (const auto &evento : eventos) {
    if (evento.contains("[DistribuicaoDFe]", Qt::CaseInsensitive)) { processarEventoPrincipal(evento, idLoja); }
    if (evento.contains("[ResDFe", Qt::CaseInsensitive)) { processarEventoNFe(evento); }
    if (evento.contains("[ResEve", Qt::CaseInsensitive)) { processarEventoInformacao(evento); }
  }
}

void WidgetNFeDistribuicao::on_pushButtonCiencia_clicked() {
  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!", this); }

  //----------------------------------------------------------

  for (const auto &index : selection) {
    const QString statusDistribuicao = model.data(index.row(), "statusDistribuicao").toString();

    if (statusDistribuicao == "CANCELADA") { continue; }

    model.setData(index.row(), "ciencia", true);
  }

  const auto monitorar = User::getSetting("User/monitorarNFe").toBool();

  ACBr acbr;

  monitorar ? darCiencia(acbr, false) : agendarOperacao();
}

void WidgetNFeDistribuicao::on_pushButtonConfirmacao_clicked() {
  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!", this); }

  //----------------------------------------------------------

  for (const auto &index : selection) {
    const QString statusDistribuicao = model.data(index.row(), "statusDistribuicao").toString();

    if (statusDistribuicao == "CANCELADA") { continue; }

    model.setData(index.row(), "confirmar", true);
  }

  const auto monitorar = User::getSetting("User/monitorarNFe").toBool();

  ACBr acbr;

  monitorar ? confirmar(acbr, false) : agendarOperacao();
}

void WidgetNFeDistribuicao::on_pushButtonDesconhecimento_clicked() {
  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!", this); }

  //----------------------------------------------------------

  for (const auto &index : selection) {
    const QString statusDistribuicao = model.data(index.row(), "statusDistribuicao").toString();

    if (statusDistribuicao == "CANCELADA") { continue; }

    model.setData(index.row(), "desconhecer", true);
  }

  const auto monitorar = User::getSetting("User/monitorarNFe").toBool();

  ACBr acbr;

  monitorar ? desconhecer(acbr, false) : agendarOperacao();
}

void WidgetNFeDistribuicao::on_pushButtonNaoRealizada_clicked() {
  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!", this); }

  //----------------------------------------------------------

  for (const auto &index : selection) {
    const QString statusDistribuicao = model.data(index.row(), "statusDistribuicao").toString();

    if (statusDistribuicao == "CANCELADA") { continue; }

    model.setData(index.row(), "naoRealizar", true);
  }

  const auto monitorar = User::getSetting("User/monitorarNFe").toBool();

  ACBr acbr;

  monitorar ? naoRealizar(acbr, false) : agendarOperacao();
}

void WidgetNFeDistribuicao::agendarOperacao() {
  qApp->startTransaction("NFeDistribuicao::agendarOperacao");

  model.submitAll();

  qApp->endTransaction();

  qApp->enqueueInformation("Operação agendada com sucesso!", this);
}

bool WidgetNFeDistribuicao::enviarEvento(ACBr &acbr, const QString &operacao, const QVector<int> &selection) {
  QString column;

  if (operacao == "CIÊNCIA") { column = "ciencia"; }
  if (operacao == "CONFIRMAÇÃO") { column = "confirmar"; }
  if (operacao == "DESCONHECIMENTO") { column = "desconhecer"; }
  if (operacao == "NÃO REALIZADA") { column = "naoRealizar"; }

  if (column.isEmpty()) { return false; }

  QString codigoEvento;

  if (operacao == "CIÊNCIA") { codigoEvento = "210210"; }
  if (operacao == "CONFIRMAÇÃO") { codigoEvento = "210200"; }
  if (operacao == "DESCONHECIMENTO") { codigoEvento = "210220"; }
  if (operacao == "NÃO REALIZADA") { codigoEvento = "210240"; }

  if (codigoEvento.isEmpty()) { return false; }

  //----------------------------------------------------------

  const QString dataHora = qApp->serverDateTime().toString("dd/MM/yyyy HH:mm");

  QString justificativa;

  if (operacao == "NÃO REALIZADA") {
    justificativa = QInputDialog::getText(this, "Justificativa", "Entre 15 e 255 caracteres: ");
    if (justificativa.size() < 15 or justificativa.size() > 255) { throw RuntimeError("Justificativa fora do tamanho!", this); }
  }

  QString comando;
  comando += R"(NFE.EnviarEvento("[EVENTO])" + QString("\r\n");
  comando += "idLote = 1\r\n";

  int count = 0;

  for (const auto row : selection) {
    const QString statusDistribuicao = model.data(row, "statusDistribuicao").toString();

    if (statusDistribuicao == "CANCELADA") { continue; }

    if (operacao == "CIÊNCIA" and statusDistribuicao != "DESCONHECIDO") { continue; }

    const QString chaveAcesso = model.data(row, "chaveAcesso").toString();
    const QString numEvento = QString::number(++count).rightJustified(3, '0'); // padding with zeros

    comando += "[EVENTO" + numEvento + "]\r\n";
    comando += "cOrgao = 91\r\n";
    comando += "CNPJ = " + cnpjDest + "\r\n";
    comando += "chNFe = " + chaveAcesso + "\r\n";
    comando += "dhEvento = " + dataHora + "\r\n";
    comando += "tpEvento = " + codigoEvento + "\r\n";
    comando += "nSeqEvento = 1\r\n";
    comando += "versaoEvento = 1.00\r\n";
    if (operacao == "NÃO REALIZADA") { comando += "xJust = " + justificativa + "\r\n"; }
  }

  comando += R"("))";

  //----------------------------------------------------------

  const QString resposta = acbr.enviarComando(comando, "Enviando evento de " + operacao.toLower() + "...");

  if (resposta.contains("ERRO: ", Qt::CaseInsensitive)) { throw RuntimeException(resposta, this); }

  //----------------------------------------------------------

  const QStringList eventos = resposta.split("\r\n\r\n", Qt::SkipEmptyParts);

  qApp->startTransaction("NFeDistribuicao::enviarEvento_" + operacao);

  for (const auto &evento : eventos) {
    const QString motivo = qApp->findTag(evento, "XMotivo=");

    if (motivo == "Lote de evento processado") { continue; }

    //----------------------------------------------------------

    const QString chaveAcesso = qApp->findTag(evento, "chNFe=");

    //----------------------------------------------------------

    if (motivo.contains("Evento de Ciencia da Operacao informado apos a manifestacao final do destinatario", Qt::CaseInsensitive)) {
      SqlQuery query;

      if (not query.exec("UPDATE nfe SET statusDistribuicao = 'FINALIZADA', dataDistribuicao = NOW(), "
                         "ciencia = FALSE, confirmar = FALSE, desconhecer = FALSE, naoRealizar = FALSE "
                         "WHERE chaveAcesso = '" +
                         chaveAcesso + "'")) {
        throw RuntimeException("Erro atualizando statusDistribuicao da NF-e: " + query.lastError().text());
      }

      continue;
    }

    if (motivo.contains("Rejeicao:", Qt::CaseInsensitive) and motivo.contains("para NFe cancelada ou denegada", Qt::CaseInsensitive)) {
      SqlQuery query;

      if (not query.exec("UPDATE nfe SET status = 'CANCELADA', statusDistribuicao = 'CANCELADA', "
                         "ciencia = FALSE, confirmar = FALSE, desconhecer = FALSE, naoRealizar = FALSE "
                         "WHERE chaveAcesso = '" +
                         chaveAcesso + "'")) {
        throw RuntimeException("Erro atualizando statusDistribuicao da NF-e: " + query.lastError().text());
      }

      continue;
    }

    if (motivo.contains("Evento registrado e vinculado a NF-e", Qt::CaseInsensitive) or motivo.contains("Rejeicao: Duplicidade de evento", Qt::CaseInsensitive)) {
      SqlQuery query;

      if (not query.exec("UPDATE nfe SET statusDistribuicao = '" + operacao +
                         "', dataDistribuicao = NOW(), ciencia = FALSE, confirmar = FALSE, desconhecer = FALSE, naoRealizar = FALSE "
                         "WHERE chaveAcesso = '" +
                         chaveAcesso + "'")) {
        throw RuntimeException("Erro atualizando statusDistribuicao da NF-e: " + query.lastError().text());
      }

      continue;
    }

    // TODO: ao ocorrer um evento não tratado limpar os flags de eventos no banco de dados para evitar que o sistema fique enviando o mesmo evento repetidamente

    throw RuntimeException("Evento não tratado: " + evento);
  }

  model.select();

  qApp->endTransaction();

  return true;
}

void WidgetNFeDistribuicao::on_table_activated(const QModelIndex &index) {
  SqlQuery query;
  query.prepare("SELECT xml FROM nfe WHERE idNFe = :idNFe");
  query.bindValue(":idNFe", model.data(index.row(), "idNFe"));

  if (not query.exec()) { throw RuntimeException("Erro buscando XML da NF-e: " + query.lastError().text(), this); }

  if (not query.first()) { throw RuntimeException("Não encontrado XML da NF-e com id: '" + model.data(index.row(), "idNFe").toString() + "'", this); }

  ACBrLib::gerarDanfe(query.value("xml"), true);
}

QString WidgetNFeDistribuicao::encontraInfCpl(const QString &xml) {
  const int indexInfCpl1 = xml.indexOf("<infCpl>");
  const int indexInfCpl2 = xml.indexOf("</infCpl>");

  if (indexInfCpl1 != -1 and indexInfCpl2 != -1) { return xml.mid(indexInfCpl1, indexInfCpl2 - indexInfCpl1).remove("<infCpl>"); }

  return "";
}

QString WidgetNFeDistribuicao::encontraTransportadora(const QString &xml) {
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

void WidgetNFeDistribuicao::montaFiltro() {
  ajustarGroupBoxStatus();

  //-------------------------------------

  QStringList filtros;

  //------------------------------------- filtro status

  QStringList filtroCheck;

  const auto children = ui->groupBoxStatus->findChildren<QCheckBox *>(QRegularExpression("checkBox"));

  for (const auto &child : children) {
    if (child->isChecked()) { filtroCheck << "'" + child->text().toUpper() + "'"; }
  }

  if (not filtroCheck.isEmpty()) { filtros << "statusDistribuicao IN (" + filtroCheck.join(", ") + ")"; }

  //------------------------------------- filtro loja

  if (ui->groupBoxLoja->isChecked()) {
    const QString lojaNome = ui->itemBoxLoja->text();
    const QString idLoja = ui->itemBoxLoja->getId().toString();

    if (not lojaNome.isEmpty()) {
      QSqlQuery queryLoja;

      if (not queryLoja.exec("SELECT cnpj FROM loja WHERE idLoja = " + idLoja)) { throw RuntimeException("Erro buscando CNPJ loja: " + queryLoja.lastError().text()); }

      if (not queryLoja.first()) { throw RuntimeException("Dados não encontrados para loja com id: '" + idLoja + "'"); }

      filtros << "cnpjDest = '" + queryLoja.value("cnpj").toString().remove(".").remove("/").remove("-") + "'";
    }
  }

  //-------------------------------------

  model.setFilter(filtros.join(" AND "));
}

void WidgetNFeDistribuicao::on_groupBoxStatus_toggled(const bool enabled) {
  unsetConnections();

  try {
    const auto children = ui->groupBoxStatus->findChildren<QCheckBox *>(QRegularExpression("checkBox"));

    for (const auto &child : children) {
      child->setEnabled(true);
      child->setChecked(enabled);
    }
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();

  montaFiltro();
}

void WidgetNFeDistribuicao::processarEventoPrincipal(const QString &evento, const QString &idLoja) {
  const QString ultNSU = qApp->findTag(evento, "ultNSU=");
  const QString maxNSU = qApp->findTag(evento, "maxNSU=");

  ultimoNSU = ultNSU.toInt();
  maximoNSU = maxNSU.toInt();

  //----------------------------------------------------------

  SqlQuery queryLoja;

  if (not queryLoja.exec("UPDATE loja SET ultimoNSU = " + ultNSU + ", maximoNSU = " + maxNSU + " WHERE idLoja = " + idLoja)) {
    throw RuntimeException("Erro guardando NSU: " + queryLoja.lastError().text());
  }
}

void WidgetNFeDistribuicao::processarEventoNFe(const QString &evento) {
  const QString chaveAcesso = qApp->findTag(evento, "chDFe=");
  const QString numeroNFe = chaveAcesso.mid(25, 9);
  const QString cnpjOrig = qApp->findTag(evento, "CNPJCPF=");
  const QString dataHoraEmissaoString = qApp->findTag(evento, "dhEmi=");
  const QDateTime dataHoraEmissao = QDateTime::fromString(dataHoraEmissaoString, "dd/MM/yyyy hh:mm:ss");
  const QString nomeEmitente = qApp->findTag(evento, "xNome=");
  const QString valor = qApp->findTag(evento, "vNF=").replace(',', '.');
  const QString nsu = qApp->findTag(evento, "NSU=");
  const QString xml = qApp->findTag(evento, "XML=");
  const QString schemaEvento = qApp->findTag(evento, "schema=");

  //----------------------------------------------------------

  SqlQuery queryNFe;

  if (not queryNFe.exec("SELECT status FROM nfe WHERE chaveAcesso = '" + chaveAcesso + "'")) { throw RuntimeException("Erro verificando se NF-e já cadastrada: " + queryNFe.lastError().text()); }

  const bool cadastrado = queryNFe.first();

  //----------------------------------------------------------

  if (not cadastrado) {
    const QString status = (schemaEvento == "procNFe") ? "AUTORIZADA" : "RESUMO";
    const QString ciencia = (schemaEvento == "procNFe") ? "0" : "1";

    SqlQuery queryCadastrar;
    queryCadastrar.prepare(
        "INSERT INTO nfe (numeroNFe, tipo, xml, status, dataHoraEmissao, emitente, cnpjOrig, cnpjDest, chaveAcesso, transportadora, valor, infCpl, nsu, statusDistribuicao, ciencia) VALUES "
        "(:numeroNFe, 'ENTRADA', :xml, :status, :dataHoraEmissao, :emitente, :cnpjOrig, :cnpjDest, :chaveAcesso, :transportadora, :valor, :infCpl, :nsu, 'DESCONHECIDO', :ciencia)");
    queryCadastrar.bindValue(":numeroNFe", numeroNFe);
    queryCadastrar.bindValue(":xml", xml);
    queryCadastrar.bindValue(":status", status);
    queryCadastrar.bindValue(":dataHoraEmissao", dataHoraEmissao);
    queryCadastrar.bindValue(":emitente", nomeEmitente);
    queryCadastrar.bindValue(":cnpjOrig", cnpjOrig);
    queryCadastrar.bindValue(":cnpjDest", cnpjDest);
    queryCadastrar.bindValue(":chaveAcesso", chaveAcesso);
    queryCadastrar.bindValue(":transportadora", encontraTransportadora(xml));
    queryCadastrar.bindValue(":valor", valor);
    queryCadastrar.bindValue(":infCpl", encontraInfCpl(xml));
    queryCadastrar.bindValue(":nsu", nsu);
    queryCadastrar.bindValue(":ciencia", ciencia);

    if (not queryCadastrar.exec()) { throw RuntimeException("Erro cadastrando resumo da NF-e: " + queryCadastrar.lastError().text()); }

    return;
  }

  // TODO: condition is always true
  if (cadastrado) {
    const QString statusCadastrado = queryNFe.value("status").toString();

    if (schemaEvento == "resNFe" or statusCadastrado == "AUTORIZADA") { return; }

    SqlQuery queryAtualizar;
    queryAtualizar.prepare("UPDATE nfe SET xml = :xml, status = 'AUTORIZADA', transportadora = :transportadora, infCpl = :infCpl, nsu = :nsu WHERE chaveAcesso = :chaveAcesso AND status = 'RESUMO'");
    queryAtualizar.bindValue(":xml", xml);
    queryAtualizar.bindValue(":transportadora", encontraTransportadora(xml));
    queryAtualizar.bindValue(":infCpl", encontraInfCpl(xml));
    queryAtualizar.bindValue(":nsu", nsu);
    queryAtualizar.bindValue(":chaveAcesso", chaveAcesso);

    if (not queryAtualizar.exec()) { throw RuntimeException("Erro atualizando xml: " + queryAtualizar.lastError().text()); }

    return;
  }

  // TODO: unreachable code
  throw RuntimeException("Evento de NF-e não tratado: " + evento);
}

void WidgetNFeDistribuicao::processarEventoInformacao(const QString &evento) {
  if (evento.contains("Comprovante de Entrega do CT-e", Qt::CaseInsensitive)) { return; }

  //----------------------------------------------------------

  const QString eventoTipo = qApp->findTag(evento, "xEvento=");
  const QString motivo = qApp->findTag(evento, "xMotivo=");
  const QString chaveAcesso = qApp->findTag(evento, "chDFe=");

  //----------------------------------------------------------

  SqlQuery queryNFe;

  if (not queryNFe.exec("SELECT nsu, statusDistribuicao FROM nfe WHERE chaveAcesso = '" + chaveAcesso + "'")) {
    throw RuntimeException("Erro verificando se NF-e já cadastrada: " + queryNFe.lastError().text());
  }

  const bool cadastrado = queryNFe.first();

  if (cadastrado) {
    const int nsuCadastrado = queryNFe.value("nsu").toInt();
    const QString statusCadastrado = queryNFe.value("statusDistribuicao").toString();

    if (nsuCadastrado == 0) { return; } // se NF-e foi importada manualmente não fazer eventos de ciência

    if (motivo.contains("Evento registrado e vinculado a NF-e", Qt::CaseInsensitive)) {
      if (eventoTipo == "Ciencia da Operacao" and statusCadastrado == "DESCONHECIDO") {
        SqlQuery queryAtualiza;

        if (not queryAtualiza.exec("UPDATE nfe SET statusDistribuicao = 'CIÊNCIA', ciencia = FALSE, confirmar = FALSE, desconhecer = FALSE, naoRealizar = FALSE WHERE chaveAcesso = '" + chaveAcesso +
                                   "'")) {
          throw RuntimeException("Erro atualizando statusDistribuicao: " + queryAtualiza.lastError().text());
        }

        return;
      }

      if (eventoTipo == "Confirmacao da Operacao" and statusCadastrado != "CONFIRMAÇÃO") {
        SqlQuery queryAtualiza;

        if (not queryAtualiza.exec("UPDATE nfe SET statusDistribuicao = 'CONFIRMAÇÃO', ciencia = FALSE, confirmar = FALSE, desconhecer = FALSE, naoRealizar = FALSE WHERE chaveAcesso = '" +
                                   chaveAcesso + "'")) {
          throw RuntimeException("Erro atualizando statusDistribuicao: " + queryAtualiza.lastError().text());
        }

        return;
      }

      if (eventoTipo == "CANCELAMENTO" and statusCadastrado != "CANCELADA") {
        SqlQuery queryAtualiza;

        if (not queryAtualiza.exec(
                "UPDATE nfe SET status = 'CANCELADA', statusDistribuicao = 'CANCELADA', ciencia = FALSE, confirmar = FALSE, desconhecer = FALSE, naoRealizar = FALSE WHERE chaveAcesso = '" +
                chaveAcesso + "'")) {
          throw RuntimeException("Erro atualizando statusDistribuicao: " + queryAtualiza.lastError().text());
        }

        return;
      }

      // TODO: implement
      //        if (eventoTipo == "DESCONHECIMENTO" and statusDistribuicao == "DESCONHECIDO") {}
      //        if (eventoTipo == "NÃO REALIZADA" and statusDistribuicao == "DESCONHECIDO") {}

      throw RuntimeException("Evento de informação não tratado: " + evento);
    }
  }
}

bool WidgetNFeDistribuicao::houveConsultaEmOutroPc() {
  QSqlQuery query;

  if (not query.exec("SELECT lastDistribuicao IS NULL, timestampdiff(SECOND, lastDistribuicao, NOW()) / 60 AS tempo FROM loja WHERE idLoja = " + idLoja)) {
    throw RuntimeException("Erro buscando última consulta: " + query.lastError().text(), this);
  }

  if (not query.first()) { throw RuntimeException("Última consulta não encontrada!", this); }

  if (query.value("lastDistribuicao IS NULL").toBool()) { return false; }

  return query.value("tempo").toInt() < 60;
}

void WidgetNFeDistribuicao::ajustarGroupBoxStatus() {
  bool empty = true;
  auto filtrosStatus = ui->groupBoxStatus->findChildren<QCheckBox *>();

  for (auto *checkBox : filtrosStatus) {
    if (checkBox->isChecked()) { empty = false; }
  }

  unsetConnections();

  ui->groupBoxStatus->setChecked(not empty);

  for (auto *checkBox : filtrosStatus) { checkBox->setEnabled(true); }

  setConnections();
}

void WidgetNFeDistribuicao::consultarSefaz() {
  SqlQuery query;

  if (not query.exec("SELECT monitorarCNPJ1, monitorarServidor1, monitorarPorta1, monitorarCNPJ2, monitorarServidor2, monitorarPorta2 FROM config")) {
    throw RuntimeException("Erro buscando dados do monitor de NF-e: " + query.lastError().text());
  }

  if (not query.first()) { throw RuntimeError("Dados não configurados para o monitor de NF-e!"); }

  const QString cnpj1 = query.value("monitorarCNPJ1").toString();
  const QString cnpj2 = query.value("monitorarCNPJ2").toString();
  const QString servidor1 = query.value("monitorarServidor1").toString();
  const QString servidor2 = query.value("monitorarServidor2").toString();
  const QString porta1 = query.value("monitorarPorta1").toString();
  const QString porta2 = query.value("monitorarPorta2").toString();

  if (not cnpj1.isEmpty()) { buscarNFes(cnpj1, servidor1, porta1); }
  if (not cnpj2.isEmpty()) { buscarNFes(cnpj2, servidor2, porta2); }

  qDebug() << "maintenance";
  SqlQuery queryMaintenance;

  if (not queryMaintenance.exec("UPDATE maintenance SET lastDistribuicao = NOW()")) { throw RuntimeException("Erro guardando lastDistribuicao:" + queryMaintenance.lastError().text(), this); }
}

// TODO: nos casos em que o usuario importar um xml já cadastrado como RESUMO utilizar o xml do usuario
// TODO: lidar com mais de um pc tentando baixar nfes (usar maintenance.lastDistribuicao)
// TODO: autoconfirmar nfes com mais de x dias para evitar perder o prazo e ser multado
// TODO: substituir itemBoxLoja por comboBoxLoja (copiar de widgetOrcamento)
// TODO: quando clicar em uma coluna para ordenar na tabela em vez de ordenar localmente pedir para o banco de dados ordenar para ser rápido
// TODO: criar um cnpj/cpf delegate para formatar o valor na tabela
// TODO: colocar legenda de cor explicando o que significa cada cor

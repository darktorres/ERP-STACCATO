#include "widgetnfedistribuicao.h"
#include "ui_widgetnfedistribuicao.h"

#include "acbrlib.h"
#include "application.h"
#include "log.h"
#include "nfeproxymodel.h"
#include "reaisdelegate.h"
#include "user.h"

#include <QInputDialog>
#include <QSqlError>
#include <QTimer>

using namespace std::chrono_literals;

WidgetNFeDistribuicao::WidgetNFeDistribuicao(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetNFeDistribuicao) {
  ui->setupUi(this);

  connect(&timer, &QTimer::timeout, this, &WidgetNFeDistribuicao::downloadAutomatico);
  timer.setTimerType(Qt::VeryCoarseTimer);

  // Connect to application's connection status to detect DB reconnection
  connect(qApp, &Application::setConnectionStatus, this, [this](bool connected) {
    if (connected) {
      resetTables();
      timer.stop();
      timer.start(10s);
    }
  });

  // Inicializar com 0 minutos - será recalculado dinamicamente
  timer.start(0min);
}

WidgetNFeDistribuicao::~WidgetNFeDistribuicao() { delete ui; }

void WidgetNFeDistribuicao::downloadAutomatico() {
  if (not User::getSetting("User/monitorarNFe").toBool()) { return; }

  timer.stop();

  // TODO: is this still needed?
  updateTables();

  try {
    // Verificar se há alguma consulta disponível agora
    QSqlQuery queryDisponivel;
    if (not queryDisponivel.exec("SELECT COUNT(*) as consultas FROM loja l "
                                "JOIN config c ON (l.cnpj LIKE CONCAT(c.monitorarCNPJ1, '%') OR l.cnpj LIKE CONCAT(c.monitorarCNPJ2, '%')) "
                                "WHERE l.desativado = FALSE "
                                "AND (c.monitorarCNPJ1 IS NOT NULL OR c.monitorarCNPJ2 IS NOT NULL) "
                                "AND (l.proximaConsultaPermitida IS NULL OR l.proximaConsultaPermitida <= NOW())")) {
      throw RuntimeException("Erro verificando consultas disponíveis: " + queryDisponivel.lastError().text(), this);
    }

    queryDisponivel.first();
    int consultasDisponiveis = queryDisponivel.value("consultas").toInt();

    if (consultasDisponiveis > 0) {
      qDebug() << "download Automatico - executando consultas (" << consultasDisponiveis << " disponíveis)";
      qApp->setSilent(true);
      consultarSefaz();
      qApp->setSilent(false);

      // Recalcular próximo timer baseado no resultado
      int proximosMinutos = calcularMinutosProximaConsultaGeral();
      qDebug() << "Próximas consultas disponíveis em" << proximosMinutos << "minutos";
      timer.start(std::chrono::minutes(proximosMinutos));
    } else {
      // Nenhuma consulta disponível, calcular quando será a próxima
      int proximosMinutos = calcularMinutosProximaConsultaGeral();
      qDebug() << "download Automatico - nenhuma consulta disponível, reagendando para" << proximosMinutos << "minutos";
      timer.start(std::chrono::minutes(proximosMinutos));
    }
  } catch (std::exception &) {
    qApp->setSilent(false);
    // Em caso de erro, tentar novamente em 5 minutos
    timer.start(5min);
    throw;
  }
}

void WidgetNFeDistribuicao::resetTables() {
  setupTables();
  montaFiltro();
}

void WidgetNFeDistribuicao::updateTables() {
  if (not isSet) {
    ui->itemBoxLoja->setSearchDialog(SearchDialog::loja(this));
    setupTables();
    montaFiltro();
    setConnections();
    isSet = true;
  }

  // Só atualizar a tabela se não estiver em modo silent (download automático)
  // if (!qApp->isSilent()) {
    model.select();
  // }
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
  connect(ui->pushButtonBaixarNFe, &QPushButton::clicked, this, &WidgetNFeDistribuicao::on_pushButtonBaixarNFe_clicked, connectionType);
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
  disconnect(ui->pushButtonBaixarNFe, &QPushButton::clicked, this, &WidgetNFeDistribuicao::on_pushButtonBaixarNFe_clicked);
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
  
  // Desabilitar autoResize para melhor performance
  ui->table->setAutoResize(false);
}

void WidgetNFeDistribuicao::on_pushButtonBaixarNFe_clicked() {
  timer.stop();

  try {
    consultarSefaz();
    qApp->enqueueInformation("Operação realizada com sucesso!", this);

    // Recalcular próximo timer baseado no resultado das consultas
    int proximosMinutos = calcularMinutosProximaConsultaGeral();
    qDebug() << "Consulta manual concluída - próximas consultas em" << proximosMinutos << "minutos";
    timer.start(std::chrono::minutes(proximosMinutos));
  } catch (std::exception &) {
    // Em caso de erro, tentar novamente em 5 minutos
    timer.start(5min);
    throw;
  }
}

bool WidgetNFeDistribuicao::enviarComando(ACBr &acbr) {
  qDebug() << "pesquisar cnpj - nsu: " << cnpjDest << " - " << ultimoNSU;

  // TODO: parametrizar o código do estado em vez de usar 35
  const QString comando = R"(NFe.DistribuicaoDFePorUltNSU("35", ")" + cnpjDest + R"(", )" + QString::number(ultimoNSU) + ")";
  const QString resposta = acbr.enviarComando(comando, "Consultando NF-es do CNPJ " + cnpjDest + "...");
  qDebug() << "resposta: " << resposta;
  // TODO: se essas mensagens são silenciosas como o usuario vai arrumar quando der erro?

  if (resposta.contains("Consumo Indevido", Qt::CaseInsensitive)) {
    qDebug() << "CONSUMO INDEVIDO para CNPJ" << cnpjDest << "- definindo próxima consulta para 1h";

    // Definir próxima consulta permitida para 1h no futuro
    SqlQuery queryUpdate;
    if (not queryUpdate.exec("UPDATE loja SET proximaConsultaPermitida = DATE_ADD(NOW(), INTERVAL 1 HOUR), ultimaConsultaNSU = NOW() WHERE idLoja = " + idLoja)) {
      throw RuntimeException("Erro atualizando próxima consulta: " + queryUpdate.lastError().text());
    }

    qDebug() << "Próxima consulta permitida em 1h para loja" << idLoja << "- pulando para próximo CNPJ";
    return false; // Para este CNPJ, mas continua outros
  }

  if (resposta.contains("ERRO: Rejeicao: CNPJ-Base consultado difere do CNPJ-Base do Certificado Digital")) { throw RuntimeError("Certificado plugado é de outro CNPJ!"); }

  // Detectar dessincronização de NSU com SEFAZ
  if (resposta.contains("Deve ser utilizado o ultNSU nas solicitacoes subsequentes", Qt::CaseInsensitive)) {
    qDebug() << "DESSINCRONIZAÇÃO DE NSU DETECTADA para CNPJ" << cnpjDest << "- resetando NSUs e definindo próxima consulta para 1h";

    // Resetar NSUs no banco e definir próxima consulta para 1h no futuro
    SqlQuery queryReset;
    if (not queryReset.exec("UPDATE loja SET ultimoNSU = 0, maximoNSU = 0, proximaConsultaPermitida = DATE_ADD(NOW(), INTERVAL 1 HOUR), ultimaConsultaNSU = NOW() WHERE idLoja = " + idLoja)) {
      throw RuntimeException("Erro resetando NSUs e definindo próxima consulta: " + queryReset.lastError().text());
    }

    qDebug() << "NSUs resetados e próxima consulta permitida em 1h para loja" << idLoja << "- pulando para próximo CNPJ";
    return false; // Para este CNPJ, mas continua outros
  }

  if (resposta.contains("ERRO: ", Qt::CaseInsensitive)) { throw RuntimeException(resposta, this); }

  //----------------------------------------------------------

  qApp->startTransaction("NFeDistribuicao::enviarComando");

  processarResposta(resposta, idLoja);

  qApp->endTransaction();

  // Só atualizar a tabela se não estiver em modo silent (download automático)
  // if (!qApp->isSilent()) {
    model.select();
  // }

  // Atualizar próxima consulta permitida baseado no resultado
  SqlQuery queryProximaConsulta;
  QString intervalo;

  if (resposta.contains("Nenhum documento localizado", Qt::CaseInsensitive)) {
    intervalo = "DATE_ADD(NOW(), INTERVAL 1 HOUR)";
    qDebug() << "Nenhum documento localizado para CNPJ" << cnpjDest << "- próxima consulta em 1h";
  } else {
    intervalo = "DATE_ADD(NOW(), INTERVAL 15 MINUTE)";
    qDebug() << "Documentos encontrados para CNPJ" << cnpjDest << "- próxima consulta em 15min";
  }

  if (not queryProximaConsulta.exec("UPDATE loja SET proximaConsultaPermitida = " + intervalo + ", ultimaConsultaNSU = NOW() WHERE idLoja = " + idLoja)) {
    throw RuntimeException("Erro definindo próxima consulta: " + queryProximaConsulta.lastError().text());
  }

  return true; // Sucesso
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

    if (!podeConsultarAgora()) { continue; }

    if (!enviarComando(acbr)) {
      qDebug() << "Erro na consulta do CNPJ" << cnpjDest << "- pulando para próximo";
      continue;
    }

    //----------------------------------------------------------

    while (ultimoNSU < maximoNSU) {
      qDebug() << "pesquisar novamente";
      if (!enviarComando(acbr)) {
        qDebug() << "Erro na consulta adicional do CNPJ" << cnpjDest << "- interrompendo loop";
        break;
      }
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

    if (not queryMaintenance.exec("UPDATE loja SET ultimaConsultaNSU = NOW() WHERE idLoja = " + idLoja)) {
      throw RuntimeException("Erro guardando ultimaConsultaNSU:" + queryMaintenance.lastError().text(), this);
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
  //  darCiencia(acbr, false);
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
  //  confirmar(acbr, false);
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
  //  desconhecer(acbr, false);
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
  //  naoRealizar(acbr, false);
}

void WidgetNFeDistribuicao::agendarOperacao() {
  qApp->startTransaction("NFeDistribuicao::agendarOperacao");

  model.submitAll();

  qApp->endTransaction();

  qApp->enqueueInformation("Operação agendada com sucesso!", this);
}

bool WidgetNFeDistribuicao::enviarEvento(ACBr &acbr, const QString &operacao, const QVector<int> &selection) {
  qDebug() << selection;

  if (cnpjDest.isEmpty()) {
    SqlQuery queryLoja;

    if (not queryLoja.exec("SELECT cnpj FROM loja WHERE idLoja = (SELECT lojaACBr FROM config)")) { throw RuntimeException("Erro buscando CNPJ da loja: " + queryLoja.lastError().text(), this); }

    if (not queryLoja.first()) { throw RuntimeException("Dados não encontrados para loja com id: '" + idLoja + "'", this); }

    cnpjDest = queryLoja.value("cnpj").toString().remove(".").remove("/").remove("-");
  }

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
    const QString chaveAcesso = model.data(row, "chaveAcesso").toString();

    if (statusDistribuicao == "CANCELADA" or (operacao == "CIÊNCIA" and statusDistribuicao != "DESCONHECIDO")) {
      SqlQuery query;

      if (not query.exec("UPDATE nfe SET ciencia = -1, confirmar = -1, desconhecer = -1, naoRealizar = -1 "
                         "WHERE chaveAcesso = '" +
                         chaveAcesso + "'")) {
        throw RuntimeException("Erro atualizando statusDistribuicao da NF-e: " + query.lastError().text());
      }

      continue;
    }

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

    qDebug() << "motivo: " << motivo;

    //----------------------------------------------------------

    const QString chaveAcesso = qApp->findTag(evento, "chNFe=");

    //----------------------------------------------------------

    if (motivo.contains("Evento de Ciencia da Operacao informado apos a manifestacao final do destinatario", Qt::CaseInsensitive)) {
      SqlQuery query;

      if (not query.exec("UPDATE nfe SET statusDistribuicao = 'FINALIZADA', dataDistribuicao = NOW(), "
                         "ciencia = -1, confirmar = -1, desconhecer = -1, naoRealizar = -1 "
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

    if (motivo.contains("Rejeicao: Evento apresentado apos o prazo permitido para o evento:")) {
      SqlQuery query;

      if (not query.exec("UPDATE nfe SET statusDistribuicao = 'FINALIZADA', dataDistribuicao = NOW(), ciencia = -1, confirmar = -1, desconhecer = -1, naoRealizar = -1 "
                         "WHERE chaveAcesso = '" +
                         chaveAcesso + "'")) {
        throw RuntimeException("Erro atualizando statusDistribuicao da NF-e: " + query.lastError().text());
      }

      continue;
    }

    // TODO: ao ocorrer um evento não tratado limpar os flags de eventos no banco de dados para evitar que o sistema fique enviando o mesmo evento repetidamente
    SqlQuery query;

    if (not query.exec("UPDATE nfe SET confirmar = -1, ciencia = -1, desconhecer = -1, naoRealizar = -1 "
                       "WHERE chaveAcesso = '" +
                       chaveAcesso + "'")) {
      throw RuntimeException("Erro atualizando statusDistribuicao da NF-e: " + query.lastError().text());
    }

    Log::createLog("Exceção", "Evento não tratado: \nChave: " + chaveAcesso + "\nEvento:" + evento + "\nMotivo: " + motivo);
    continue;
  }

  // Só atualizar a tabela se não estiver em modo silent (download automático)
  // if (!qApp->isSilent()) {
    model.select();
  // }

  qApp->endTransaction();

  return true;
}

void WidgetNFeDistribuicao::on_table_activated(const QModelIndex &index) {
  SqlQuery query;
  query.prepare("SELECT xml FROM nfe WHERE idNFe = :idNFe");
  query.bindValue(":idNFe", model.data(index.row(), "idNFe"));

  if (not query.exec()) { throw RuntimeException("Erro buscando XML da NF-e: " + query.lastError().text(), this); }

  if (not query.first()) { throw RuntimeException("Não encontrado XML da NF-e com id: '" + model.data(index.row(), "idNFe").toString() + "'", this); }

  ACBrLib::gerarDanfe(query.value("xml").toString(), true);
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
  const QString xml = qApp->findTag(evento, "XML=");

  // Extrair dhEmi diretamente do XML (sempre completo e confiável)
  QRegularExpression regex("<dhEmi>(.*?)</dhEmi>");
  QRegularExpressionMatch match = regex.match(xml);

  QDateTime dataHoraEmissao;
  if (match.hasMatch()) {
    const QString dhEmiISO = match.captured(1);
    qDebug() << "PROCESSAMENTO NFE - dhEmi do XML:" << dhEmiISO;

    // Converter formato ISO para QDateTime
    dataHoraEmissao = QDateTime::fromString(dhEmiISO, Qt::ISODate);
  }

  qDebug() << "PROCESSAMENTO NFE - dataHoraEmissao final:" << dataHoraEmissao << "valido:" << dataHoraEmissao.isValid();
  const QString nomeEmitente = qApp->findTag(evento, "xNome=");
  const QString valor = qApp->findTag(evento, "vNF=").replace(',', '.');
  const QString nsu = qApp->findTag(evento, "NSU=");
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
  } else {
    const QString statusCadastrado = queryNFe.value("status").toString();

    if (schemaEvento == "resNFe" or statusCadastrado == "AUTORIZADA") { return; }

    SqlQuery queryAtualizar;
    queryAtualizar.prepare("UPDATE nfe SET xml = :xml, status = 'AUTORIZADA', transportadora = :transportadora, infCpl = :infCpl, nsu = :nsu WHERE chaveAcesso = :chaveAcesso AND status = 'RESUMO'");
    queryAtualizar.bindValue(":xml", xml);
    queryAtualizar.bindValue(":transportadora", encontraTransportadora(xml));
    queryAtualizar.bindValue(":infCpl", encontraInfCpl(xml));
    queryAtualizar.bindValue(":nsu", nsu);
    queryAtualizar.bindValue(":chaveAcesso", chaveAcesso);

    if (not queryAtualizar.exec()) { throw RuntimeException("Erro atualizando XML: " + queryAtualizar.lastError().text()); }
  }
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

        if (not queryAtualiza.exec("UPDATE nfe SET statusDistribuicao = 'CIÊNCIA', ciencia = FALSE, confirmar = FALSE, desconhecer = FALSE, naoRealizar = FALSE "
                                   "WHERE chaveAcesso = '" +
                                   chaveAcesso + "'")) {
          throw RuntimeException("Erro atualizando statusDistribuicao: " + queryAtualiza.lastError().text());
        }

        return;
      }

      if (eventoTipo == "Confirmacao da Operacao" and statusCadastrado != "CONFIRMAÇÃO") {
        SqlQuery queryAtualiza;

        if (not queryAtualiza.exec("UPDATE nfe SET statusDistribuicao = 'CONFIRMAÇÃO', ciencia = -1, confirmar = -1, desconhecer = -1, naoRealizar = -1 "
                                   "WHERE chaveAcesso = '" +
                                   chaveAcesso + "'")) {
          throw RuntimeException("Erro atualizando statusDistribuicao: " + queryAtualiza.lastError().text());
        }

        return;
      }

      if (eventoTipo == "CANCELAMENTO" and statusCadastrado != "CANCELADA") {
        SqlQuery queryAtualiza;

        if (not queryAtualiza.exec("UPDATE nfe SET status = 'CANCELADA', statusDistribuicao = 'CANCELADA', ciencia = -1, confirmar = -1, desconhecer = -1, naoRealizar = -1 "
                                   "WHERE chaveAcesso = '" +
                                   chaveAcesso + "'")) {
          throw RuntimeException("Erro atualizando statusDistribuicao: " + queryAtualiza.lastError().text());
        }

        return;
      }

      // TODO: implement
      //        if (eventoTipo == "DESCONHECIMENTO" and statusDistribuicao == "DESCONHECIDO") {}
      //        if (eventoTipo == "NÃO REALIZADA" and statusDistribuicao == "DESCONHECIDO") {}

      Log::createLog("Exceção", "Evento de informação não tratado: " + evento);
    }
  }
}

bool WidgetNFeDistribuicao::podeConsultarAgora() {
  QSqlQuery query;

  if (not query.exec("SELECT proximaConsultaPermitida IS NULL, "
                     "timestampdiff(SECOND, proximaConsultaPermitida, NOW()) AS segundosRestantes "
                     "FROM loja WHERE idLoja = " + idLoja)) {
    throw RuntimeException("Erro buscando próxima consulta permitida: " + query.lastError().text(), this);
  }

  if (not query.first()) { throw RuntimeException("Dados da loja não encontrados!", this); }

  // Se o campo é NULL, permite consultar (primeira vez)
  if (query.value("proximaConsultaPermitida IS NULL").toBool()) { return true; }

  // Se segundosRestantes >= 0, significa que chegou o horário permitido
  return query.value("segundosRestantes").toInt() >= 0;
}

int WidgetNFeDistribuicao::calcularMinutosProximaConsulta() {
  QSqlQuery query;

  if (not query.exec("SELECT GREATEST(0, CEIL(timestampdiff(SECOND, NOW(), proximaConsultaPermitida) / 60.0)) AS minutosRestantes "
                     "FROM loja WHERE idLoja = " + idLoja)) {
    throw RuntimeException("Erro calculando próxima consulta: " + query.lastError().text(), this);
  }

  if (not query.first()) { return 15; } // Default

  return qMax(1, query.value("minutosRestantes").toInt()); // Mínimo 1 minuto
}

int WidgetNFeDistribuicao::calcularMinutosProximaConsultaGeral() {
  QSqlQuery query;

  // Buscar o menor tempo até a próxima consulta de todas as lojas ativas dos CNPJs monitorados
  if (not query.exec("SELECT "
                     "MIN(GREATEST(0, CEIL(timestampdiff(SECOND, NOW(), proximaConsultaPermitida) / 60.0))) AS menorTempoRestante "
                     "FROM loja l "
                     "JOIN config c ON (l.cnpj LIKE CONCAT(c.monitorarCNPJ1, '%') OR l.cnpj LIKE CONCAT(c.monitorarCNPJ2, '%')) "
                     "WHERE l.desativado = FALSE AND (c.monitorarCNPJ1 IS NOT NULL OR c.monitorarCNPJ2 IS NOT NULL)")) {
    throw RuntimeException("Erro calculando próxima consulta geral: " + query.lastError().text(), this);
  }

  if (not query.first() || query.value("menorTempoRestante").isNull()) {
    return 15; // Default se não há dados
  }

  return qMax(1, query.value("menorTempoRestante").toInt()); // Mínimo 1 minuto
}

void WidgetNFeDistribuicao::ajustarGroupBoxStatus() {
  bool empty = true;
  auto filtrosStatus = ui->groupBoxStatus->findChildren<QCheckBox *>(QRegularExpression("checkBox"));

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
  if (not cnpj2.isEmpty() and cnpj2 != cnpj1) { buscarNFes(cnpj2, servidor2, porta2); }
}

// TODO: nos casos em que o usuario importar um xml já cadastrado como RESUMO utilizar o xml do usuario
// TODO: autoconfirmar nfes com mais de x dias para evitar perder o prazo e ser multado
// TODO: substituir itemBoxLoja por comboBoxLoja (copiar de widgetOrcamento)
// TODO: quando clicar em uma coluna para ordenar na tabela em vez de ordenar localmente pedir para o banco de dados ordenar para ser rápido
// TODO: criar um cnpj/cpf delegate para formatar o valor na tabela
// TODO: colocar legenda de cor explicando o que significa cada cor

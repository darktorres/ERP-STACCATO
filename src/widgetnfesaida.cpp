#include "widgetnfesaida.h"
#include "ui_widgetnfesaida.h"

#include "acbr.h"
#include "acbrlib.h"
#include "application.h"
#include "doubledelegate.h"
#include "file.h"
#include "followup.h"
#include "reaisdelegate.h"
#include "sqlquery.h"
#include "sqltablemodel.h"

#if __has_include("lrreportengine.h")
#include "lrreportengine.h"
#endif

#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QInputDialog>
#include <QMessageBox>
#include <QSqlError>

// sem filtro de Data, o JOIN contra o historico inteiro (~38 mil NF-e's) leva segundos; corta para
// as N mais recentes nesse caso (mesmo raciocinio/medicao da correção equivalente em Entrada)
constexpr int LIMITE_HISTORICO_SEM_DATA = 1000;

WidgetNfeSaida::WidgetNfeSaida(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetNfeSaida) { ui->setupUi(this); }

WidgetNfeSaida::~WidgetNfeSaida() { delete ui; }

void WidgetNfeSaida::setConnections() {
  if (not blockingSignals.isEmpty()) { blockingSignals.pop(); } // avoid crashing on first setConnections

  if (not blockingSignals.isEmpty()) { return; } // delay setting connections until last unset/set block

  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->checkBoxAutorizado, &QCheckBox::toggled, this, &WidgetNfeSaida::montaFiltro, connectionType);
  connect(ui->checkBoxCancelado, &QCheckBox::toggled, this, &WidgetNfeSaida::montaFiltro, connectionType);
  connect(ui->checkBoxDenegada, &QCheckBox::toggled, this, &WidgetNfeSaida::montaFiltro, connectionType);
  connect(ui->checkBoxPendente, &QCheckBox::toggled, this, &WidgetNfeSaida::montaFiltro, connectionType);
  connect(ui->dateEditAte, &QDateEdit::dateChanged, this, &WidgetNfeSaida::montaFiltro, connectionType);
  connect(ui->dateEditDe, &QDateEdit::dateChanged, this, &WidgetNfeSaida::on_dateEditDe_dateChanged, connectionType);
  connect(ui->groupBoxMes, &QGroupBox::toggled, this, &WidgetNfeSaida::montaFiltro, connectionType);
  connect(ui->groupBoxMes, &QGroupBox::toggled, this, &WidgetNfeSaida::on_groupBoxMes_toggled, connectionType);
  connect(ui->groupBoxStatus, &QGroupBox::toggled, this, &WidgetNfeSaida::on_groupBoxStatus_toggled, connectionType);
  connect(ui->lineEditBusca, &LineEdit::delayedTextChanged, this, &WidgetNfeSaida::montaFiltro, connectionType);
  connect(ui->pushButtonCancelarNFe, &QPushButton::clicked, this, &WidgetNfeSaida::on_pushButtonCancelarNFe_clicked, connectionType);
  connect(ui->pushButtonConsultarNFe, &QPushButton::clicked, this, &WidgetNfeSaida::on_pushButtonConsultarNFe_clicked, connectionType);
  connect(ui->pushButtonExportar, &QPushButton::clicked, this, &WidgetNfeSaida::on_pushButtonExportar_clicked, connectionType);
  connect(ui->pushButtonFollowup, &QPushButton::clicked, this, &WidgetNfeSaida::on_pushButtonFollowup_clicked, connectionType);
  connect(ui->pushButtonRelatorio, &QPushButton::clicked, this, &WidgetNfeSaida::on_pushButtonRelatorio_clicked, connectionType);
  connect(ui->table, &TableView::activated, this, &WidgetNfeSaida::on_table_activated, connectionType);
}

void WidgetNfeSaida::unsetConnections() {
  blockingSignals.push(0);

  disconnect(ui->checkBoxAutorizado, &QCheckBox::toggled, this, &WidgetNfeSaida::montaFiltro);
  disconnect(ui->checkBoxCancelado, &QCheckBox::toggled, this, &WidgetNfeSaida::montaFiltro);
  disconnect(ui->checkBoxDenegada, &QCheckBox::toggled, this, &WidgetNfeSaida::montaFiltro);
  disconnect(ui->checkBoxPendente, &QCheckBox::toggled, this, &WidgetNfeSaida::montaFiltro);
  disconnect(ui->dateEditAte, &QDateEdit::dateChanged, this, &WidgetNfeSaida::montaFiltro);
  disconnect(ui->dateEditDe, &QDateEdit::dateChanged, this, &WidgetNfeSaida::on_dateEditDe_dateChanged);
  disconnect(ui->groupBoxMes, &QGroupBox::toggled, this, &WidgetNfeSaida::montaFiltro);
  disconnect(ui->groupBoxMes, &QGroupBox::toggled, this, &WidgetNfeSaida::on_groupBoxMes_toggled);
  disconnect(ui->groupBoxStatus, &QGroupBox::toggled, this, &WidgetNfeSaida::on_groupBoxStatus_toggled);
  disconnect(ui->lineEditBusca, &LineEdit::delayedTextChanged, this, &WidgetNfeSaida::montaFiltro);
  disconnect(ui->pushButtonCancelarNFe, &QPushButton::clicked, this, &WidgetNfeSaida::on_pushButtonCancelarNFe_clicked);
  disconnect(ui->pushButtonConsultarNFe, &QPushButton::clicked, this, &WidgetNfeSaida::on_pushButtonConsultarNFe_clicked);
  disconnect(ui->pushButtonExportar, &QPushButton::clicked, this, &WidgetNfeSaida::on_pushButtonExportar_clicked);
  disconnect(ui->pushButtonFollowup, &QPushButton::clicked, this, &WidgetNfeSaida::on_pushButtonFollowup_clicked);
  disconnect(ui->pushButtonRelatorio, &QPushButton::clicked, this, &WidgetNfeSaida::on_pushButtonRelatorio_clicked);
  disconnect(ui->table, &TableView::activated, this, &WidgetNfeSaida::on_table_activated);
}

void WidgetNfeSaida::updateTables() {
  if (not isSet) {
    ui->lineEditBusca->setDelayed();
    ui->dateEditAte->setDate(qApp->serverDate());
    ui->dateEditDe->setDate(QDate(qApp->serverDate().year(), qApp->serverDate().month(), 1));
    setupTables();
    setConnections();
    isSet = true;
  }

  model.select();

  // ---------------------------------------------------

  modelResumo.select();

  const int fWidth = ui->tableResumo->frameWidth() * 2;

  const int vWidth = ui->tableResumo->verticalHeader()->width();
  const int hWidth = ui->tableResumo->horizontalHeader()->length();

  ui->tableResumo->setFixedWidth(vWidth + hWidth + fWidth);

  const int hHeight = ui->tableResumo->horizontalHeader()->height();
  const int vHeight = ui->tableResumo->verticalHeader()->length();

  ui->tableResumo->setFixedHeight(hHeight + vHeight + fWidth);
}

void WidgetNfeSaida::resetTables() { setupTables(); }

void WidgetNfeSaida::setupTables() {
  // TODO: mudar view para puxar apenas as NF-es com cnpjOrig igual a raiz da staccato

  montaFiltro(); // monta e executa a query base primeiro, para popular as colunas do model antes de configurar a tabela

  ui->table->setModel(&model);

  model.setHeaderData("valor", "R$");
  model.setHeaderData("dataHoraEmissao", "Data");
  model.setHeaderData("dataFollowup", "Data Followup");
  model.setHeaderData("observacao", "Observação");

  ui->table->hideColumn("idNFe");
  ui->table->hideColumn("chaveAcesso");

  ui->table->setItemDelegate(new DoubleDelegate(this));

  ui->table->setItemDelegateForColumn("valor", new ReaisDelegate(this));

  // ----------------------------------------------------

  modelResumo.setQuery("SELECT status AS Status, COUNT(*) AS `` FROM nfe n WHERE tipo = 'SAÍDA' GROUP BY status");

  ui->tableResumo->setModel(&modelResumo);

  ui->tableResumo->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  ui->tableResumo->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

void WidgetNfeSaida::on_table_activated(const QModelIndex &index) {
  const QString header = model.headerData(index.column(), Qt::Horizontal).toString();

  if (header == "Venda") { return qApp->abrirVenda(model.data(index.row(), "Venda")); }

  // -------------------------------------------------------------------------

  SqlQuery query;
  query.prepare("SELECT xml FROM nfe WHERE idNFe = :idNFe");
  query.bindValue(":idNFe", model.data(index.row(), "idNFe"));

  if (not query.exec()) { throw RuntimeException("Erro buscando XML da NF-e: " + query.lastError().text(), this); }

  if (not query.first()) { throw RuntimeException("XML não encontrado para NF-e com id: '" + model.data(index.row(), "idNFe").toString() + "'"); }

  ACBrLib::gerarDanfe(query.value("xml").toString(), true);
}

void WidgetNfeSaida::montaFiltro() {
  // ajustarGroupBoxStatus();

  //-------------------------------------

  QStringList filtrosPre; // aplicados em n.* antes do GROUP BY (permite usar indice em tipo/dataHoraEmissao)
  QStringList filtrosPos; // aplicados depois do GROUP BY (tocam colunas ligadas por join: CPF/CNPJ, Cliente)

  //------------------------------------- filtro texto

  const QString text = qApp->sanitizeSQL(ui->lineEditBusca->text());

  if (not text.isEmpty()) { filtrosPos << "(NFe LIKE '%" + text + "%' OR Venda LIKE '%" + text + "%' OR `CPF/CNPJ` LIKE '%" + text + "%' OR Cliente LIKE '%" + text + "%')"; }

  //------------------------------------- filtro data (comparacao direta na coluna, sem funcao, para poder usar indice)

  const bool temFiltroData = ui->groupBoxMes->isChecked();

  if (temFiltroData) {
    filtrosPre << "n.dataHoraEmissao >= '" + ui->dateEditDe->date().toString("yyyy-MM-dd") + " 00:00:00' AND n.dataHoraEmissao < '" + ui->dateEditAte->date().addDays(1).toString("yyyy-MM-dd") + " 00:00:00'";
  }

  //------------------------------------- filtro status

  QStringList filtroCheck;

  const auto children = ui->groupBoxStatus->findChildren<QCheckBox *>(QRegularExpression("checkBox"));

  for (const auto &child : children) {
    if (child->isChecked()) { filtroCheck << "'" + child->text().toUpper() + "'"; }
  }

  if (not filtroCheck.isEmpty()) { filtrosPre << "n.status IN (" + filtroCheck.join(", ") + ")"; }

  //------------------------------------- monta o FROM: direto em nfe quando ha filtro de data (ja rapido); limitado
  // as NF-e's mais recentes quando nao ha, pra nao ter que juntar o historico inteiro (o corte entra ANTES do
  // join, restrito só às colunas de n, pra poder usar o índice idx_nfe_tipo_data como range scan)

  QString fromClause;
  QStringList filtrosFinal;

  if (temFiltroData) {
    fromClause = "FROM nfe n";
    filtrosFinal << "n.tipo = 'SAÍDA'";
    filtrosFinal += filtrosPre;
  } else {
    fromClause = "FROM (SELECT idNFe FROM nfe n WHERE n.tipo = 'SAÍDA'" + (filtrosPre.isEmpty() ? "" : " AND " + filtrosPre.join(" AND ")) + " ORDER BY dataHoraEmissao DESC LIMIT " +
                 QString::number(LIMITE_HISTORICO_SEM_DATA) + ") lim JOIN nfe n ON n.idNFe = lim.idNFe";
  }

  const QString sql = "SELECT n.idNFe AS idNFe, n.chaveAcesso AS chaveAcesso, n.cnpjOrig AS Emitente, n.numeroNFe AS NFe, n.status AS Status, "
                      "n.idVenda AS Venda, IF(c.pfpj = 'PF', c.cpf, c.cnpj) AS `CPF/CNPJ`, c.nome_razao AS Cliente, "
                      "n.valor AS valor, n.dataHoraEmissao AS dataHoraEmissao, nhf.dataFollowup AS dataFollowup, nhf.observacao AS observacao " +
                      fromClause +
                      " LEFT JOIN venda v ON (n.idVenda = v.idVenda) "
                      "LEFT JOIN cliente c ON (c.idCliente = v.idCliente) "
                      "LEFT JOIN nfe_has_followup nhf ON (n.idFollowup = nhf.idFollowup)" +
                      (filtrosFinal.isEmpty() ? "" : " WHERE " + filtrosFinal.join(" AND ")) + " GROUP BY n.idNFe" +
                      (filtrosPos.isEmpty() ? "" : " HAVING " + filtrosPos.join(" AND ")) + " ORDER BY n.numeroNFe, n.emitente";

  model.setQuery(sql);
  model.select();

  ui->labelLimitado->setVisible(not temFiltroData);
}

void WidgetNfeSaida::on_pushButtonCancelarNFe_clicked() {
  // TODO: como no cancelamento o acbr nao atualiza o xml, fazer a consulta para atualizar o xml como cancelado antes de enviar para a contabilidade?

  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!", this); }

  // -------------------------------------------------------------------------

  // TODO: em vez de dizer para o usuário onde ir no menu, abrir direto a tela de UserConfig

  SqlQuery query;

  if (not query.exec("SELECT lojaACBr, emailContabilidade, emailLogistica FROM config")) { throw RuntimeException("Erro buscando dados do emissor de NF-e: " + query.lastError().text()); }

  if (not query.first()) { throw RuntimeError("Dados não configurados para o monitor de NF-e!"); }

  const QString emailContabilidade = query.value("emailContabilidade").toString();

  if (emailContabilidade.isEmpty()) { throw RuntimeError(R"("Email Contabilidade" não está configurado! Ajuste no menu "Opções->Configurações")", this); }

  const QString emailLogistica = query.value("emailLogistica").toString();

  if (emailLogistica.isEmpty()) { throw RuntimeError(R"("Email Logistica" não está configurado! Ajuste no menu "Opções->Configurações")", this); }

  // -------------------------------------------------------------------------

  QMessageBox msgBox(QMessageBox::Question, "Cancelar?", "Tem certeza que deseja cancelar?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.button(QMessageBox::Yes)->setText("Cancelar");
  msgBox.button(QMessageBox::No)->setText("Voltar");

  if (msgBox.exec() == QMessageBox::No) { return; }

  // -------------------------------------------------------------------------

  // TODO: adicionar e testar bool ok
  const QString justificativa = QInputDialog::getText(this, "Justificativa", "Entre 15 e 200 caracteres: ");

  if (justificativa.size() < 15 or justificativa.size() > 200) { throw RuntimeError("Justificativa fora do tamanho!", this); }

  // -------------------------------------------------------------------------

  const int row = selection.first().row();

  const QString chaveAcesso = model.data(row, "chaveAcesso").toString();

  ACBr acbr;

  const QString resposta = acbr.enviarComando("NFE.CancelarNFe(" + chaveAcesso + ", " + justificativa + ")", "Cancelando NF-e...");

  // TODO: verificar outras possiveis respostas (tinha algo como 'cancelamento registrado fora do prazo')
  if (not resposta.contains("xEvento=Cancelamento registrado", Qt::CaseInsensitive)) { throw RuntimeException("Resposta: " + resposta); }

  qApp->startTransaction("WidgetNfeSaida::on_pushButtonCancelarNFe");

  cancelarNFe(chaveAcesso, row);

  qApp->endTransaction();

  updateTables();

  const int xMotivoIndex = resposta.indexOf("XMotivo=", 0, Qt::CaseInsensitive);

  if (xMotivoIndex == -1) { throw RuntimeException("Não encontrou o campo 'xMotivo': " + resposta); }

  const QString xMotivo = resposta.mid(xMotivoIndex + 8).split("\r\n").first();

  qApp->enqueueInformation(xMotivo, this);

  gravarArquivo(resposta, chaveAcesso);

  // -------------------------------------------------------------------------

  //  const QString filePath = QDir::currentPath() + "/arquivos/cancelamento_" + chaveAcesso + ".xml";
  //  const QString assunto = "Cancelamento NF-e - " + modelViewNFeSaida.data(row, "NFe").toString() + " - STACCATO REVESTIMENTOS COMERCIO E REPRESENTACAO LTDA";

  // TODO: enviar o xml atualizado com o cancelamento
  // TODO: enviar a danfe

  //  ACBr acbr;
  //  acbr.enviarEmail(emailContabilidade, emailLogistica, assunto, filePath);
}

// TODO: 1verificar se ao cancelar nota ela é removida do venda_produto/veiculo_produto
// TODO: 1botao para gerar relatorio igual ao da receita

void WidgetNfeSaida::on_pushButtonRelatorio_clicked() {
#if __has_include("lrreportengine.h")
  // TODO: 3formatar decimais no padrao BR
  // TODO: 3perguntar um intervalo de tempo para filtrar as notas
  // TODO: 3verificar quais as tags na nota dos campos que faltam preencher

  if (not ui->groupBoxMes->isChecked()) { throw RuntimeError("Selecione um mês para gerar o relatório!", this); }

  const QString filename = QDir::currentPath() + "/arquivos/relatorio_nfe.pdf";

  File file(filename);

  if (not file.open(QFile::WriteOnly)) { throw RuntimeException("Erro abrindo arquivo para escrita xml: " + file.errorString()); }

  file.close();

  LimeReport::ReportEngine report;
  auto *dataManager = report.dataManager();

  SqlTableModel view;
  view.setTable("view_relatorio_nfe");

  // TODO: trocar 'Criado em' por 'DataEmissao'
  // comparacao direta na coluna (nao envolta em DATE_FORMAT) para poder usar indice (idx_nfe_tipo_status_created)
  view.setFilter("`Criado em` >= '" + ui->dateEditDe->date().toString("yyyy-MM-dd") + " 00:00:00' AND `Criado em` < '" + ui->dateEditAte->date().addDays(1).toString("yyyy-MM-dd") +
                 " 00:00:00' AND (status = 'AUTORIZADA')");

  view.select();

  dataManager->addModel("view", &view, false);

  if (not report.loadFromFile(QDir::currentPath() + "/modelos/relatorio_nfe.lrxml")) { throw RuntimeException("Não encontrou o modelo de impressão!", this); }

  // TODO: trocar 'Criado em' por 'DataEmissao'
  // comparacao direta na coluna (nao envolta em DATE_FORMAT) para poder usar indice (idx_nfe_tipo_status_created)
  SqlQuery query;
  query.prepare("SELECT SUM(icms), SUM(icmsst), SUM(frete), SUM(totalnfe), SUM(desconto), SUM(impimp), SUM(ipi), SUM(cofins), SUM(0), SUM(0), SUM(seguro), SUM(pis), SUM(0) FROM view_relatorio_nfe "
                "WHERE `Criado em` >= :dataDe AND `Criado em` < :dataAte AND (status = 'AUTORIZADA')");
  query.bindValue(":dataDe", ui->dateEditDe->date().toString("yyyy-MM-dd") + " 00:00:00");
  query.bindValue(":dataAte", ui->dateEditAte->date().addDays(1).toString("yyyy-MM-dd") + " 00:00:00");

  if (not query.exec()) { throw RuntimeException("Erro buscando dados: " + query.lastError().text(), this); }

  if (not query.first()) { throw RuntimeException("Não foram encontrado dados para a data: '" + ui->dateEditDe->date().toString("yyyy-MM-dd") + "/" + ui->dateEditAte->date().toString("yyyy-MM-dd") + "'"); }

  dataManager->setReportVariable("TotalIcms", "R$ " + QString::number(query.value("sum(icms)").toDouble(), 'f', 2));
  dataManager->setReportVariable("TotalIcmsSt", "R$ " + QString::number(query.value("sum(icmsst)").toDouble(), 'f', 2));
  dataManager->setReportVariable("TotalFrete", "R$ " + QString::number(query.value("sum(frete)").toDouble(), 'f', 2));
  dataManager->setReportVariable("TotalNfe", "R$ " + QString::number(query.value("sum(totalnfe)").toDouble(), 'f', 2));
  dataManager->setReportVariable("TotalDesconto", "R$ " + QString::number(query.value("sum(desconto)").toDouble(), 'f', 2));
  dataManager->setReportVariable("TotalImpImp", "R$ " + QString::number(query.value("sum(impimp)").toDouble(), 'f', 2));
  dataManager->setReportVariable("TotalIpi", "R$ " + QString::number(query.value("sum(ipi)").toDouble(), 'f', 2));
  dataManager->setReportVariable("TotalCofins", "R$ " + QString::number(query.value("sum(cofins)").toDouble(), 'f', 2));
  dataManager->setReportVariable("TotalPisSt", "R$ XXX");
  dataManager->setReportVariable("TotalCofinsSt", "R$ XXX");
  dataManager->setReportVariable("TotalSeguro", "R$ " + QString::number(query.value("sum(seguro)").toDouble(), 'f', 2));
  dataManager->setReportVariable("TotalPis", "R$ " + QString::number(query.value("sum(pis)").toDouble(), 'f', 2));
  dataManager->setReportVariable("TotalIssqn", "R$ XXX");

  if (not report.printToPDF(filename)) { throw RuntimeException("Erro gerando relatório: " + report.lastError(), this); }

  if (not QDesktopServices::openUrl(QUrl::fromLocalFile(filename))) { throw RuntimeException("Erro abrindo arquivo: " + QDir::currentPath() + filename, this); }
#else
  qApp->enqueueWarning("LimeReport desativado!", this);
#endif
}

void WidgetNfeSaida::on_pushButtonExportar_clicked() {
  // TODO: 5zipar arquivos exportados com nome descrevendo mes/notas/etc

  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  SqlQuery query;
  query.prepare("SELECT xml FROM nfe WHERE chaveAcesso = :chaveAcesso");

  for (const auto &index : selection) {
    // TODO: se a conexao com o acbr falhar ou der algum erro pausar o loop e perguntar para o usuario se ele deseja tentar novamente (do ponto que parou)
    // quando enviar para o acbr guardar a nota com status 'pendente' para consulta na receita
    // quando conseguir consultar se a receita retornar que a nota nao existe lá apagar aqui
    // se ela existir lá verificar se consigo pegar o xml autorizado e atualizar a nota pendente

    // pegar XML do MySQL e salvar em arquivo

    const QString chaveAcesso = model.data(index.row(), "chaveAcesso").toString();

    query.bindValue(":chaveAcesso", chaveAcesso);

    if (not query.exec()) { throw RuntimeException("Erro buscando XML: " + query.lastError().text()); }

    if (not query.first()) { throw RuntimeException("XML não encontrado para a NF-e com chave de acesso: '" + chaveAcesso + "'"); }

    File fileXml(QDir::currentPath() + "/arquivos/" + chaveAcesso + ".xml");

    if (not fileXml.open(QFile::WriteOnly)) { throw RuntimeException("Erro abrindo arquivo para escrita XML: " + fileXml.errorString()); }

    fileXml.write(query.value("xml").toByteArray());

    fileXml.flush();
    fileXml.close();

    // mandar XML para ACBr gerar PDF

    ACBrLib::gerarDanfe(query.value("xml").toString(), false);

    // copiar para pasta predefinida

    const QString pdfOrigem = QDir::currentPath() + "/pdf/" + chaveAcesso + "-nfe.pdf";
    const QString pdfDestino = QDir::currentPath() + "/arquivos/" + chaveAcesso + ".pdf";

    File filePdf(pdfDestino);

    if (filePdf.exists()) { filePdf.remove(); }

    if (not QFile::copy(pdfOrigem, pdfDestino)) { throw RuntimeException("Erro copiando PDF!"); }
  }

  qApp->enqueueInformation("Arquivos exportados com sucesso para:\n" + QDir::currentPath() + "/arquivos/", this);
}

void WidgetNfeSaida::on_groupBoxStatus_toggled(const bool enabled) {
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

void WidgetNfeSaida::on_pushButtonConsultarNFe_clicked() {
  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!", this); }

  const int idNFe = model.data(selection.first().row(), "idNFe").toInt();

  ACBr acbr;

  try {
    const auto [xml, resposta] = acbr.consultarNFe(idNFe);

    qApp->startTransaction("WidgetNfeSaida::on_pushButtonConsultarNFe");

    atualizarNFe(resposta, idNFe, xml);

    qApp->endTransaction();

    const int xMotivoIndex = resposta.indexOf("XMotivo=", 0, Qt::CaseInsensitive);

    if (xMotivoIndex == -1) { throw RuntimeException("Não encontrou o campo 'XMotivo':\n" + resposta); }

    const QString xMotivo = resposta.mid(xMotivoIndex + 8).split("\r\n").first();

    qApp->enqueueInformation(xMotivo, this);
  } catch (std::exception &) {
    updateTables();
    throw;
  }

  updateTables();
}

void WidgetNfeSaida::atualizarNFe(const QString &resposta, const int idNFe, const QString &xml) {
  QString status;

  if (resposta.contains("XMotivo=Autorizado o uso da NF-e", Qt::CaseInsensitive)) { status = "AUTORIZADA"; }
  if (resposta.contains("xEvento=Cancelamento registrado", Qt::CaseInsensitive)) { status = "CANCELADA"; }
  if (resposta.contains("XMotivo=Uso Denegado", Qt::CaseInsensitive)) { status = "DENEGADA"; }

  if (status.isEmpty()) { throw RuntimeException("Erro status vazio!"); }

  SqlQuery query;
  query.prepare("UPDATE nfe SET status = :status, xml = :xml WHERE idNFe = :idNFe");
  query.bindValue(":status", status);
  query.bindValue(":xml", xml);
  query.bindValue(":idNFe", idNFe);

  if (not query.exec()) { throw RuntimeException("Erro atualizando XML da NF-e: " + query.lastError().text()); }
}

void WidgetNfeSaida::cancelarNFe(const QString &chaveAcesso, const int row) {
  SqlQuery query;
  query.prepare("UPDATE nfe SET status = 'CANCELADA' WHERE chaveAcesso = :chaveAcesso");
  query.bindValue(":chaveAcesso", chaveAcesso);

  if (not query.exec()) { throw RuntimeException("Erro marcando NF-e como cancelada: " + query.lastError().text()); }

  // ---------------------------------------------------------

  const int idNFe = model.data(row, "idNFe").toInt();

  query.prepare("UPDATE venda_has_produto2 SET status = 'ENTREGA AGEND.', idNFeSaida = NULL WHERE status = 'EM ENTREGA' AND idNFeSaida = :idNFe");
  query.bindValue(":idNFe", idNFe);

  if (not query.exec()) { throw RuntimeException("Erro removendo NF-e da venda_produto: " + query.lastError().text()); }

  // ---------------------------------------------------------

  query.prepare("UPDATE venda_has_produto2 SET idNFeFutura = NULL WHERE idNFeFutura = :idNFe");
  query.bindValue(":idNFe", idNFe);

  if (not query.exec()) { throw RuntimeException("Erro removendo NF-e da venda_produto: " + query.lastError().text()); }

  // ---------------------------------------------------------

  query.prepare("UPDATE veiculo_has_produto SET status = 'ENTREGA AGEND.', idNFeSaida = NULL WHERE status = 'EM ENTREGA' AND idNFeSaida = :idNFe");
  query.bindValue(":idNFe", idNFe);

  if (not query.exec()) { throw RuntimeException("Erro removendo NF-e do veiculo_produto: " + query.lastError().text()); }
}

void WidgetNfeSaida::gravarArquivo(const QString &resposta, const QString &chaveAcesso) {
  File arquivo(QDir::currentPath() + "/arquivos/cancelamento_" + chaveAcesso + ".xml");

  if (not arquivo.open(QFile::WriteOnly)) { throw RuntimeException("Erro abrindo arquivo para escrita: " + arquivo.errorString()); }

  QTextStream stream(&arquivo);

  stream << resposta;

  arquivo.close();
}

void WidgetNfeSaida::ajustarGroupBoxStatus() {
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

void WidgetNfeSaida::on_pushButtonFollowup_clicked() {
  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!", this); }

  const QString idVenda = model.data(selection.first().row(), "idNFe").toString();

  auto *followup = new FollowUp(idVenda, FollowUp::Tipo::NFe, this);
  followup->setAttribute(Qt::WA_DeleteOnClose);
  followup->show();
}

void WidgetNfeSaida::on_dateEditDe_dateChanged(const QDate date) { ui->dateEditAte->setDate(date); }

void WidgetNfeSaida::on_groupBoxMes_toggled(const bool enabled) {
  const auto children = ui->groupBoxMes->findChildren<QDateEdit *>(QRegularExpression("dateEdit"));

  for (const auto &child : children) { child->setEnabled(enabled); }
}

// TODO: 2tela para importar notas de amostra (aba separada)
// TODO: nesta tela colocar um campo dizendo qual loja que emitiu a nota (nao precisa mostrar o cnpj, apenas o nome da loja) (e talvez poder filtrar pela loja)
// TODO: alterar status das NF-es para feminino -> autorizada, pendente, cancelada, denegada

#include "widgetnfesaida.h"
#include "ui_widgetnfesaida.h"

#include "acbr.h"
#include "application.h"
#include "doubledelegate.h"
#include "file.h"
#include "lrreportengine.h"
#include "reaisdelegate.h"
#include "sqlquery.h"
#include "usersession.h"
#include "xml_viewer.h"

#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QInputDialog>
#include <QMessageBox>
#include <QSqlError>

WidgetNfeSaida::WidgetNfeSaida(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetNfeSaida) { ui->setupUi(this); }

WidgetNfeSaida::~WidgetNfeSaida() { delete ui; }

void WidgetNfeSaida::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->checkBoxAutorizado, &QCheckBox::toggled, this, &WidgetNfeSaida::montaFiltro, connectionType);
  connect(ui->checkBoxCancelado, &QCheckBox::toggled, this, &WidgetNfeSaida::montaFiltro, connectionType);
  connect(ui->checkBoxPendente, &QCheckBox::toggled, this, &WidgetNfeSaida::montaFiltro, connectionType);
  connect(ui->dateEdit, &QDateEdit::dateChanged, this, &WidgetNfeSaida::montaFiltro, connectionType);
  connect(ui->groupBoxMes, &QGroupBox::toggled, this, &WidgetNfeSaida::montaFiltro, connectionType);
  connect(ui->groupBoxStatus, &QGroupBox::toggled, this, &WidgetNfeSaida::on_groupBoxStatus_toggled, connectionType);
  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetNfeSaida::montaFiltro, connectionType);
  connect(ui->pushButtonCancelarNFe, &QPushButton::clicked, this, &WidgetNfeSaida::on_pushButtonCancelarNFe_clicked, connectionType);
  connect(ui->pushButtonConsultarNFe, &QPushButton::clicked, this, &WidgetNfeSaida::on_pushButtonConsultarNFe_clicked, connectionType);
  connect(ui->pushButtonExportar, &QPushButton::clicked, this, &WidgetNfeSaida::on_pushButtonExportar_clicked, connectionType);
  connect(ui->pushButtonRelatorio, &QPushButton::clicked, this, &WidgetNfeSaida::on_pushButtonRelatorio_clicked, connectionType);
  connect(ui->table, &TableView::activated, this, &WidgetNfeSaida::on_table_activated, connectionType);
}

void WidgetNfeSaida::unsetConnections() {
  disconnect(ui->checkBoxAutorizado, &QCheckBox::toggled, this, &WidgetNfeSaida::montaFiltro);
  disconnect(ui->checkBoxCancelado, &QCheckBox::toggled, this, &WidgetNfeSaida::montaFiltro);
  disconnect(ui->checkBoxPendente, &QCheckBox::toggled, this, &WidgetNfeSaida::montaFiltro);
  disconnect(ui->dateEdit, &QDateEdit::dateChanged, this, &WidgetNfeSaida::montaFiltro);
  disconnect(ui->groupBoxMes, &QGroupBox::toggled, this, &WidgetNfeSaida::montaFiltro);
  disconnect(ui->groupBoxStatus, &QGroupBox::toggled, this, &WidgetNfeSaida::on_groupBoxStatus_toggled);
  disconnect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetNfeSaida::montaFiltro);
  disconnect(ui->pushButtonCancelarNFe, &QPushButton::clicked, this, &WidgetNfeSaida::on_pushButtonCancelarNFe_clicked);
  disconnect(ui->pushButtonConsultarNFe, &QPushButton::clicked, this, &WidgetNfeSaida::on_pushButtonConsultarNFe_clicked);
  disconnect(ui->pushButtonExportar, &QPushButton::clicked, this, &WidgetNfeSaida::on_pushButtonExportar_clicked);
  disconnect(ui->pushButtonRelatorio, &QPushButton::clicked, this, &WidgetNfeSaida::on_pushButtonRelatorio_clicked);
  disconnect(ui->table, &TableView::activated, this, &WidgetNfeSaida::on_table_activated);
}

void WidgetNfeSaida::updateTables() {
  if (not isSet) {
    ui->dateEdit->setDate(qApp->serverDate());
    setConnections();
    isSet = true;
  }

  if (not modelIsSet) {
    setupTables();
    montaFiltro();
    modelIsSet = true;
  }

  modelViewNFeSaida.select();
}

void WidgetNfeSaida::resetTables() { modelIsSet = false; }

void WidgetNfeSaida::setupTables() {
  modelViewNFeSaida.setTable("view_nfe_saida");

  modelViewNFeSaida.setHeaderData("valor", "R$");

  modelViewNFeSaida.setSort("Criado em");

  ui->table->setModel(&modelViewNFeSaida);

  ui->table->hideColumn("idNFe");
  ui->table->hideColumn("chaveAcesso");

  ui->table->setItemDelegate(new DoubleDelegate(this));

  ui->table->setItemDelegateForColumn("valor", new ReaisDelegate(this));
}

void WidgetNfeSaida::on_table_activated(const QModelIndex &index) {
  SqlQuery query;
  query.prepare("SELECT xml FROM nfe WHERE idNFe = :idNFe");
  query.bindValue(":idNFe", modelViewNFeSaida.data(index.row(), "idNFe"));

  if (not query.exec() or not query.first()) { throw RuntimeException("Erro buscando xml da nota: " + query.lastError().text(), this); }

  auto *viewer = new XML_Viewer(query.value("xml").toByteArray(), this);
  viewer->setAttribute(Qt::WA_DeleteOnClose);
}

void WidgetNfeSaida::montaFiltro() {
  QStringList filtros;

  const QString text = qApp->sanitizeSQL(ui->lineEditBusca->text());

  const QString filtroBusca = "(NFe LIKE '%" + text + "%' OR Venda LIKE '%" + text + "%' OR `CPF/CNPJ` LIKE '%" + text + "%' OR Cliente LIKE '%" + text + "%')";
  if (not text.isEmpty()) { filtros << filtroBusca; }

  //-------------------------------------

  const QString filtroData = ui->groupBoxMes->isChecked() ? "DATE_FORMAT(`Criado em`, '%Y-%m') = '" + ui->dateEdit->date().toString("yyyy-MM") + "'" : "";
  if (not filtroData.isEmpty()) { filtros << filtroData; }

  //-------------------------------------

  QStringList filtroCheck;

  const auto children = ui->groupBoxStatus->findChildren<QCheckBox *>();

  for (const auto &child : children) {
    if (child->isChecked()) { filtroCheck << "'" + child->text().toUpper() + "'"; }
  }

  if (not filtroCheck.isEmpty()) { filtros << "status IN (" + filtroCheck.join(", ") + ")"; }

  //-------------------------------------

  modelViewNFeSaida.setFilter(filtros.join(" AND "));
}

void WidgetNfeSaida::on_pushButtonCancelarNFe_clicked() {
  // TODO: como no cancelamento o acbr nao atualiza o xml, fazer a consulta para atualizar o xml como cancelado antes de enviar para a contabilidade?

  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!", this); }

  // -------------------------------------------------------------------------

  const QString emailContabilidade = UserSession::getSetting("User/emailContabilidade").toString();

  if (emailContabilidade.isEmpty()) { throw RuntimeError(R"("Email Contabilidade" não está configurado! Ajuste no menu "Opções->Configurações")", this); }

  const QString emailLogistica = UserSession::getSetting("User/emailLogistica").toString();

  if (emailLogistica.isEmpty()) { throw RuntimeError(R"("Email Logistica" não está configurado! Ajuste no menu "Opções->Configurações")", this); }

  // -------------------------------------------------------------------------

  QMessageBox msgBox(QMessageBox::Question, "Cancelar?", "Tem certeza que deseja cancelar?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.setButtonText(QMessageBox::Yes, "Cancelar");
  msgBox.setButtonText(QMessageBox::No, "Voltar");

  if (msgBox.exec() == QMessageBox::No) { return; }

  // -------------------------------------------------------------------------

  const QString justificativa = QInputDialog::getText(this, "Justificativa", "Entre 15 e 200 caracteres: ");

  if (justificativa.size() < 15 or justificativa.size() > 200) { throw RuntimeError("Justificativa fora do tamanho!", this); }

  // -------------------------------------------------------------------------

  const int row = list.first().row();

  const QString chaveAcesso = modelViewNFeSaida.data(row, "chaveAcesso").toString();

  ACBr acbrRemoto;

  const QString resposta = acbrRemoto.enviarComando("NFE.CancelarNFe(" + chaveAcesso + ", " + justificativa + ")");

  // TODO: verificar outras possiveis respostas (tinha algo como 'cancelamento registrado fora do prazo')
  if (not resposta.contains("xEvento=Cancelamento registrado")) { throw RuntimeException("Resposta: " + resposta); }

  qApp->startTransaction("WidgetNfeSaida::on_pushButtonCancelarNFe");

  cancelarNFe(chaveAcesso, row);

  qApp->endTransaction();

  updateTables();

  const int xMotivoIndex = resposta.indexOf("XMotivo=", Qt::CaseInsensitive);

  if (xMotivoIndex == -1) { throw RuntimeException("Não encontrou o campo 'xMotivo': " + resposta); }

  const QString xMotivo = resposta.mid(xMotivoIndex + 8).split("\r\n").first();

  qApp->enqueueInformation(xMotivo, this);

  gravarArquivo(resposta, chaveAcesso);

  // -------------------------------------------------------------------------

  //  const QString filePath = QDir::currentPath() + "/arquivos/cancelamento_" + chaveAcesso + ".xml";
  //  const QString assunto = "Cancelamento NFe - " + modelViewNFeSaida.data(row, "NFe").toString() + " - STACCATO REVESTIMENTOS COMERCIO E REPRESENTACAO LTDA";

  // TODO: enviar o xml atualizado com o cancelamento
  // TODO: enviar a danfe
  //  ACBr acbrLocal;
  //  acbrLocal.enviarEmail(emailContabilidade, emailLogistica, assunto, filePath);
}

// TODO: 1verificar se ao cancelar nota ela é removida do venda_produto/veiculo_produto
// TODO: 1botao para gerar relatorio igual ao da receita

void WidgetNfeSaida::on_pushButtonRelatorio_clicked() {
  // TODO: 3formatar decimais no padrao BR
  // TODO: 3perguntar um intervalo de tempo para filtrar as notas
  // TODO: 3verificar quais as tags na nota dos campos que faltam preencher

  if (not ui->groupBoxMes->isChecked()) { throw RuntimeError("Selecione um mês para gerar o relatório!", this); }

  const QString filename = "arquivos/relatorio_nfe.pdf";

  File file(filename);

  if (not file.open(QFile::WriteOnly)) { throw RuntimeException("Erro abrindo arquivo para escrita xml: " + file.errorString()); }

  file.close();

  LimeReport::ReportEngine report;
  auto dataManager = report.dataManager();

  SqlTableModel view;
  view.setTable("view_relatorio_nfe");

  // TODO: trocar 'Criado em' por 'DataEmissao'
  view.setFilter("DATE_FORMAT(`Criado em`, '%Y-%m') = '" + ui->dateEdit->date().toString("yyyy-MM") + "' AND (status = 'AUTORIZADO')");

  view.select();

  dataManager->addModel("view", &view, true);

  if (not report.loadFromFile("modelos/relatorio_nfe.lrxml")) { throw RuntimeException("Não encontrou o modelo de impressão!", this); }

  // TODO: trocar 'Criado em' por 'DataEmissao'
  SqlQuery query;
  query.prepare("SELECT SUM(icms), SUM(icmsst), SUM(frete), SUM(totalnfe), SUM(desconto), SUM(impimp), SUM(ipi), SUM(cofins), SUM(0), SUM(0), SUM(seguro), SUM(pis), SUM(0) FROM view_relatorio_nfe "
                "WHERE DATE_FORMAT(`Criado em`, '%Y-%m') = :data AND (status = 'AUTORIZADO')");
  query.bindValue(":data", ui->dateEdit->date().toString("yyyy-MM"));

  if (not query.exec() or not query.first()) { throw RuntimeException("Erro buscando dados: " + query.lastError().text(), this); }

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

  if (not report.printToPDF(filename)) { throw RuntimeException("Erro gerando relatório!", this); }

  if (not QDesktopServices::openUrl(QUrl::fromLocalFile(filename))) { throw RuntimeException("Erro abrindo arquivo: " + QDir::currentPath() + filename, this); }
}

void WidgetNfeSaida::on_pushButtonExportar_clicked() {
  // TODO: 5zipar arquivos exportados com nome descrevendo mes/notas/etc

  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  SqlQuery query;
  query.prepare("SELECT xml FROM nfe WHERE chaveAcesso = :chaveAcesso");

  ACBr acbrLocal;

  for (const auto &index : list) {
    // TODO: se a conexao com o acbr falhar ou der algum erro pausar o loop e perguntar para o usuario se ele deseja tentar novamente (do ponto que parou)
    // quando enviar para o acbr guardar a nota com status 'pendente' para consulta na receita
    // quando conseguir consultar se a receita retornar que a nota nao existe lá apagar aqui
    // se ela existir lá verificar se consigo pegar o xml autorizado e atualizar a nota pendente

    if (modelViewNFeSaida.data(index.row(), "status").toString() != "AUTORIZADO") { continue; }

    // pegar XML do MySQL e salvar em arquivo

    const QString chaveAcesso = modelViewNFeSaida.data(index.row(), "chaveAcesso").toString();

    query.bindValue(":chaveAcesso", chaveAcesso);

    if (not query.exec() or not query.first()) { throw RuntimeException("Erro buscando xml: " + query.lastError().text()); }

    File fileXml("arquivos/" + chaveAcesso + ".xml");

    if (not fileXml.open(QFile::WriteOnly)) { throw RuntimeException("Erro abrindo arquivo para escrita xml: " + fileXml.errorString()); }

    fileXml.write(query.value("xml").toByteArray());

    fileXml.flush();
    fileXml.close();

    // mandar XML para ACBr gerar PDF

    const QString pdfOrigem = acbrLocal.gerarDanfe(query.value("xml").toByteArray(), false);

    if (pdfOrigem.isEmpty()) { throw RuntimeException("Resposta vazia!"); }

    // copiar para pasta predefinida

    const QString pdfDestino = "arquivos/" + chaveAcesso + ".pdf";

    File filePdf(pdfDestino);

    if (filePdf.exists()) { filePdf.remove(); }

    if (not QFile::copy(pdfOrigem, pdfDestino)) { throw RuntimeException("Erro copiando pdf!"); }
  }

  qApp->enqueueInformation("Arquivos exportados com sucesso para " + QDir::currentPath() + "/arquivos/", this);
}

void WidgetNfeSaida::on_groupBoxStatus_toggled(const bool enabled) {
  unsetConnections();

  try {
    const auto children = ui->groupBoxStatus->findChildren<QCheckBox *>();

    for (const auto &child : children) {
      child->setEnabled(true);
      child->setChecked(enabled);
    }
  } catch (std::exception &e) {}

  setConnections();

  montaFiltro();
}

void WidgetNfeSaida::on_pushButtonConsultarNFe_clicked() {
  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!", this); }

  const int idNFe = modelViewNFeSaida.data(selection.first().row(), "idNFe").toInt();

  ACBr acbrRemoto;

  const auto [xml, resposta] = acbrRemoto.consultarNFe(idNFe);

  qApp->startTransaction("WidgetNfeSaida::on_pushButtonConsultarNFe");

  atualizarNFe(resposta, idNFe, xml);

  qApp->endTransaction();

  updateTables();

  const int xMotivoIndex = resposta.indexOf("XMotivo=", Qt::CaseInsensitive);

  if (xMotivoIndex == -1) { throw RuntimeException("Não encontrou o campo 'XMotivo':\n" + resposta); }

  const QString xMotivo = resposta.mid(xMotivoIndex + 8).split("\r\n").first();

  qApp->enqueueInformation(xMotivo, this);
}

void WidgetNfeSaida::atualizarNFe(const QString &resposta, const int idNFe, const QString &xml) {
  QString status;

  if (resposta.contains("XMotivo=Autorizado o uso da NF-e")) { status = "AUTORIZADO"; }
  if (resposta.contains("xEvento=Cancelamento registrado")) { status = "CANCELADA"; }

  if (status.isEmpty()) { throw RuntimeException("Erro status vazio"); }

  SqlQuery query;
  query.prepare("UPDATE nfe SET status = :status, xml = :xml WHERE idNFe = :idNFe");
  query.bindValue(":status", status);
  query.bindValue(":xml", xml);
  query.bindValue(":idNFe", idNFe);

  if (not query.exec()) { throw RuntimeException("Erro atualizando xml da nota: " + query.lastError().text()); }
}

void WidgetNfeSaida::cancelarNFe(const QString &chaveAcesso, const int row) {
  SqlQuery query;
  query.prepare("UPDATE nfe SET status = 'CANCELADA' WHERE chaveAcesso = :chaveAcesso");
  query.bindValue(":chaveAcesso", chaveAcesso);

  if (not query.exec()) { throw RuntimeException("Erro marcando nota como cancelada: " + query.lastError().text()); }

  const int idNFe = modelViewNFeSaida.data(row, "idNFe").toInt();

  query.prepare("UPDATE venda_has_produto2 SET status = 'ENTREGA AGEND.', idNFeSaida = NULL WHERE status = 'EM ENTREGA' AND idNFeSaida = :idNFe");
  query.bindValue(":idNFe", idNFe);

  if (not query.exec()) { throw RuntimeException("Erro removendo NFe da venda_produto: " + query.lastError().text()); }

  query.prepare("UPDATE veiculo_has_produto SET status = 'ENTREGA AGEND.', idNFeSaida = NULL WHERE status = 'EM ENTREGA' AND idNFeSaida = :idNFe");
  query.bindValue(":idNFe", idNFe);

  if (not query.exec()) { throw RuntimeException("Erro removendo NFe do veiculo_produto: " + query.lastError().text()); }
}

void WidgetNfeSaida::gravarArquivo(const QString &resposta, const QString &chaveAcesso) {
  File arquivo("arquivos/cancelamento_" + chaveAcesso + ".xml");

  if (not arquivo.open(QFile::WriteOnly)) { throw RuntimeException("Erro abrindo arquivo para escrita: " + arquivo.errorString()); }

  QTextStream stream(&arquivo);

  stream << resposta;

  arquivo.close();
}

// TODO: 2tela para importar notas de amostra (aba separada)
// TODO: nesta tela colocar um campo dizendo qual loja que emitiu a nota (nao precisa mostrar o cnpj, apenas o nome da loja) (e talvez poder filtrar pela loja)

#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QInputDialog>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>
#include <QUrl>

#include "acbr.h"
#include "application.h"
#include "doubledelegate.h"
#include "lrreportengine.h"
#include "reaisdelegate.h"
#include "sendmail.h"
#include "ui_widgetnfesaida.h"
#include "usersession.h"
#include "widgetnfesaida.h"
#include "xml_viewer.h"

WidgetNfeSaida::WidgetNfeSaida(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetNfeSaida) { ui->setupUi(this); }

WidgetNfeSaida::~WidgetNfeSaida() { delete ui; }

void WidgetNfeSaida::setConnections() {
  connect(ui->checkBoxAutorizado, &QCheckBox::toggled, this, &WidgetNfeSaida::montaFiltro);
  connect(ui->checkBoxCancelado, &QCheckBox::toggled, this, &WidgetNfeSaida::montaFiltro);
  connect(ui->checkBoxPendente, &QCheckBox::toggled, this, &WidgetNfeSaida::montaFiltro);
  connect(ui->dateEdit, &QDateEdit::dateChanged, this, &WidgetNfeSaida::montaFiltro);
  connect(ui->groupBoxMes, &QGroupBox::toggled, this, &WidgetNfeSaida::montaFiltro);
  connect(ui->groupBoxStatus, &QGroupBox::toggled, this, &WidgetNfeSaida::on_groupBoxStatus_toggled);
  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetNfeSaida::montaFiltro);
  connect(ui->pushButtonCancelarNFe, &QPushButton::clicked, this, &WidgetNfeSaida::on_pushButtonCancelarNFe_clicked);
  connect(ui->pushButtonConsultarNFe, &QPushButton::clicked, this, &WidgetNfeSaida::on_pushButtonConsultarNFe_clicked);
  connect(ui->pushButtonExportar, &QPushButton::clicked, this, &WidgetNfeSaida::on_pushButtonExportar_clicked);
  connect(ui->pushButtonRelatorio, &QPushButton::clicked, this, &WidgetNfeSaida::on_pushButtonRelatorio_clicked);
  connect(ui->table, &TableView::activated, this, &WidgetNfeSaida::on_table_activated);
  connect(ui->table, &TableView::entered, this, &WidgetNfeSaida::on_table_entered);
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
  disconnect(ui->table, &TableView::entered, this, &WidgetNfeSaida::on_table_entered);
}

void WidgetNfeSaida::updateTables() {
  if (not isSet) {
    ui->dateEdit->setDate(QDate::currentDate());
    setConnections();
    isSet = true;
  }

  if (not modelIsSet) {
    setupTables();
    montaFiltro();
    modelIsSet = true;
  }

  if (not modelViewNFeSaida.select()) { return; }

  ui->table->resizeColumnsToContents();
}

void WidgetNfeSaida::resetTables() { modelIsSet = false; }

void WidgetNfeSaida::setupTables() {
  modelViewNFeSaida.setTable("view_nfe_saida");
  modelViewNFeSaida.setEditStrategy(QSqlTableModel::OnManualSubmit);

  modelViewNFeSaida.setHeaderData("created", "Criado em");
  modelViewNFeSaida.setHeaderData("valor", "R$");

  ui->table->setModel(&modelViewNFeSaida);
  ui->table->hideColumn("idNFe");
  ui->table->hideColumn("chaveAcesso");
  ui->table->setItemDelegate(new DoubleDelegate(this));
  ui->table->setItemDelegateForColumn("valor", new ReaisDelegate(this));
}

void WidgetNfeSaida::on_table_activated(const QModelIndex &index) {
  QSqlQuery query;
  query.prepare("SELECT xml FROM nfe WHERE idNFe = :idNFe");
  query.bindValue(":idNFe", modelViewNFeSaida.data(index.row(), "idNFe"));

  if (not query.exec() or not query.first()) { return qApp->enqueueError("Erro buscando xml da nota: " + query.lastError().text()); }

  auto *viewer = new XML_Viewer(this);
  viewer->setAttribute(Qt::WA_DeleteOnClose);
  viewer->exibirXML(query.value("xml").toByteArray());
}

void WidgetNfeSaida::montaFiltro() {
  // TODO: 5ordenar por 'data criado'

  QStringList filtros;

  const QString text = ui->lineEditBusca->text();

  const QString filtroBusca = "(NFe LIKE '%" + text + "%' OR Venda LIKE '%" + text + "%' OR `CPF/CNPJ` LIKE '%" + text + "%' OR Cliente LIKE '%" + text + "%')";
  if (not text.isEmpty()) { filtros << filtroBusca; }

  //-------------------------------------

  const QString filtroData = ui->groupBoxMes->isChecked() ? "DATE_FORMAT(`Criado em`, '%Y-%m') = '" + ui->dateEdit->date().toString("yyyy-MM") + "'" : "";
  if (not filtroData.isEmpty()) { filtros << filtroData; }

  //-------------------------------------

  QStringList filtroCheck;

  Q_FOREACH (const auto &child, ui->groupBoxStatus->findChildren<QCheckBox *>()) {
    if (child->isChecked()) { filtroCheck << "status = '" + child->text().toUpper() + "'"; }
  }

  if (not filtroCheck.isEmpty()) { filtros << "(" + filtroCheck.join(" OR ") + ")"; }

  //-------------------------------------

  modelViewNFeSaida.setFilter(filtros.join(" AND "));

  if (not modelViewNFeSaida.select()) { return; }

  ui->table->resizeColumnsToContents();
}

void WidgetNfeSaida::on_table_entered(const QModelIndex) { ui->table->resizeColumnsToContents(); }

void WidgetNfeSaida::on_pushButtonCancelarNFe_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhuma linha selecionada!"); }

  QMessageBox msgBox(QMessageBox::Question, "Cancelar?", "Tem certeza que deseja cancelar?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.setButtonText(QMessageBox::Yes, "Cancelar");
  msgBox.setButtonText(QMessageBox::No, "Voltar");

  if (msgBox.exec() == QMessageBox::No) { return; }

  const QString justificativa = QInputDialog::getText(this, "Justificativa", "Entre 15 e 200 caracteres: ");

  if (justificativa.size() < 15 or justificativa.size() > 200) { return qApp->enqueueError("Justificativa fora do tamanho!"); }

  const int row = list.first().row();

  const QString chaveAcesso = modelViewNFeSaida.data(row, "chaveAcesso").toString();

  ACBr acbr;

  const auto resposta = acbr.enviarComando("NFE.CancelarNFe(" + chaveAcesso + ", " + justificativa + ")");

  if (not resposta) { return; }

  // TODO: verificar outras possiveis respostas (tinha algo como 'cancelamento registrado fora do prazo')
  if (not resposta->contains("xEvento=Cancelamento registrado")) { return qApp->enqueueError("Resposta: " + *resposta); }

  if (not qApp->startTransaction()) { return; }

  if (not cancelarNFe(chaveAcesso, row)) { return qApp->rollbackTransaction(); }

  if (not qApp->endTransaction()) { return; }

  qApp->enqueueInformation(*resposta);

  if (not gravarArquivo(*resposta)) { return; }

  const QString filePath = QDir::currentPath() + "/cancelamento.xml";

  // -------------------------------------------------------------------------

  const auto emailContabilidade = UserSession::getSetting("User/emailContabilidade");

  if (not emailContabilidade) { return qApp->enqueueError("A chave 'emailContabilidade' não está configurada!"); }

  const auto emailLogistica = UserSession::getSetting("User/emailLogistica");

  if (not emailLogistica) { return qApp->enqueueError("A chave 'emailLogistica' não está configurada!"); }

  const QString assunto = "Cancelamento NFe - " + modelViewNFeSaida.data(row, "NFe").toString() + " - STACCATO REVESTIMENTOS COMERCIO E REPRESENTACAO LTDA";

  acbr.enviarEmail(emailContabilidade.value().toString(), emailLogistica.value().toString(), assunto, filePath);
}

// TODO: 1verificar se ao cancelar nota ela é removida do venda_produto/veiculo_produto
// TODO: 1botao para gerar relatorio igual ao da receita

void WidgetNfeSaida::on_pushButtonRelatorio_clicked() {
  // TODO: 3formatar decimais no padrao BR
  // TODO: 3perguntar um intervalo de tempo para filtrar as notas
  // TODO: 3verificar quais as tags na nota dos campos que faltam preencher

  if (not ui->groupBoxMes->isChecked()) { return qApp->enqueueError("Selecione um mês para gerar o relatório!"); }

  LimeReport::ReportEngine report;
  auto dataManager = report.dataManager();

  SqlRelationalTableModel view;
  view.setTable("view_relatorio_nfe");
  view.setFilter("DATE_FORMAT(`Criado em`, '%Y-%m') = '" + ui->dateEdit->date().toString("yyyy-MM") + "' AND (status = 'AUTORIZADO')");

  if (not view.select()) { return; }

  dataManager->addModel("view", &view, true);

  if (not report.loadFromFile("relatorio_nfe.lrxml")) { return qApp->enqueueError("Não encontrou o modelo de impressão!"); }

  QSqlQuery query;
  query.prepare("SELECT SUM(icms), SUM(icmsst), SUM(frete), SUM(totalnfe), SUM(desconto), SUM(impimp), SUM(ipi), SUM(cofins), SUM(0), SUM(0), SUM(seguro), SUM(pis), SUM(0) FROM view_relatorio_nfe "
                "WHERE DATE_FORMAT(`Criado em`, '%Y-%m') = :data AND (status = 'AUTORIZADO')");
  query.bindValue(":data", ui->dateEdit->date().toString("yyyy-MM"));

  if (not query.exec() or not query.first()) { return qApp->enqueueError("Erro buscando dados: " + query.lastError().text()); }

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

  if (not report.printToPDF(QDir::currentPath() + "/relatorio.pdf")) { return qApp->enqueueError("Erro gerando relatório!"); }

  if (not QDesktopServices::openUrl(QUrl::fromLocalFile(QDir::currentPath() + "/relatorio.pdf"))) { return qApp->enqueueError("Erro abrindo arquivo: " + QDir::currentPath() + "/relatorio.pdf'!"); }
}

void WidgetNfeSaida::on_pushButtonExportar_clicked() {
  // TODO: 5zipar arquivos exportados com nome descrevendo mes/notas/etc

  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhum item selecionado!"); }

  QSqlQuery query;
  query.prepare("SELECT xml FROM nfe WHERE chaveAcesso = :chaveAcesso");

  const auto folderKeyXml = UserSession::getSetting("User/EntregasXmlFolder");

  if (not folderKeyXml) { return qApp->enqueueError("Não há uma pasta definida para salvar XML. Por favor escolha uma nas configurações do ERP!"); }

  const auto folderKeyPdf = UserSession::getSetting("User/EntregasPdfFolder");

  if (not folderKeyPdf) { return qApp->enqueueError("Não há uma pasta definida para salvar PDF. Por favor escolha uma nas configurações do ERP!"); }

  const QString xmlFolder = folderKeyXml.value().toString();
  const QString pdfFolder = folderKeyPdf.value().toString();

  // TODO: create folders if they dont exist (it wont work if they dont)

  ACBr acbr;

  for (const auto &item : list) {
    // TODO: se a conexao com o acbr falhar ou der algum erro pausar o loop e perguntar para o usuario se ele deseja tentar novamente (do ponto que parou)
    // quando enviar para o acbr guardar a nota com status 'pendente' para consulta na receita
    // quando conseguir consultar se a receita retornar que a nota nao existe lá apagar aqui
    // se ela existir lá verificar se consigo pegar o xml autorizado e atualizar a nota pendente

    if (modelViewNFeSaida.data(item.row(), "status").toString() != "AUTORIZADO") { continue; }

    // pegar xml do bd e salvar em arquivo

    const QString chaveAcesso = modelViewNFeSaida.data(item.row(), "chaveAcesso").toString();

    query.bindValue(":chaveAcesso", chaveAcesso);

    if (not query.exec() or not query.first()) { return qApp->enqueueError("Erro buscando xml: " + query.lastError().text()); }

    QFile fileXml(xmlFolder + "/" + chaveAcesso + ".xml");

    qDebug() << "xml: " + xmlFolder + "/" + chaveAcesso + ".xml";

    if (not fileXml.open(QFile::WriteOnly)) { return qApp->enqueueError("Erro abrindo arquivo para escrita xml: " + fileXml.errorString()); }

    fileXml.write(query.value("xml").toByteArray());

    fileXml.flush();
    fileXml.close();

    // mandar xml para acbr gerar pdf

    const auto pdfOrigem = acbr.gerarDanfe(query.value("xml").toByteArray(), false);

    if (not pdfOrigem) { return; }

    if (pdfOrigem->isEmpty()) { return qApp->enqueueError("Resposta vazia!"); }

    // copiar para pasta predefinida

    const QString pdfDestino = pdfFolder + "/" + chaveAcesso + ".pdf";

    QFile filePdf(pdfDestino);

    if (filePdf.exists()) { filePdf.remove(); }

    if (not QFile::copy(*pdfOrigem, pdfDestino)) { return qApp->enqueueError("Erro copiando pdf!"); }
  }

  qApp->enqueueInformation("Arquivos exportados com sucesso para " + pdfFolder + "!");
}

void WidgetNfeSaida::on_groupBoxStatus_toggled(const bool enabled) {
  unsetConnections();

  Q_FOREACH (const auto &child, ui->groupBoxStatus->findChildren<QCheckBox *>()) {
    child->setEnabled(true);
    child->setChecked(enabled);
  }

  setConnections();

  montaFiltro();
}

void WidgetNfeSaida::on_pushButtonConsultarNFe_clicked() {
  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { return qApp->enqueueError("Nenhuma linha selecionada!"); }

  const int idNFe = modelViewNFeSaida.data(selection.first().row(), "idNFe").toInt();

  ACBr acbr;

  if (auto tuple = acbr.consultarNFe(idNFe); tuple) {
    const auto [xml, resposta] = tuple.value();

    if (not qApp->startTransaction()) { return; }

    if (not atualizarNFe(idNFe, xml)) { return qApp->rollbackTransaction(); }

    if (not qApp->endTransaction()) { return; }

    qApp->enqueueInformation(resposta);
  }
}

bool WidgetNfeSaida::atualizarNFe(const int idNFe, const QString &xml) {
  QSqlQuery query;
  query.prepare("UPDATE nfe SET status = 'AUTORIZADO', xml = :xml WHERE idNFe = :idNFe");
  query.bindValue(":xml", xml);
  query.bindValue(":idNFe", idNFe);

  if (not query.exec()) { return qApp->enqueueError(false, "Erro marcando nota como 'AUTORIZADO': " + query.lastError().text()); }

  return true;
}

bool WidgetNfeSaida::cancelarNFe(const QString &chaveAcesso, const int row) {
  QSqlQuery query;
  query.prepare("UPDATE nfe SET status = 'CANCELADO' WHERE chaveAcesso = :chaveAcesso");
  query.bindValue(":chaveAcesso", chaveAcesso);

  if (not query.exec()) { return qApp->enqueueError(false, "Erro marcando nota como cancelada: " + query.lastError().text()); }

  const int idNFe = modelViewNFeSaida.data(row, "idNFe").toInt();

  query.prepare("UPDATE venda_has_produto SET status = 'ENTREGA AGEND.', idNFeSaida = NULL WHERE idNFeSaida = :idNFe AND status NOT IN ('CANCELADO', 'DEVOLVIDO')");
  query.bindValue(":idNFe", idNFe);

  if (not query.exec()) { return qApp->enqueueError(false, "Erro removendo NFe da venda_produto: " + query.lastError().text()); }

  query.prepare("UPDATE veiculo_has_produto SET status = 'ENTREGA AGEND.', idNFeSaida = NULL WHERE idNFeSaida = :idNFe");
  query.bindValue(":idNFe", idNFe);

  if (not query.exec()) { return qApp->enqueueError(false, "Erro removendo NFe do veiculo_produto: " + query.lastError().text()); }

  return true;
}

bool WidgetNfeSaida::gravarArquivo(const QString &resposta) {
  QFile arquivo(QDir::currentPath() + "/cancelamento.xml");

  if (not arquivo.open(QFile::WriteOnly)) { return qApp->enqueueError(false, "Erro abrindo arquivo para escrita: " + arquivo.errorString()); }

  QTextStream stream(&arquivo);

  stream << resposta;

  arquivo.close();

  return true;
}

// TODO: guardar idVenda/outros dados na nfe para quando cancelar nao perder os vinculos
// TODO: 2tela para importar notas de amostra (aba separada)
// TODO: 0nao estou guardando o valor na nota
// TODO: 0algumas notas nao estao mostrando valor
// TODO: nesta tela colocar um campo dizendo qual loja que emitiu a nota (nao precisa mostrar o cnpj, apenas o nome da loja) (e talvez poder filtrar pela loja)
// TODO: fazer sistema para trocar notas futuras pela nota real

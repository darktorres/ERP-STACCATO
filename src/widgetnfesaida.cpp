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
#include "doubledelegate.h"
#include "lrreportengine.h"
#include "reaisdelegate.h"
#include "sendmail.h"
#include "ui_widgetnfesaida.h"
#include "usersession.h"
#include "widgetnfesaida.h"
#include "xml_viewer.h"

WidgetNfeSaida::WidgetNfeSaida(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetNfeSaida) {
  ui->setupUi(this);

  ui->dateEdit->setDate(QDate::currentDate());

  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetNfeSaida::montaFiltro);
  connect(ui->groupBoxMes, &QGroupBox::toggled, this, &WidgetNfeSaida::montaFiltro);
  connect(ui->dateEdit, &QDateEdit::dateChanged, this, &WidgetNfeSaida::montaFiltro);
  connect(ui->checkBoxAutorizado, &QCheckBox::toggled, this, &WidgetNfeSaida::montaFiltro);
  connect(ui->checkBoxPendente, &QCheckBox::toggled, this, &WidgetNfeSaida::montaFiltro);
  connect(ui->checkBoxCancelado, &QCheckBox::toggled, this, &WidgetNfeSaida::montaFiltro);
}

WidgetNfeSaida::~WidgetNfeSaida() { delete ui; }

bool WidgetNfeSaida::updateTables() {
  if (model.tableName().isEmpty()) setupTables();

  // TODO: 1refatorar
  //  if (not model.select()) {
  //    emit errorSignal("Erro lendo tabela NFe: " + model.lastError().text());
  //    return false;
  //  }
  montaFiltro();

  ui->table->resizeColumnsToContents();

  return true;
}

void WidgetNfeSaida::setupTables() {
  model.setTable("view_nfe_saida");
  model.setEditStrategy(QSqlTableModel::OnManualSubmit);

  model.setHeaderData("created", "Criado em");
  model.setHeaderData("valor", "R$");

  ui->table->setModel(&model);
  ui->table->hideColumn("idNFe");
  ui->table->hideColumn("chaveAcesso");
  ui->table->setItemDelegate(new DoubleDelegate(this));
  ui->table->setItemDelegateForColumn("valor", new ReaisDelegate(this));
}

void WidgetNfeSaida::on_table_activated(const QModelIndex &index) {
  QSqlQuery query;
  query.prepare("SELECT xml FROM nfe WHERE idNFe = :idNFe");
  query.bindValue(":idNFe", model.data(index.row(), "idNFe"));

  if (not query.exec() or not query.first()) {
    QMessageBox::critical(this, "Erro!", "Erro buscando xml da nota: " + query.lastError().text());
    return;
  }

  auto *viewer = new XML_Viewer(this);
  viewer->setAttribute(Qt::WA_DeleteOnClose);
  viewer->exibirXML(query.value("xml").toByteArray());
}

void WidgetNfeSaida::montaFiltro() {
  // TODO: 5ordenar por 'data criado'

  const QString text = ui->lineEditBusca->text();

  const QString filtroBusca = "(NFe LIKE '%" + text + "%' OR Venda LIKE '%" + text + "%' OR `CPF/CNPJ` LIKE '%" + text + "%' OR Cliente LIKE '%" + text + "%')";

  const QString filtroData = ui->groupBoxMes->isChecked() ? " AND DATE_FORMAT(`Criado em`, '%Y-%m') = '" + ui->dateEdit->date().toString("yyyy-MM") + "'" : "";

  QString filtroCheck;

  for (auto const &child : ui->groupBoxStatus->findChildren<QCheckBox *>()) {
    if (child->isChecked()) {
      filtroCheck += filtroCheck.isEmpty() ? "status = '" + child->text().toUpper() + "'" : " OR status = '" + child->text().toUpper() + "'";
    }
  }

  filtroCheck = filtroCheck.isEmpty() ? "" : " AND (" + filtroCheck + ")";

  model.setFilter(filtroBusca + filtroData + filtroCheck);

  if (not model.select()) QMessageBox::critical(this, "Erro!", "Erro lendo tabela: " + model.lastError().text());

  ui->table->resizeColumnsToContents();
}

void WidgetNfeSaida::on_table_entered(const QModelIndex &) { ui->table->resizeColumnsToContents(); }

void WidgetNfeSaida::on_pushButtonCancelarNFe_clicked() {
  // TODO: colocar returns se as operações falharem

  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) {
    QMessageBox::critical(this, "Erro!", "Nenhuma linha selecionada!");
    return;
  }

  QMessageBox msgBox(QMessageBox::Question, "Cancelar?", "Tem certeza que deseja cancelar?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.setButtonText(QMessageBox::Yes, "Cancelar");
  msgBox.setButtonText(QMessageBox::No, "Voltar");

  if (msgBox.exec() == QMessageBox::No) return;

  const QString justificativa = QInputDialog::getText(this, "Justificativa", "Entre 15 e 200 caracteres: ");

  if (justificativa.size() < 15 or justificativa.size() > 200) {
    QMessageBox::critical(this, "Erro!", "Justificativa fora do tamanho!");
    return;
  }

  const int idNFe = model.data(list.first().row(), "idNFe").toInt();
  const int numeroNFe = model.data(list.first().row(), "NFe").toInt();

  const QString chaveAcesso = model.data(list.first().row(), "chaveAcesso").toString();

  const QString comando = "NFE.CancelarNFe(" + chaveAcesso + ", " + justificativa + ")";

  QString resposta;
  if (not ACBr::enviarComando(comando, resposta)) return;

  // REFAC: if not contains return?

  if (resposta.contains("xEvento=Cancelamento registrado")) {
    //  if (resposta.contains("XMotivo=Evento registrado e vinculado a NF-e")) {
    QSqlQuery query;
    query.prepare("UPDATE nfe SET status = 'CANCELADO' WHERE chaveAcesso = :chaveAcesso");
    query.bindValue(":chaveAcesso", chaveAcesso);

    if (not query.exec()) QMessageBox::critical(this, "Erro!", "Erro marcando nota como cancelada: " + query.lastError().text());
  }

  QMessageBox::information(this, "Resposta", resposta);

  QFile arquivo(QDir::currentPath() + "/cancelamento.xml");

  if (not arquivo.open(QFile::WriteOnly)) {
    QMessageBox::critical(this, "Erro!", "Erro abrindo arquivo para escrita: " + arquivo.errorString());
    return;
  }

  QTextStream stream(&arquivo);

  stream << resposta;

  arquivo.close();

  QSqlQuery query;
  query.prepare("UPDATE venda_has_produto SET status = 'ENTREGA AGEND.', idNFeSaida = NULL WHERE idNFeSaida = :idNFe");
  query.bindValue(":idNFe", idNFe);

  if (not query.exec()) {
    QMessageBox::critical(this, "Erro!", "Erro removendo NFe da venda_produto: " + query.lastError().text());
  }

  query.prepare("UPDATE veiculo_has_produto SET status = 'ENTREGA AGEND.', idNFeSaida = NULL WHERE idNFeSaida = :idNFe");
  query.bindValue(":idNFe", idNFe);

  if (not query.exec()) {
    QMessageBox::critical(this, "Erro!", "Erro removendo NFe do veiculo_produto: " + query.lastError().text());
  }

  const QString anexo = QDir::currentPath() + "/cancelamento.xml";

  // TODO: 0enviar resposta xml do cancelamento para a contabilidade
  auto *mail = new SendMail(SendMail::Tipo::CancelarNFe, anexo, QString(), this);
  mail->setAttribute(Qt::WA_DeleteOnClose);

  mail->exec();
}

// TODO: 1verificar se ao cancelar nota ela é removida do venda_produto/veiculo_produto
// TODO: 1botao para gerar relatorio igual ao da receita

void WidgetNfeSaida::on_pushButtonRelatorio_clicked() {
  // TODO: 3formatar decimais no padrao BR
  // TODO: 3perguntar um intervalo de tempo para filtrar as notas
  // TODO: 3verificar quais as tags na nota dos campos que faltam preencher

  if (not ui->groupBoxMes->isChecked()) {
    QMessageBox::critical(this, "Erro!", "Selecione um mês para gerar o relatório!");
    return;
  }

  LimeReport::ReportEngine report;

  SqlTableModel view;
  view.setTable("view_relatorio_nfe");
  view.setFilter("DATE_FORMAT(`Criado em`, '%Y-%m') = '" + ui->dateEdit->date().toString("yyyy-MM") + "' AND (status = 'AUTORIZADO')");

  if (not view.select()) {
    QMessageBox::critical(this, "Erro!", "Erro comunicando com banco de dados: " + view.lastError().text());
    return;
  }

  report.dataManager()->addModel("view", &view, true);

  if (not report.loadFromFile("view.lrxml")) {
    QMessageBox::critical(nullptr, "Erro!", "Não encontrou o modelo de impressão!");
    return;
  }

  QSqlQuery query;
  query.prepare("SELECT SUM(icms), SUM(icmsst), SUM(frete), SUM(totalnfe), SUM(desconto), SUM(impimp), SUM(ipi), "
                "SUM(cofins), SUM(0), SUM(0), SUM(seguro), SUM(pis), SUM(0) FROM view_relatorio_nfe WHERE DATE_FORMAT(`Criado "
                "em`, '%Y-%m') = :data AND (status = 'AUTORIZADO')");
  query.bindValue(":data", ui->dateEdit->date().toString("yyyy-MM"));

  if (not query.exec() or not query.first()) {
    QMessageBox::critical(this, "Erro!", "Erro buscando dados: " + query.lastError().text());
    return;
  }

  report.dataManager()->setReportVariable("TotalIcms", "R$ " + QString::number(query.value("sum(icms)").toDouble(), 'f', 2));
  report.dataManager()->setReportVariable("TotalIcmsSt", "R$ " + QString::number(query.value("sum(icmsst)").toDouble(), 'f', 2));
  report.dataManager()->setReportVariable("TotalFrete", "R$ " + QString::number(query.value("sum(frete)").toDouble(), 'f', 2));
  report.dataManager()->setReportVariable("TotalNfe", "R$ " + QString::number(query.value("sum(totalnfe)").toDouble(), 'f', 2));
  report.dataManager()->setReportVariable("TotalDesconto", "R$ " + QString::number(query.value("sum(desconto)").toDouble(), 'f', 2));
  report.dataManager()->setReportVariable("TotalImpImp", "R$ " + QString::number(query.value("sum(impimp)").toDouble(), 'f', 2));
  report.dataManager()->setReportVariable("TotalIpi", "R$ " + QString::number(query.value("sum(ipi)").toDouble(), 'f', 2));
  report.dataManager()->setReportVariable("TotalCofins", "R$ " + QString::number(query.value("sum(cofins)").toDouble(), 'f', 2));
  report.dataManager()->setReportVariable("TotalPisSt", "R$ XXX");
  report.dataManager()->setReportVariable("TotalCofinsSt", "R$ XXX");
  report.dataManager()->setReportVariable("TotalSeguro", "R$ " + QString::number(query.value("sum(seguro)").toDouble(), 'f', 2));
  report.dataManager()->setReportVariable("TotalPis", "R$ " + QString::number(query.value("sum(pis)").toDouble(), 'f', 2));
  report.dataManager()->setReportVariable("TotalIssqn", "R$ XXX");

  if (not report.printToPDF(QDir::currentPath() + "relatorio.pdf")) {
    QMessageBox::critical(this, "Erro!", "Erro gerando relatório!");
    return;
  }

  if (not QDesktopServices::openUrl(QUrl::fromLocalFile(QDir::currentPath() + "relatorio.pdf"))) {
    QMessageBox::critical(this, "Erro!", "Erro abrindo arquivo: " + QDir::currentPath() + "relatorio.pdf'!");
    return;
  }
}

void WidgetNfeSaida::on_pushButtonExportar_clicked() {
  // TODO: 5zipar arquivos exportados com nome descrevendo mes/notas/etc

  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) {
    QMessageBox::critical(this, "Erro!", "Nenhum item selecionado!");
    return;
  }

  for (const auto &item : list) {
    // TODO: 3wrap this in a function so if connection to acbr is dropped the erp retries
    // quando enviar para o acbr guardar a nota com status 'pendente' para consulta na receita
    // quando conseguir consultar se a receita retornar que a nota nao existe lá apagar aqui
    // se ela existir lá verificar se consigo pegar o xml autorizado e atualizar a nota pendente

    if (model.data(item.row(), "status").toString() != "AUTORIZADO") continue;

    // pegar xml do bd e salvar em arquivo

    const QString chaveAcesso = model.data(item.row(), "chaveAcesso").toString();

    QSqlQuery query;
    query.prepare("SELECT xml FROM nfe WHERE chaveAcesso = :chaveAcesso");
    query.bindValue(":chaveAcesso", chaveAcesso);

    if (not query.exec() or not query.first()) {
      QMessageBox::critical(this, "Erro!", "Erro buscando xml: " + query.lastError().text());
      return;
    }

    QFile fileXml(UserSession::settings("User/EntregasXmlFolder").toString() + "/" + chaveAcesso + ".xml");

    if (not fileXml.open(QFile::WriteOnly)) {
      QMessageBox::critical(this, "Erro!", "Erro abrindo arquivo para escrita xml: " + fileXml.errorString());
      return;
    }

    fileXml.write(query.value("xml").toByteArray());

    fileXml.flush();
    fileXml.close();

    // mandar xml para acbr gerar pdf

    QString pdfOrigem;
    if (not ACBr::gerarDanfe(query.value("xml").toByteArray(), pdfOrigem, false)) return;

    if (pdfOrigem.isEmpty()) {
      QMessageBox::critical(this, "Erro!", "pdf is empty!");
      return;
    }

    // copiar para pasta predefinida

    const QString pdfDestino = UserSession::settings("User/EntregasPdfFolder").toString() + "/" + chaveAcesso + ".pdf";

    QFile filePdf(pdfDestino);

    if (filePdf.exists()) filePdf.remove();

    if (not QFile::copy(pdfOrigem, pdfDestino)) {
      QMessageBox::critical(this, "Erro!", "Erro copiando pdf!");
      return;
    }
  }

  QMessageBox::information(this, "Aviso!", "Arquivos exportados com sucesso para " + UserSession::settings("User/EntregasPdfFolder").toString() + "!");
}

void WidgetNfeSaida::on_groupBoxStatus_toggled(const bool enabled) {
  for (auto const &child : ui->groupBoxStatus->findChildren<QCheckBox *>()) {
    child->setEnabled(true);
    child->setChecked(enabled);
  }
}

void WidgetNfeSaida::on_pushButtonConsultarNFe_clicked() {
  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) {
    QMessageBox::critical(this, "Erro!", "Nenhuma linha selecionada!");
    return;
  }

  //  if (model.data(selection.first().row(), "status").toString() != "NOTA PENDENTE") {
  //    QMessageBox::critical(this, "Erro!", "Nota não está pendente!");
  //    return;
  //  }

  const int idNFe = model.data(selection.first().row(), "idNFe").toInt();

  QSqlQuery query;
  query.prepare("SELECT xml FROM nfe WHERE idNFe = :idNFe");
  query.bindValue(":idNFe", idNFe);

  if (not query.exec() or not query.first()) {
    QMessageBox::critical(this, "Erro!", "Erro buscando XML: " + query.lastError().text());
    return;
  }

  QFile file(QDir::currentPath() + "/temp.xml");

  if (not file.open(QFile::WriteOnly)) {
    QMessageBox::critical(this, "Erro!", "Erro abrindo arquivo para escrita: " + file.errorString());
    return;
  }

  QTextStream stream(&file);

  stream << query.value("xml").toString();

  file.close();

  QString resposta;

  if (not ACBr::enviarComando("NFE.ConsultarNFe(" + file.fileName() + ")", resposta)) return;

  if (not resposta.contains("XMotivo=Autorizado o uso da NF-e")) {
    QMessageBox::critical(this, "Resposta ConsultarNFe", resposta);
    return;
  }

  QFile newFile(QDir::currentPath() + "/temp.xml");

  if (not newFile.open(QFile::ReadOnly)) {
    QMessageBox::critical(this, "Erro!", "Erro abrindo arquivo para leitura: " + newFile.errorString());
    return;
  }

  const QString xml = newFile.readAll();

  QSqlQuery("SET SESSION TRANSACTION ISOLATION LEVEL SERIALIZABLE").exec();
  QSqlQuery("START TRANSACTION").exec();

  if (not atualizarNFe(idNFe, xml)) {
    QSqlQuery("ROLLBACK").exec();
    if (not error.isEmpty()) QMessageBox::critical(this, "Erro!", error);
    return;
  }

  QSqlQuery("COMMIT").exec();

  QMessageBox::information(this, "Aviso!", resposta);
}

bool WidgetNfeSaida::atualizarNFe(const int idNFe, const QString &xml) {
  QSqlQuery query;
  query.prepare("UPDATE nfe SET status = 'AUTORIZADO', xml = :xml WHERE idNFe = :idNFe");
  query.bindValue(":xml", xml);
  query.bindValue(":idNFe", idNFe);

  if (not query.exec()) {
    error = "Erro marcando nota como 'AUTORIZADO': " + query.lastError().text();
    return false;
  }

  return true;
}

// TODO: guardar idVenda/outros dados na nfe para quando cancelar nao perder os vinculos
// TODO: 2tela para importar notas de amostra (aba separada)
// TODO: 0nao estou guardando o valor na nota
// TODO: 0algumas notas nao estao mostrando valor
// TODO: nesta tela colocar um campo dizendo qual loja que emitiu a nota (nao precisa mostrar o cnpj, apenas o nome da loja) (e talvez poder filtrar pela loja)

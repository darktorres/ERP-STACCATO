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
#include <QScrollBar>
#include <QSet>
#include <QSqlError>

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
  connect(ui->table->verticalScrollBar(), &QScrollBar::valueChanged, this, &WidgetNfeSaida::onTableScrolled, connectionType);
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
  disconnect(ui->table->verticalScrollBar(), &QScrollBar::valueChanged, this, &WidgetNfeSaida::onTableScrolled);
}

void WidgetNfeSaida::updateTables() {
  if (not isSet) {
    ui->lineEditBusca->setDelayed();
    ui->dateEditAte->setDate(qApp->serverDate());
    ui->dateEditDe->setDate(QDate(qApp->serverDate().year(), qApp->serverDate().month(), 1));
    setupTables(); // ja chama montaFiltro() para popular as colunas do model
    setConnections();
    isSet = true;
  } else {
    montaFiltro();
  }

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

  ui->table->setModel(&model);

  montaFiltro(); // monta e executa a 1a pagina, para popular as colunas do model antes de configurar a tabela

  model.setHeaderLabel("valor", "R$");
  model.setHeaderLabel("dataHoraEmissao", "Data");
  model.setHeaderLabel("dataFollowup", "Data Followup");
  model.setHeaderLabel("observacao", "Observação");

  ui->table->hideColumn("idNFe");
  ui->table->hideColumn("chaveAcesso");

  ui->table->setItemDelegate(new DoubleDelegate(this));

  // "R$" (não "valor"): o model paginado não é QSqlQueryModel, então TableView resolve a coluna via
  // headerData() (o rótulo renomeado acima), não via record() (nome cru da coluna SQL)
  ui->table->setItemDelegateForColumn("R$", new ReaisDelegate(this));

  // ----------------------------------------------------

  modelResumo.setQuery("SELECT status AS Status, COUNT(*) AS `` FROM nfe n WHERE tipo = 'SAÍDA' GROUP BY status");

  ui->tableResumo->setModel(&modelResumo);

  ui->tableResumo->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  ui->tableResumo->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

}

void WidgetNfeSaida::onTableScrolled(const int value) {
  if (carregandoPagina) { return; }

  carregandoPagina = true;

  try {
    auto *scrollBar = ui->table->verticalScrollBar();
    const int threshold = ui->table->verticalHeader()->defaultSectionSize() * 5;

    // ver nota em WidgetFinanceiroContas::onTableScrolled: o QTableView nao reancora o scrollbar
    // quando linhas entram/saem acima do viewport
    int deslocamento = 0;

    if (value >= scrollBar->maximum() - threshold) { deslocamento += model.tryLoadNext(); }
    if (value <= threshold) { deslocamento += model.tryLoadPrevious(); }

    if (deslocamento != 0) {
      // O QTableView adia o layout apos inserir/remover linhas (rowCountChanged -> doDelayedItemsLayout),
      // entao a faixa do scrollbar ainda seria a antiga aqui e o setValue poderia ser recortado nela
      ui->table->executarLayoutPendente();
      scrollBar->setValue(scrollBar->value() + deslocamento);
    }
  } catch (...) {
    carregandoPagina = false;
    throw;
  }

  carregandoPagina = false;
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

  QStringList filtrosPre; // entram na subquery de corte, antes do JOIN

  //------------------------------------- filtro texto
  // Vai junto no corte, nao num HAVING externo: assim o corte devolve direto as ~1000 linhas que
  // casam, em vez de o model varrer o historico janela por janela. Cliente/CPF-CNPJ entram por um
  // "IN (subquery)" NAO correlacionado - o MySQL materializa o conjunto de idVenda uma unica vez
  // (~117ms) em vez de consultar cliente por linha de nfe.

  const QString text = qApp->escaparBusca(ui->lineEditBusca->text());
  const bool temBusca = not text.isEmpty();

  if (temBusca) {
    filtrosPre << "(n.numeroNFe LIKE '%" + text + "%' OR n.idVenda LIKE '%" + text +
                      "%' OR n.idVenda IN (SELECT v.idVenda FROM venda v JOIN cliente c ON c.idCliente = v.idCliente WHERE c.nome_razao LIKE '%" + text +
                      "%' OR IF(c.pfpj = 'PF', c.cpf, c.cnpj) LIKE '%" + text + "%'))";
  }

  //------------------------------------- filtro data (comparacao direta na coluna, sem funcao, para poder usar indice)

  if (ui->groupBoxMes->isChecked()) {
    filtrosPre << "n.dataHoraEmissao >= '" + ui->dateEditDe->date().toString("yyyy-MM-dd") + " 00:00:00' AND n.dataHoraEmissao < '" + ui->dateEditAte->date().addDays(1).toString("yyyy-MM-dd") + " 00:00:00'";
  }

  //------------------------------------- filtro status

  QStringList filtroCheck;

  const auto children = ui->groupBoxStatus->findChildren<QCheckBox *>(QRegularExpression("checkBox"));

  for (const auto &child : children) {
    if (child->isChecked()) { filtroCheck << "'" + child->text().toUpper() + "'"; }
  }

  if (not filtroCheck.isEmpty()) { filtrosPre << "n.status IN (" + filtroCheck.join(", ") + ")"; }

  //------------------------------------- colunas e expressao SQL de cada uma (pra ORDER BY/keyset da paginacao)
  // -- CPF/CNPJ e Cliente exigem o join ate cliente (via venda) tambem na subquery de corte (indice
  // nfe(tipo, idVenda) cobre); dataFollowup/observacao exigem o join ate nfe_has_followup -- em ambos
  // os casos a coluna de junção (idVenda/idFollowup) NÃO está coberta por nenhum índice, então o corte
  // busca a linha inteira mesmo (mais lento, ~100-800ms, aceitável só pra essas 2 ações explícitas de
  // clique); as demais colunas ficam só em nfe (ou nfe+resumo em Entrada), sem precisar desses joins.

  static const QStringList fieldNames = {"idNFe", "chaveAcesso", "Emitente", "NFe", "Status", "Venda", "CPF/CNPJ", "Cliente", "valor", "dataHoraEmissao", "dataFollowup", "observacao"};

  static const QHash<QString, QString> exprPorCampo = {
      {"idNFe", "n.idNFe"},         {"chaveAcesso", "n.chaveAcesso"},          {"Emitente", "n.cnpjOrig"}, {"NFe", "n.numeroNFe"},
      {"Status", "n.status"},       {"Venda", "n.idVenda"},                   {"CPF/CNPJ", "IF(c.pfpj = 'PF', c.cpf, c.cnpj)"}, {"Cliente", "c.nome_razao"},
      {"valor", "n.valor"},         {"dataHoraEmissao", "n.dataHoraEmissao"}, {"dataFollowup", "nhf.dataFollowup"}, {"observacao", "nhf.observacao"},
  };

  static const QSet<QString> colunasComJoinCliente = {"CPF/CNPJ", "Cliente"};
  static const QSet<QString> colunasComJoinFollowup = {"dataFollowup", "observacao"};

  const QStringList filtrosPreCopia = filtrosPre;

  const SqlPaginatedModel::QueryBuilderFactory factory = [filtrosPreCopia, temBusca](const QString &sortColumn, const Qt::SortOrder order) -> SqlPaginatedModel::PageQueryBuilder {
    const QString sortExpr = exprPorCampo.value(sortColumn, "n.dataHoraEmissao");
    const bool precisaJoinCliente = colunasComJoinCliente.contains(sortColumn);
    const bool precisaJoinFollowup = colunasComJoinFollowup.contains(sortColumn);

    return [filtrosPreCopia, temBusca, sortExpr, order, precisaJoinCliente, precisaJoinFollowup](const SqlPaginatedModel::PageRequest &request) -> QString {
      const bool forward = request.direction != SqlPaginatedModel::Direction::Previous;

      QStringList capFiltros;
      capFiltros << "n.tipo = 'SAÍDA'";
      capFiltros += filtrosPreCopia;

      const QVector<SqlPaginatedModel::KeyExpr> keys = {{sortExpr, order}, {"n.idNFe", order}};

      if (request.direction != SqlPaginatedModel::Direction::First) { capFiltros << SqlPaginatedModel::buildKeysetWhere(keys, request.cursorValues, forward); }

      const QString capOrderBy = SqlPaginatedModel::buildOrderBy(keys, forward);

      const QString capJoinCliente = precisaJoinCliente ? " LEFT JOIN venda v ON (n.idVenda = v.idVenda) LEFT JOIN cliente c ON (c.idCliente = v.idCliente)" : "";
      const QString capJoinFollowup = precisaJoinFollowup ? " LEFT JOIN nfe_has_followup nhf ON (n.idFollowup = nhf.idFollowup)" : "";

      // FORCE INDEX: sem isso o otimizador as vezes escolhe um indice so de status (nao-covering,
      // bookmark lookup linha a linha) quando ha filtro de status sem filtro de data - mesmo problema
      // medido em Entrada (ver db/add_index_nfe_tipo_status_utilizada_data.sql). Ordenar por
      // Cliente/CPF-CNPJ precisa do indice de idVenda (ja validado antes, ver add_index_nfe_tipo_idvenda.sql).
      // Com busca ativa nenhum dos dois serve: eles nao cobrem numeroNFe/idVenda, entao o MySQL le a
      // linha inteira de `nfe` (tabela de ~2,9 GB por causa da coluna xml) - medido 5,6s com o indice
      // forcado contra 0,8s deixando o otimizador escolher.
      const QString forceIndex = temBusca ? "" : (precisaJoinCliente ? " FORCE INDEX (idx_nfe_tipo_idvenda)" : " FORCE INDEX (idx_nfe_tipo_status_utilizada_data)");

      const QString capSql = "SELECT n.idNFe AS idNFe FROM nfe n" + forceIndex + capJoinFollowup + capJoinCliente + " WHERE " + capFiltros.join(" AND ") + " ORDER BY " + capOrderBy + " LIMIT " +
                             QString::number(1000);

      const QString exibicaoOrderBy = SqlPaginatedModel::buildOrderBy(keys, true); // exibicao sempre na ordem normal

      return "SELECT n.idNFe AS idNFe, n.chaveAcesso AS chaveAcesso, n.cnpjOrig AS Emitente, n.numeroNFe AS NFe, n.status AS Status, "
             "n.idVenda AS Venda, IF(c.pfpj = 'PF', c.cpf, c.cnpj) AS `CPF/CNPJ`, c.nome_razao AS Cliente, "
             "n.valor AS valor, n.dataHoraEmissao AS dataHoraEmissao, nhf.dataFollowup AS dataFollowup, nhf.observacao AS observacao "
             "FROM (" +
             capSql +
             ") lim "
             "JOIN nfe n ON n.idNFe = lim.idNFe "
             "LEFT JOIN venda v ON (n.idVenda = v.idVenda) "
             "LEFT JOIN cliente c ON (c.idCliente = v.idCliente) "
             "LEFT JOIN nfe_has_followup nhf ON (n.idFollowup = nhf.idFollowup) "
             "GROUP BY n.idNFe ORDER BY " +
             exibicaoOrderBy;
    };
  };

  const QString sortColumnAtual = model.sortColumn().isEmpty() ? "dataHoraEmissao" : model.sortColumn();

  model.reset(fieldNames, "idNFe", sortColumnAtual, model.sortOrder(), {}, factory);

  // Mantem a seta do cabecalho coerente com a ordenacao que o model realmente aplicou. O QHeaderView
  // nasce apontando pra secao 0 (DESC) e so muda por clique do usuario, entao sem isso ele mente ja
  // na 1a abertura. Nao dispara recarga: SqlPaginatedModel::sort() sai cedo quando a ordem ja e essa.
  const int secaoOrdenada = model.fieldIndex(model.sortColumn(), true);

  if (secaoOrdenada != -1) { ui->table->horizontalHeader()->setSortIndicator(secaoOrdenada, model.sortOrder()); }
}

void WidgetNfeSaida::on_pushButtonCancelarNFe_clicked() {
  // TODO: como no cancelamento o acbr nao atualiza o xml, fazer a consulta para atualizar o xml como cancelado antes de enviar para a contabilidade?

  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!", this); }

  // chave e id resolvidos ANTES dos diálogos: o model é paginado (janela deslizante), então
  // index.row() só identifica uma linha enquanto a janela não for recarregada — e daqui até o envio
  // rodam DOIS event loops aninhados (msgBox.exec() e QInputDialog::getText), em que um timer
  // (busca com atraso, reconexão do banco) pode disparar montaFiltro() e trocar as linhas debaixo
  // do índice. Cancelar a NF-e errada na SEFAZ é irreversível.
  const QString chaveAcesso = model.data(selection.first().row(), "chaveAcesso").toString();
  const QVariant idNFe = model.data(selection.first().row(), "idNFe");

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

  ACBr acbr;

  const QString resposta = acbr.enviarComando("NFE.CancelarNFe(" + chaveAcesso + ", " + justificativa + ")", "Cancelando NF-e...");

  // TODO: verificar outras possiveis respostas (tinha algo como 'cancelamento registrado fora do prazo')
  if (not resposta.contains("xEvento=Cancelamento registrado", Qt::CaseInsensitive)) { throw RuntimeException("Resposta: " + resposta); }

  qApp->startTransaction("WidgetNfeSaida::on_pushButtonCancelarNFe");

  cancelarNFe(chaveAcesso, idNFe);

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

void WidgetNfeSaida::cancelarNFe(const QString &chaveAcesso, const QVariant &idNFe) {
  SqlQuery query;
  query.prepare("UPDATE nfe SET status = 'CANCELADA' WHERE chaveAcesso = :chaveAcesso");
  query.bindValue(":chaveAcesso", chaveAcesso);

  if (not query.exec()) { throw RuntimeException("Erro marcando NF-e como cancelada: " + query.lastError().text()); }

  // ---------------------------------------------------------

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

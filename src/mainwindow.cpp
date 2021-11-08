#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "application.h"
#include "cadastrocliente.h"
#include "cadastrofornecedor.h"
#include "cadastroloja.h"
#include "cadastroncm.h"
#include "cadastropagamento.h"
#include "cadastroproduto.h"
#include "cadastroprofissional.h"
#include "cadastrostaccatooff.h"
#include "cadastrotransportadora.h"
#include "cadastrousuario.h"
#include "calculofrete.h"
#include "importaprodutos.h"
#include "importatabelaibpt.h"
#include "orcamento.h"
#include "precoestoque.h"
#include "user.h"
#include "userconfig.h"

#include <QSqlError>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);

  setWindowIcon(QIcon("Staccato.ico"));
  setWindowTitle("ERP Staccato");

  auto *shortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q), this);
  connect(shortcut, &QShortcut::activated, this, &QWidget::close);

  const QString hostname = User::getSetting("Login/hostname").toString();

  if (hostname.isEmpty()) { throw RuntimeException("A chave 'hostname não está configurada!'"); }

  const QString hostnameText = qApp->getMapLojas().key(hostname);

  setWindowTitle(windowTitle() + " - " + User::nome + " - " + User::tipo + " - " + (hostnameText.isEmpty() ? hostname : hostnameText));

  if (not User::isAdmin()) { ui->actionCadastrarUsuario->setDisabled(true); }

  if (not User::isAdministrativo()) {
    ui->actionGerenciar_Lojas->setDisabled(true);
    ui->actionGerenciar_pagamentos->setDisabled(true);
    ui->actionGerenciar_Transportadoras->setDisabled(true);
    ui->actionCadastrarFornecedor->setDisabled(true);
    ui->actionCadastrarProdutos->setDisabled(true);
    ui->actionGerenciar_preco_estoque->setDisabled(true);
    ui->actionGerenciar_NCMs->setDisabled(true);
    ui->actionGerenciar_staccatoOff->setDisabled(true);

    ui->menuImportar_tabela_fornecedor->setDisabled(true);
    ui->actionImportar_tabela_IBPT->setDisabled(true);
  }

  ui->actionCalcular_frete->setDisabled(true);

  // -------------------------------------------------------------------------

  SqlQuery query;
  query.prepare("SELECT * FROM usuario_has_permissao WHERE idUsuario = :idUsuario");
  query.bindValue(":idUsuario", User::idUsuario);

  if (not query.exec()) { throw RuntimeException("Erro lendo permissões: " + query.lastError().text(), this); }

  if (not query.first()) { throw RuntimeException("Permissões não encontradas para usuário com id: " + User::idUsuario, this); }

  ui->tabWidget->setTabEnabled(ui->tabWidget->indexOf(ui->tabOrcamentos), query.value("view_tab_orcamento").toBool());
  ui->tabWidget->setTabEnabled(ui->tabWidget->indexOf(ui->tabVendas), query.value("view_tab_venda").toBool());
  ui->tabWidget->setTabEnabled(ui->tabWidget->indexOf(ui->tabCompras), query.value("view_tab_compra").toBool());
  ui->tabWidget->setTabEnabled(ui->tabWidget->indexOf(ui->tabLogistica), query.value("view_tab_logistica").toBool());
  ui->tabWidget->setTabEnabled(ui->tabWidget->indexOf(ui->tabNFe), query.value("view_tab_nfe").toBool());
  ui->tabWidget->setTabEnabled(ui->tabWidget->indexOf(ui->tabEstoque), query.value("view_tab_estoque").toBool());
  ui->tabWidget->setTabEnabled(ui->tabWidget->indexOf(ui->tabGalpao), query.value("view_tab_galpao").toBool());
  ui->tabWidget->setTabEnabled(ui->tabWidget->indexOf(ui->tabFinanceiro), query.value("view_tab_financeiro").toBool());
  ui->tabWidget->setTabEnabled(ui->tabWidget->indexOf(ui->tabRelatorios), query.value("view_tab_relatorio").toBool());
  ui->tabWidget->setTabEnabled(ui->tabWidget->indexOf(ui->tabGraficos), query.value("view_tab_grafico").toBool());
  ui->tabWidget->setTabEnabled(ui->tabWidget->indexOf(ui->tabRh), query.value("view_tab_rh").toBool());

  // -------------------------------------------------------------------------

  pushButtonStatus = new QPushButton(this);
  pushButtonStatus->setIcon(QIcon(":/reconnect.png"));
  pushButtonStatus->setText("Conectado: " + User::getSetting("Login/hostname").toString());
  pushButtonStatus->setStyleSheet("color: rgb(0, 190, 0);");

  ui->statusBar->addWidget(pushButtonStatus);

  connect(pushButtonStatus, &QPushButton::clicked, this, &MainWindow::reconnectDb);

  //---------------------------------------------------------------------------

  ui->tabWidget->setTabEnabled(ui->tabWidget->indexOf(ui->tabConsistencia), false);

  const QString nomeUsuario = User::nome;

  if (nomeUsuario == "ADMINISTRADOR" or nomeUsuario == "EDUARDO OLIVEIRA" or nomeUsuario == "RODRIGO TORRES") { ui->tabWidget->setTabEnabled(ui->tabWidget->indexOf(ui->tabConsistencia), true); }

  //---------------------------------------------------------------------------

  setConnections();

  //---------------------------------------------------------------------------

  return;

  SqlQuery queryGare;

  //  if (not queryGare.exec("select * from nfe where tipo = 'ENTRADA' and created > '2021-01-01' AND utilizada = TRUE")) { throw RuntimeError("erro buscando nfes: " + queryGare.lastError().text()); }
  //  if (not queryGare.exec("SELECT * FROM nfe n LEFT JOIN conta_a_pagar_has_pagamento chp ON n.idNFe = chp.idNFe WHERE n.tipo = 'ENTRADA' AND n.created > '2021-01-01' AND n.utilizada = TRUE AND "
  //                         "chp.grupo = 'IMPOSTOS - ICMS;ST;ISS' AND chp.status IN ('PENDENTE GARE', 'LIBERADO GARE', 'GERADO GARE')")) {
  if (not queryGare.exec("select * from nfe where idNFe = 53191")) { throw RuntimeError("erro buscando nfes: " + queryGare.lastError().text()); }

  qApp->startTransaction("correção gares");

  while (queryGare.next()) {
    XML xml = XML(queryGare.value("xml").toByteArray());

    const int id = queryGare.value("idNFe").toInt();

    const double valorAntigo = qApp->roundDouble(queryGare.value("gare").toDouble());
    const double valorNovo = qApp->roundDouble(calculaGare(xml));

    //    qDebug() << "idNFe: " << id;
    //    qDebug() << "old gare: " << valorAntigo;
    //    qDebug() << "new gare: " << valorNovo;

    //        if (not qFuzzyCompare(1.0 + valorAntigo, 1.0 + valorNovo)) {
    //   if (not qFuzzyCompare(valorAntigo, valorNovo)) {
    if (qAbs(valorAntigo - valorNovo) > 1) {
      qDebug() << "idNFe: " << id;
      qDebug() << "old gare: " << valorAntigo;
      qDebug() << "new gare: " << valorNovo;

      SqlQuery queryNFe;

      if (not queryNFe.exec("UPDATE nfe SET gare = " + QString::number(valorNovo) + " WHERE idNFe = " + QString::number(id))) {
        throw RuntimeError("erro atualizando gare na nfe: " + queryNFe.lastError().text());
      }

      SqlQuery queryConta;

      if (not queryConta.exec("UPDATE conta_a_pagar_has_pagamento SET valor = " + QString::number(valorNovo) + " WHERE idNFe = " + QString::number(id) + " AND grupo = 'IMPOSTOS - ICMS;ST;ISS'")) {
        throw RuntimeError("erro atualizando gare na conta: " + queryConta.lastError().text());
      }
    }
  }

  qApp->endTransaction();
}

MainWindow::MainWindow() : MainWindow(nullptr) {}

MainWindow::~MainWindow() { delete ui; }

double MainWindow::calculaGare(XML &xml) {
  double total = 0;

  for (auto &produto : xml.produtos) {
    const MainWindow::NCM ncm = buscaNCM(xml, produto.ncm);

    // CST 00 Tributada integralmente (pago ao governo)
    // CST 10 Tributada e com cobrança do ICMS por substituição tributária
    // CST 60 ICMS cobrado anteriormente por substituição tributária (pago ao fornecedor)

    if (produto.tipoICMS != "ICMS00") { continue; }

    const double icmsIntra = ncm.aliq;
    const double mva = (qFuzzyCompare(produto.pICMS, 4)) ? ncm.mva4 : ncm.mva12;
    const double baseCalculo = produto.valor + produto.vIPI + produto.outros + produto.frete + produto.seguro - produto.desconto;
    const double icmsProprio = produto.vICMS;
    const double baseST = (baseCalculo) * (1 + mva);
    double icmsST = (baseST * icmsIntra) - icmsProprio;
    icmsST = qMax(icmsST, 0.);

    total += icmsST;

    produto.valorGare = icmsST;

    //    qDebug() << "baseCalculo: " << baseCalculo;
    //    qDebug() << "mvaAjustado: " << mva;
    //    qDebug() << "icmsIntra: " << icmsIntra;
    //    qDebug() << "icmsProprio: " << icmsProprio;
    //    qDebug() << "baseST: " << baseST;
    //    qDebug() << "icmsST: " << icmsST;
  }

  //  qDebug() << "new gare: " << total;

  return total;
}

MainWindow::NCM MainWindow::buscaNCM(const XML &xml, const QString &ncm) {
  SqlQuery query;

  if (not query.exec("SELECT * FROM ncm WHERE ncm = '" + ncm + "'")) { throw RuntimeException("Erro buscando ncm " + ncm + ": " + query.lastError().text()); }

  //  if (not query.first()) { throw RuntimeError("NCM " + ncm + " não cadastrado!"); }
  if (not query.first()) {
    //    qDebug() << "NFe: " << xml.nNF << ", NCM " + ncm + " não cadastrado!";
    qDebug() << "'" << xml.nNF << "'";
    return NCM{0, 0, 0};
  }

  return NCM{query.value("mva4").toDouble() / 100, query.value("mva12").toDouble() / 100, query.value("aliq").toDouble() / 100};
}

void MainWindow::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(qApp, &Application::setConnectionStatus, this, &MainWindow::setConnectionStatus, connectionType);
  connect(ui->actionCadastrarCliente, &QAction::triggered, this, &MainWindow::on_actionCadastrarCliente_triggered, connectionType);
  connect(ui->actionCadastrarFornecedor, &QAction::triggered, this, &MainWindow::on_actionCadastrarFornecedor_triggered, connectionType);
  connect(ui->actionCadastrarProdutos, &QAction::triggered, this, &MainWindow::on_actionCadastrarProdutos_triggered, connectionType);
  connect(ui->actionCadastrarProfissional, &QAction::triggered, this, &MainWindow::on_actionCadastrarProfissional_triggered, connectionType);
  connect(ui->actionCadastrarUsuario, &QAction::triggered, this, &MainWindow::on_actionCadastrarUsuario_triggered, connectionType);
  connect(ui->actionCalculadora, &QAction::triggered, this, &MainWindow::on_actionCalculadora_triggered, connectionType);
  connect(ui->actionCalcular_frete, &QAction::triggered, this, &MainWindow::on_actionCalcular_frete_triggered, connectionType);
  connect(ui->actionClaro, &QAction::triggered, this, &MainWindow::on_actionClaro_triggered, connectionType);
  connect(ui->actionConfiguracoes, &QAction::triggered, this, &MainWindow::on_actionConfiguracoes_triggered, connectionType);
  connect(ui->actionCriarOrcamento, &QAction::triggered, this, &MainWindow::on_actionCriarOrcamento_triggered, connectionType);
  connect(ui->actionEscuro, &QAction::triggered, this, &MainWindow::on_actionEscuro_triggered, connectionType);
  connect(ui->actionGerenciar_Lojas, &QAction::triggered, this, &MainWindow::on_actionGerenciar_Lojas_triggered, connectionType);
  connect(ui->actionGerenciar_NCMs, &QAction::triggered, this, &MainWindow::on_actionGerenciar_NCMs_triggered, connectionType);
  connect(ui->actionGerenciar_Transportadoras, &QAction::triggered, this, &MainWindow::on_actionGerenciar_Transportadoras_triggered, connectionType);
  connect(ui->actionGerenciar_pagamentos, &QAction::triggered, this, &MainWindow::on_actionGerenciar_Pagamentos_triggered, connectionType);
  connect(ui->actionGerenciar_preco_estoque, &QAction::triggered, this, &MainWindow::on_actionGerenciar_preco_estoque_triggered, connectionType);
  connect(ui->actionGerenciar_staccatoOff, &QAction::triggered, this, &MainWindow::on_actionGerenciar_staccatoOff_triggered, connectionType);
  connect(ui->actionImportar_tabela_IBPT, &QAction::triggered, this, &MainWindow::on_actionImportar_tabela_IBPT_triggered, connectionType);
  connect(ui->actionProdutos, &QAction::triggered, this, &MainWindow::on_actionProdutos_triggered, connectionType);
  connect(ui->actionPromocao, &QAction::triggered, this, &MainWindow::on_actionPromocao_triggered, connectionType);
  connect(ui->actionSobre, &QAction::triggered, this, &MainWindow::on_actionSobre_triggered, connectionType);
  connect(ui->tabWidget, &QTabWidget::currentChanged, this, &MainWindow::on_tabWidget_currentChanged, connectionType);
}

void MainWindow::resetTables() {
  ui->widgetOrcamento->resetTables();
  ui->widgetVenda->resetTables();
  ui->widgetCompra->resetTables();
  ui->widgetLogistica->resetTables();
  ui->widgetNFe->resetTables();
  ui->widgetEstoque->resetTables();
  ui->widgetFinanceiro->resetTables();
  ui->widgetRelatorio->resetTables();
  ui->widgetGraficos->resetTables();
  ui->widgetConsistencia->resetTables();

  updateTables();
}

void MainWindow::updateTables() {
  if (qApp->getUpdating()) { return; }
  if (not qApp->getIsConnected()) { return; }
  if (qApp->getShowingMessages()) { return; }

  qApp->setUpdating(true);

  try {
    const QString currentTab = ui->tabWidget->tabText(ui->tabWidget->currentIndex());

    if (currentTab == "Orçamentos") { ui->widgetOrcamento->updateTables(); }
    if (currentTab == "Vendas") { ui->widgetVenda->updateTables(); }
    if (currentTab == "Compras") { ui->widgetCompra->updateTables(); }
    if (currentTab == "Logística") { ui->widgetLogistica->updateTables(); }
    if (currentTab == "NFe") { ui->widgetNFe->updateTables(); }
    if (currentTab == "Estoque") { ui->widgetEstoque->updateTables(); }
    if (currentTab == "Galpão") { ui->widgetGalpao->updateTables(); }
    if (currentTab == "Financeiro") { ui->widgetFinanceiro->updateTables(); }
    if (currentTab == "Relatórios") { ui->widgetRelatorio->updateTables(); }
    if (currentTab == "Gráfico") { ui->widgetGraficos->updateTables(); }
    //    if (currentTab == "RH") { ui->widgetRh->updateTables(); }
    if (currentTab == "Consistência") { ui->widgetConsistencia->updateTables(); }
  } catch (std::exception &) {
    qApp->setUpdating(false);
    throw;
  }

  qApp->setUpdating(false);
}

void MainWindow::reconnectDb() {
  const bool conectado = qApp->dbReconnect();

  setConnectionStatus(conectado);
}

void MainWindow::setConnectionStatus(const bool conectado) {
  pushButtonStatus->setText(conectado ? "Conectado: " + User::getSetting("Login/hostname").toString() : "Desconectado");
  pushButtonStatus->setStyleSheet(conectado ? "color: rgb(0, 190, 0);" : "color: rgb(255, 0, 0);");

  if (conectado) { resetTables(); }
}

bool MainWindow::event(QEvent *event) {
  switch (event->type()) {
  case QEvent::WindowActivate: updateTables(); break;

  default: break;
  }

  return QMainWindow::event(event);
}

void MainWindow::on_tabWidget_currentChanged() { updateTables(); }

void MainWindow::on_actionCriarOrcamento_triggered() {
  auto *orcamento = new Orcamento(this);
  orcamento->setAttribute(Qt::WA_DeleteOnClose);
  orcamento->show();
}

void MainWindow::on_actionCadastrarProdutos_triggered() {
  auto *cad = new CadastroProduto(this);
  cad->setAttribute(Qt::WA_DeleteOnClose);
  cad->show();
}

void MainWindow::on_actionCadastrarCliente_triggered() {
  auto *cad = new CadastroCliente(this);
  cad->setAttribute(Qt::WA_DeleteOnClose);
  cad->show();
}

void MainWindow::on_actionCadastrarUsuario_triggered() {
  auto *cad = new CadastroUsuario(this);
  cad->setAttribute(Qt::WA_DeleteOnClose);
  cad->show();
}

void MainWindow::on_actionCadastrarProfissional_triggered() {
  auto *cad = new CadastroProfissional(this);
  cad->setAttribute(Qt::WA_DeleteOnClose);
  cad->show();
}

void MainWindow::on_actionGerenciar_Transportadoras_triggered() {
  auto *cad = new CadastroTransportadora(this);
  cad->setAttribute(Qt::WA_DeleteOnClose);
  cad->show();
}

void MainWindow::on_actionGerenciar_Lojas_triggered() {
  auto *cad = new CadastroLoja(this);
  cad->setAttribute(Qt::WA_DeleteOnClose);
  cad->show();
}

void MainWindow::on_actionCadastrarFornecedor_triggered() {
  auto *cad = new CadastroFornecedor(this);
  cad->setAttribute(Qt::WA_DeleteOnClose);
  cad->show();
}

void MainWindow::on_actionSobre_triggered() {
  QMessageBox::about(this, "Sobre ERP Staccato", "Versão " + qApp->applicationVersion() + "\nDesenvolvedor: Rodrigo Torres\nCelular/WhatsApp: (12)98138-3504\nE-mail: torres.dark@gmail.com");
}

void MainWindow::on_actionClaro_triggered() { qApp->lightTheme(); }

void MainWindow::on_actionEscuro_triggered() { qApp->darkTheme(); }

void MainWindow::on_actionConfiguracoes_triggered() {
  auto *config = new UserConfig(this);
  config->setAttribute(Qt::WA_DeleteOnClose);
  config->show();
}

void MainWindow::on_actionCalculadora_triggered() { QDesktopServices::openUrl(QUrl::fromLocalFile(R"(C:\Windows\System32\calc.exe)")); }

void MainWindow::on_actionProdutos_triggered() {
  auto *importa = new ImportaProdutos(ImportaProdutos::Tipo::Normal, this);
  importa->setAttribute(Qt::WA_DeleteOnClose);
  importa->importarTabela();
}

void MainWindow::on_actionPromocao_triggered() {
  auto *importa = new ImportaProdutos(ImportaProdutos::Tipo::Promocao, this);
  importa->setAttribute(Qt::WA_DeleteOnClose);
  importa->importarTabela();
}

void MainWindow::on_actionGerenciar_preco_estoque_triggered() {
  auto *estoque = new PrecoEstoque(this);
  estoque->setAttribute(Qt::WA_DeleteOnClose);
  estoque->show();
}

void MainWindow::on_actionCalcular_frete_triggered() {
  auto *frete = new CalculoFrete(this);
  frete->setAttribute(Qt::WA_DeleteOnClose);
  frete->show();
}

void MainWindow::on_actionImportar_tabela_IBPT_triggered() {
  ImportaTabelaIBPT ibpt(this);
  ibpt.importar();
}

void MainWindow::on_actionGerenciar_NCMs_triggered() {
  auto *cadastroNCM = new CadastroNCM(this);
  cadastroNCM->setAttribute(Qt::WA_DeleteOnClose);
  cadastroNCM->show();
}

void MainWindow::on_actionGerenciar_Pagamentos_triggered() {
  auto *pagamentos = new CadastroPagamento(this);
  pagamentos->setAttribute(Qt::WA_DeleteOnClose);
  pagamentos->show();
}

void MainWindow::on_actionGerenciar_staccatoOff_triggered() {
  auto *promocao = new CadastroStaccatoOff(this);
  promocao->setAttribute(Qt::WA_DeleteOnClose);
  promocao->show();
}

// TODO: 0montar relatorio dos caminhoes com graficos e total semanal, mensal, custos etc
// NOTE: colocar logo da staccato na mainwindow

// NOTE: prioridades atuais:
// TODO: logistica da devolucao

// TODO: cancelamento de nfe: terminar de arrumar formato do email
// TODO: ao cancelar a nota verificar se todos os campos relacionados foram corrigidos e enviar email para contabilidade com xml de canc.
// TODO: verificar com Conrado os itens com minimo mas sem multiplo (tabela produto)
// TODO: caixinha na tabela 'agendar entrega' para marcar quais pedidos foram enviados pelo anderson para a edna
// TODO: arrumar consumos em que as unidades do estoque estejam diferentes das do consumo (converter)
// TODO: terminar de implantar quebra/reposicao
// TODO: quando muda a validade de um produto descontinuado ele continua descontinuado porque o sistema leva em consideracao o produto_has_preco
// NOTE: add logging to the terminal so qdebug logging can be seen on the user pc when needed

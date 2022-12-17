#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "application.h"
#include "cadastrocliente.h"
#include "cadastrofornecedor.h"
#include "cadastrofuncionario.h"
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
#include "qsimpleupdater.h"
#include "user.h"
#include "userconfig.h"

#include <QSqlError>

using namespace std::chrono_literals;

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

  if (not User::isAdmin()) { ui->actionCadastrarUsuario->setVisible(false); }

  if (not User::isAdministrativo()) {
    ui->actionCadastrarFornecedor->setVisible(false);
    ui->actionCadastrarProdutos->setVisible(false);
    ui->actionGerenciar_Lojas->setVisible(false);
    ui->actionGerenciar_NCMs->setVisible(false);
    ui->actionGerenciar_Transportadoras->setVisible(false);
    ui->actionGerenciar_dados_bancarios->setVisible(false);
    ui->actionGerenciar_pagamentos->setVisible(false);
    ui->actionGerenciar_preco_estoque->setVisible(false);
    ui->actionGerenciar_staccatoOff->setVisible(false);

    ui->menuImportar_tabela_fornecedor->menuAction()->setVisible(false);
    ui->actionImportar_tabela_IBPT->setVisible(false);
  }

  // -------------------------------------------------------------------------

  SqlQuery query;
  query.prepare("SELECT * FROM usuario_has_permissao WHERE idUsuario = :idUsuario");
  query.bindValue(":idUsuario", User::idUsuario);

  if (not query.exec()) { throw RuntimeException("Erro lendo permissões: " + query.lastError().text(), this); }

  if (not query.first()) { throw RuntimeException("Permissões não encontradas para usuário com id: '" + User::idUsuario + "'", this); }

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

  ui->actionCalcular_frete->setVisible(query.value("ajusteFrete").toBool());

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

  auto *timer = new QTimer(this);
  connect(timer, &QTimer::timeout, this, [&] { updater(); });
  timer->start(10min);

  //---------------------------------------------------------------------------

  setConnections();
}

MainWindow::MainWindow() : MainWindow(nullptr) {}

MainWindow::~MainWindow() { delete ui; }

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
  connect(ui->actionGerenciar_dados_bancarios, &QAction::triggered, this, &MainWindow::on_actionGerenciar_dados_bancarios_triggered, connectionType);
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
  const QString currentTab = ui->tabWidget->tabText(ui->tabWidget->currentIndex());

  if (currentTab == "Orçamentos") { ui->widgetOrcamento->resetTables(); }
  if (currentTab == "Vendas") { ui->widgetVenda->resetTables(); }
  if (currentTab == "Compras") { ui->widgetCompra->resetTables(); }
  if (currentTab == "Logística") { ui->widgetLogistica->resetTables(); }
  if (currentTab == "NF-e") { ui->widgetNFe->resetTables(); }
  if (currentTab == "Estoque") { ui->widgetEstoque->resetTables(); }
  if (currentTab == "Galpão") { ui->widgetGalpao->resetTables(); }
  if (currentTab == "Financeiro") { ui->widgetFinanceiro->resetTables(); }
  if (currentTab == "Relatórios") { ui->widgetRelatorio->resetTables(); }
  if (currentTab == "Gráfico") { ui->widgetGraficos->resetTables(); }
  //    if (currentTab == "RH") { ui->widgetRh->resetTables(); }
  if (currentTab == "Consistência") { ui->widgetConsistencia->resetTables(); }

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
    if (currentTab == "NF-e") { ui->widgetNFe->updateTables(); }
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

void MainWindow::reconnectDb() { qApp->dbReconnect(); }

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

void MainWindow::on_actionGerenciar_dados_bancarios_triggered() {
  auto funcionarios = new CadastroFuncionario(this);
  funcionarios->setAttribute(Qt::WA_DeleteOnClose);
  funcionarios->show();
}

void MainWindow::updater() {
#ifdef Q_OS_WIN
  if (updaterOpen) { return; }

  const QString hostname = User::getSetting("Login/hostname").toString();

  if (hostname.isEmpty()) { return; }

  updaterOpen = true;

  // TODO: abrir updater em um processo separado e fechar erp?

  auto *updater = new QSimpleUpdater(this);
  connect(updater, &QSimpleUpdater::done, [&] { updaterOpen = false; });
  connect(updater, &QSimpleUpdater::beginDownload, this, &MainWindow::close);
  updater->setApplicationVersion(qApp->applicationVersion());
  updater->setReferenceUrl("http://" + hostname + "/versao.txt");
  updater->setDownloadUrl("http://" + hostname + "/Instalador.exe");
  updater->setSilent(true);
  updater->setShowNewestVersionMessage(true);
  updater->checkForUpdates();
#endif
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

// TODO: QToolButton: para poder ter um pushButton com menu dropdown
// https://doc.qt.io/qt-5/qtoolbutton.html#details
// https://www.walletfox.com/course/customqtoolbutton.php

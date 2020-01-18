#include <QDesktopServices>
#include <QMessageBox>
#include <QShortcut>
#include <QSqlError>

#include "application.h"
#include "cadastrocliente.h"
#include "cadastrofornecedor.h"
#include "cadastroloja.h"
#include "cadastroproduto.h"
#include "cadastroprofissional.h"
#include "cadastrotransportadora.h"
#include "cadastrousuario.h"
#include "calculofrete.h"
#include "importaprodutos.h"
#include "mainwindow.h"
#include "orcamento.h"
#include "precoestoque.h"
#include "ui_mainwindow.h"
#include "userconfig.h"
#include "usersession.h"
#include "xlsxdocument.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);

  connect(qApp, &Application::verifyDb, this, &MainWindow::verifyDb);

  connect(ui->actionCadastrarCliente, &QAction::triggered, this, &MainWindow::on_actionCadastrarCliente_triggered);
  connect(ui->actionCadastrarFornecedor, &QAction::triggered, this, &MainWindow::on_actionCadastrarFornecedor_triggered);
  connect(ui->actionCadastrarProdutos, &QAction::triggered, this, &MainWindow::on_actionCadastrarProdutos_triggered);
  connect(ui->actionCadastrarProfissional, &QAction::triggered, this, &MainWindow::on_actionCadastrarProfissional_triggered);
  connect(ui->actionCadastrarUsuario, &QAction::triggered, this, &MainWindow::on_actionCadastrarUsuario_triggered);
  connect(ui->actionCalculadora, &QAction::triggered, this, &MainWindow::on_actionCalculadora_triggered);
  connect(ui->actionCalcular_frete, &QAction::triggered, this, &MainWindow::on_actionCalcular_frete_triggered);
  connect(ui->actionClaro, &QAction::triggered, this, &MainWindow::on_actionClaro_triggered);
  connect(ui->actionConfiguracoes, &QAction::triggered, this, &MainWindow::on_actionConfiguracoes_triggered);
  connect(ui->actionCriarOrcamento, &QAction::triggered, this, &MainWindow::on_actionCriarOrcamento_triggered);
  connect(ui->actionEscuro, &QAction::triggered, this, &MainWindow::on_actionEscuro_triggered);
  connect(ui->actionStaccatoOFF, &QAction::triggered, this, &MainWindow::on_actionStaccatoOFF_triggered);
  connect(ui->actionGerenciar_Lojas, &QAction::triggered, this, &MainWindow::on_actionGerenciar_Lojas_triggered);
  connect(ui->actionGerenciar_Transportadoras, &QAction::triggered, this, &MainWindow::on_actionGerenciar_Transportadoras_triggered);
  connect(ui->actionGerenciar_preco_estoque, &QAction::triggered, this, &MainWindow::on_actionGerenciar_preco_estoque_triggered);
  connect(ui->actionProdutos, &QAction::triggered, this, &MainWindow::on_actionProdutos_triggered);
  connect(ui->actionPromocao, &QAction::triggered, this, &MainWindow::on_actionPromocao_triggered);
  connect(ui->actionSobre, &QAction::triggered, this, &MainWindow::on_actionSobre_triggered);
  connect(ui->tabWidget, &QTabWidget::currentChanged, this, &MainWindow::on_tabWidget_currentChanged);

  setWindowIcon(QIcon("Staccato.ico"));
  setWindowTitle("ERP Staccato");

  QShortcut *shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), this);
  connect(shortcut, &QShortcut::activated, this, &QWidget::close);

  if (const auto hostname = UserSession::getSetting("Login/hostname"); hostname) {
    const QString hostnameText = qApp->getMapLojas().key(hostname->toString());

    setWindowTitle(windowTitle() + " - " + UserSession::nome() + " - " + UserSession::tipoUsuario() + " - " + (hostnameText.isEmpty() ? hostname->toString() : hostnameText));
  } else {
    qApp->enqueueError("A chave 'hostname' não está configurada!", this);
  }

  if (UserSession::tipoUsuario() != "ADMINISTRADOR") { ui->actionCadastrarUsuario->setDisabled(true); }

  if (UserSession::tipoUsuario() != "ADMINISTRADOR" and UserSession::tipoUsuario() != "ADMINISTRATIVO") {
    ui->actionCadastrarFornecedor->setDisabled(true);
    ui->actionCadastrarProdutos->setDisabled(true);
    ui->actionGerenciar_Lojas->setDisabled(true);
    ui->actionGerenciar_Transportadoras->setDisabled(true);
    ui->actionGerenciar_preco_estoque->setDisabled(true);
    ui->actionProdutos->setDisabled(true);
    ui->menuImportar_tabela_fornecedor->setDisabled(true);
  }

  // -------------------------------------------------------------------------

  QSqlQuery query;
  query.prepare("SELECT * FROM usuario_has_permissao WHERE idUsuario = :idUsuario");
  query.bindValue(":idUsuario", UserSession::idUsuario());

  if (query.exec() and query.first()) {
    ui->tabWidget->setTabEnabled(ui->tabWidget->indexOf(ui->tabOrcamentos), query.value("view_tab_orcamento").toBool());
    ui->tabWidget->setTabEnabled(ui->tabWidget->indexOf(ui->tabVendas), query.value("view_tab_venda").toBool());
    ui->tabWidget->setTabEnabled(ui->tabWidget->indexOf(ui->tabCompras), query.value("view_tab_compra").toBool());
    ui->tabWidget->setTabEnabled(ui->tabWidget->indexOf(ui->tabLogistica), query.value("view_tab_logistica").toBool());
    ui->tabWidget->setTabEnabled(ui->tabWidget->indexOf(ui->tabNFe), query.value("view_tab_nfe").toBool());
    ui->tabWidget->setTabEnabled(ui->tabWidget->indexOf(ui->tabEstoque), query.value("view_tab_estoque").toBool());
    ui->tabWidget->setTabEnabled(ui->tabWidget->indexOf(ui->tabFinanceiro), query.value("view_tab_financeiro").toBool());
    ui->tabWidget->setTabEnabled(ui->tabWidget->indexOf(ui->tabRelatorios), query.value("view_tab_relatorio").toBool());
    ui->tabWidget->setTabEnabled(ui->tabWidget->indexOf(ui->tabRh), query.value("view_tab_rh").toBool());
  } else {
    qApp->enqueueError("Erro lendo permissões: " + query.lastError().text(), this);
  }

  // -------------------------------------------------------------------------

  pushButtonStatus = new QPushButton(this);
  pushButtonStatus->setIcon(QIcon(":/reconnect.png"));
  pushButtonStatus->setText("Conectado: " + UserSession::getSetting("Login/hostname")->toString());
  pushButtonStatus->setStyleSheet("color: rgb(0, 190, 0);");

  ui->statusBar->addWidget(pushButtonStatus);

  connect(pushButtonStatus, &QPushButton::clicked, this, &MainWindow::reconnectDb);

  //---------------------------------------------------------------------------

  ui->tabWidget->setTabEnabled(ui->tabWidget->indexOf(ui->tabGraficos), false);

  const QString nomeUsuario = UserSession::nome();

  if (nomeUsuario == "ADMINISTRADOR" or nomeUsuario == "EDUARDO OLIVEIRA" or nomeUsuario == "GISELY OLIVEIRA" or UserSession::tipoUsuario() == "GERENTE LOJA") {
    ui->tabWidget->setTabEnabled(ui->tabWidget->indexOf(ui->tabGraficos), true);
  }

  if (nomeUsuario == "ADMINISTRADOR" or nomeUsuario == "EDUARDO OLIVEIRA") { ui->tabWidget->setTabEnabled(ui->tabWidget->indexOf(ui->tabConsistencia), true); }
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::reconnectDb() {
  const bool conectado = qApp->dbReconnect();

  verifyDb(conectado);
}

void MainWindow::verifyDb(const bool conectado) {
  pushButtonStatus->setText(conectado ? "Conectado: " + UserSession::getSetting("Login/hostname")->toString() : "Desconectado");
  pushButtonStatus->setStyleSheet(conectado ? "color: rgb(0, 190, 0);" : "color: rgb(255, 0, 0);");

  if (conectado) { resetTables(); }
}

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

void MainWindow::resetTables() {
  ui->widgetOrcamento->resetTables();
  ui->widgetVenda->resetTables();
  ui->widgetCompra->resetTables();
  ui->widgetLogistica->resetTables();
  ui->widgetNfe->resetTables();
  ui->widgetEstoque->resetTables();
  ui->widgetFinanceiro->resetTables();
  ui->widgetRelatorio->resetTables();
  ui->widgetGraficos->resetTables();

  updateTables();
}

void MainWindow::updateTables() {
  if (qApp->getUpdating()) { return; }
  if (not qApp->getIsConnected()) { return; }
  if (qApp->getShowingErrors()) { return; }

  qApp->setUpdating(true);

  const QString currentText = ui->tabWidget->tabText(ui->tabWidget->currentIndex());

  if (currentText == "Orçamentos") { ui->widgetOrcamento->updateTables(); }
  if (currentText == "Vendas") { ui->widgetVenda->updateTables(); }
  if (currentText == "Compras") { ui->widgetCompra->updateTables(); }
  if (currentText == "Logística") { ui->widgetLogistica->updateTables(); }
  if (currentText == "NFe") { ui->widgetNfe->updateTables(); }
  if (currentText == "Estoque") { ui->widgetEstoque->updateTables(); }
  if (currentText == "Financeiro") { ui->widgetFinanceiro->updateTables(); }
  if (currentText == "Relatórios") { ui->widgetRelatorio->updateTables(); }
  if (currentText == "Gráfico") { ui->widgetGraficos->updateTables(); }
  if (currentText == "RH") { ui->widgetRh->updateTables(); }
  if (currentText == "Consistência") { ui->widgetConsistencia->updateTables(); }

  qApp->setUpdating(false);
}

void MainWindow::on_actionCadastrarFornecedor_triggered() {
  auto *cad = new CadastroFornecedor(this);
  cad->setAttribute(Qt::WA_DeleteOnClose);
  cad->show();
}

bool MainWindow::event(QEvent *event) {
  switch (event->type()) {
  case QEvent::WindowActivate: updateTables(); break;

  default: break;
  }

  return QMainWindow::event(event);
}

void MainWindow::on_tabWidget_currentChanged(const int) { updateTables(); }

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
  auto *importa = new ImportaProdutos(ImportaProdutos::Tipo::Produto, this);
  importa->setAttribute(Qt::WA_DeleteOnClose);
  importa->importarTabela();
}

void MainWindow::on_actionStaccatoOFF_triggered() {
  auto *importa = new ImportaProdutos(ImportaProdutos::Tipo::StaccatoOFF, this);
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

// TODO: 0montar relatorio dos caminhoes com graficos e total semanal, mensal, custos etc
// NOTE: colocar logo da staccato na mainwindow

// NOTE: prioridades atuais:
// TODO: logistica da devolucao

// TASK: cancelamento de nfe: terminar de arrumar formato do email
// TASK: arrumar cadastrarNFe para quando guardar a nota pendente associar ela com venda_has_produto para aparecer na
// tela de consultarNFe (depois disso só vai precisar atualizar a nota com a
// autorizacao e os status)
// TASK: verificar porque os estoques 10649, 10650 e 10651 nao mudaram de status (pararam em 'em coleta')
// TASK: anotar alteracoes que Anderson pediu nos audios do whats
// TASK: ao cancelar a nota verificar se todos os campos relacionados foram corrigidos e enviar email para contabilidade com xml de canc.
// TASK: arrumar items no workbench na tabela pf que possuam idVendaProduto mas nao idVenda
// TASK: terminar a parte de alteracao de certificado
//         alterar emitente
//         pedir para alterar cartao
// TASK: -reescrever view_estoque para retroativo (usar view_estoque2)
// TASK: botao de consultarNFe nao esta atualizando corretamente o xml
// TASK: protocolo entrega (falta o Anderson validar antes de integrar com a geracao da nota)
// TASK: verificar com Conrado os itens com minimo mas sem multiplo (tabela produto)
// TASK: verificar load balancing com proxysql
// TASK: montar chart do faturamento dia/mes (foto no skype)
// TASK: caixinha na tabela 'agendar entrega' para marcar quais pedidos foram enviados pelo anderson para a edna
// TASK: arrumar consumos em que as unidades do estoque estejam diferentes das do consumo (converter)
// TASK: pendencias conrado - nfe 118248
// TASK: terminar funcao de marcar caixas quebradas no recebimento
// TASK: verificar os 3 pedidos com totalItem que nao corresponde ao valor correto
// TASK: bloquear acesso dos usuarios apenas pela intranet (permissoes mysql) precisa de ip fixo primeiro
// TASK: quando muda a validade de um produto descontinuado ele continua descontinuado porque o sistema leva em consideracao o produto_has_preco
// TASK: alterar consumo de estoque para fazer as ligacoes idVenda/idVendaProduto na tabela de compra
// TASK: terminar de implantar quebra/reposicao
// TASK: reimportar notas do pedido 172646
// TODO: na reposicao concatenar '(REPOSICAO)' no comeco da descricao do produto
// TODO: diff defaultPalette and darkPalette to find the stuff that is missing implementing
// NOTE: add logging everywhere so when the need for debugging on the client pc arises it can be run from the terminal to see the logs

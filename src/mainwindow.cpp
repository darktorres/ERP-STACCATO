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
#include "userconfig.h"
#include "usersession.h"
#include "xlsxdocument.h"
#include "xmlDistanceAPI.h"

#include <QSqlError>

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
  connect(ui->actionGerenciar_Lojas, &QAction::triggered, this, &MainWindow::on_actionGerenciar_Lojas_triggered);
  connect(ui->actionGerenciar_NCMs, &QAction::triggered, this, &MainWindow::on_actionGerenciar_NCMs_triggered);
  connect(ui->actionGerenciar_Transportadoras, &QAction::triggered, this, &MainWindow::on_actionGerenciar_Transportadoras_triggered);
  connect(ui->actionGerenciar_pagamentos, &QAction::triggered, this, &MainWindow::on_actionGerenciar_Pagamentos_triggered);
  connect(ui->actionGerenciar_preco_estoque, &QAction::triggered, this, &MainWindow::on_actionGerenciar_preco_estoque_triggered);
  connect(ui->actionGerenciar_staccatoOff, &QAction::triggered, this, &MainWindow::on_actionGerenciar_staccatoOff_triggered);
  connect(ui->actionImportar_tabela_IBPT, &QAction::triggered, this, &MainWindow::on_actionImportar_tabela_IBPT_triggered);
  connect(ui->actionProdutos, &QAction::triggered, this, &MainWindow::on_actionProdutos_triggered);
  connect(ui->actionPromocao, &QAction::triggered, this, &MainWindow::on_actionPromocao_triggered);
  connect(ui->actionSobre, &QAction::triggered, this, &MainWindow::on_actionSobre_triggered);
  connect(ui->tabWidget, &QTabWidget::currentChanged, this, &MainWindow::on_tabWidget_currentChanged);

  setWindowIcon(QIcon("Staccato.ico"));
  setWindowTitle("ERP Staccato");

  QShortcut *shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), this);
  connect(shortcut, &QShortcut::activated, this, &QWidget::close);

  if (const auto hostname = UserSession::getSetting("Login/hostname")) {
    const QString hostnameText = qApp->getMapLojas().key(hostname->toString());

    setWindowTitle(windowTitle() + " - " + UserSession::nome() + " - " + UserSession::tipoUsuario() + " - " + (hostnameText.isEmpty() ? hostname->toString() : hostnameText));
  } else {
    qApp->enqueueException("A chave 'hostname' não está configurada!", this);
  }

  if (UserSession::tipoUsuario() != "ADMINISTRADOR") { ui->actionCadastrarUsuario->setDisabled(true); }

  if (UserSession::tipoUsuario() != "ADMINISTRADOR" and UserSession::tipoUsuario() != "ADMINISTRATIVO") {
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
    ui->tabWidget->setTabEnabled(ui->tabWidget->indexOf(ui->tabGalpao), query.value("view_tab_galpao").toBool());
    ui->tabWidget->setTabEnabled(ui->tabWidget->indexOf(ui->tabFinanceiro), query.value("view_tab_financeiro").toBool());
    ui->tabWidget->setTabEnabled(ui->tabWidget->indexOf(ui->tabRelatorios), query.value("view_tab_relatorio").toBool());
    ui->tabWidget->setTabEnabled(ui->tabWidget->indexOf(ui->tabRh), query.value("view_tab_rh").toBool());
  } else {
    qApp->enqueueException("Erro lendo permissões: " + query.lastError().text(), this);
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

  if (nomeUsuario == "ADMINISTRADOR" or nomeUsuario == "EDUARDO OLIVEIRA" or nomeUsuario == "GISELY OLIVEIRA" or nomeUsuario == "RODRIGO TORRES" or UserSession::tipoUsuario() == "GERENTE LOJA") {
    ui->tabWidget->setTabEnabled(ui->tabWidget->indexOf(ui->tabGraficos), true);
  }

  //---------------------------------------------------------------------------

  ui->tabWidget->setTabEnabled(ui->tabWidget->indexOf(ui->tabConsistencia), false);

  if (nomeUsuario == "ADMINISTRADOR" or nomeUsuario == "EDUARDO OLIVEIRA" or nomeUsuario == "RODRIGO TORRES") { ui->tabWidget->setTabEnabled(ui->tabWidget->indexOf(ui->tabConsistencia), true); }

  //---------------------------------------------------------------------------

  return;

  QStringList origens;
  origens << "Rua+Salesópolis,27,Barueri,SP";
  origens << "Rua+Ceara,157,Barueri,SP";
  origens << "Av.+Santa+Marina,+2716+-+Vila+Albertina,+São+Paulo+-+SP";

  //  QFile file("C:/temp/distance_waypoints.xml");
  QFile file("distance.xml");

  qDebug() << "open: " << file.open(QFile::ReadOnly);

  XML_Distance *xmlDistance = new XML_Distance(file.readAll(), 1, 5, "Carro ABC", QDateTime(QDate(2019, 01, 05), QTime(10, 5)));
  //  qDebug() << "legs: " << xmlDistance->legs;
  qDebug() << "route: " << xmlDistance->route;

  QStringList route = xmlDistance->route;
  const QString origin = route.takeFirst().replace(" ", "+");
  const QString destin = route.takeLast().replace(" ", "+");

  const QString waypoints = route.join("|").replace(" ", "+");

  qDebug() << "origin: " << origin;
  qDebug() << "destin: " << destin;
  qDebug() << "waypoints: " << waypoints;

  const QString url = "https://www.google.com/maps/dir/?api=1&origin=%1&destination=%2&waypoints=%3&travelmode=driving";

  qDebug() << url.arg(origin).arg(destin).arg(waypoints);

  return;

  QSqlQuery queryEndereco;
  if (not queryEndereco.exec(
          "SELECT idVeiculo, modelo, data, GROUP_CONCAT(distinct idVenda SEPARATOR '|') AS idVenda, GROUP_CONCAT(distinct endereco SEPARATOR '|') AS endereco FROM (SELECT vhp.idVenda, thv.modelo, "
          "vhp.idVeiculo, vhp.data, REPLACE(CONCAT(che.logradouro, ',', che.numero, ',', che.cidade, ',', che.uf), ' ', '+') AS endereco FROM veiculo_has_produto vhp LEFT JOIN "
          "transportadora_has_veiculo thv ON vhp.idVeiculo = thv.idVeiculo LEFT JOIN venda v ON vhp.idVenda = v.idVenda LEFT JOIN cliente_has_endereco che ON che.idEndereco = v.idEnderecoEntrega "
          "WHERE vhp.idVenda IN (SELECT idVenda FROM venda WHERE created > '2018-06-01') AND vhp.idVeiculo <> 11 AND che.idEndereco <> 1 GROUP BY vhp.idVenda , vhp.data , vhp.idVeiculo) x GROUP BY "
          "x.idVeiculo , x.data ORDER BY data , idVeiculo")) {
    qDebug() << "error: " << queryEndereco.lastError().text();
    return;
  }

  //  qDebug() << queryEndereco.size();
  //  return;

  auto *manager = new QNetworkAccessManager(this);

  int evento = 0;

  while (queryEndereco.next()) {
    const QString destinoTemp = queryEndereco.value("endereco").toString();
    QStringList destinos = destinoTemp.split("|");
    const QString destino = destinos.first();
    destinos.removeFirst();

    const int idVeiculo = queryEndereco.value("idVeiculo").toInt();
    const QString modelo = queryEndereco.value("modelo").toString();
    const QDateTime dataEvento = queryEndereco.value("data").toDateTime();

    qDebug() << idVeiculo;
    qDebug() << modelo;
    qDebug() << dataEvento;

    for (const auto &origem : origens) {
      const QUrl url = QString("https://maps.googleapis.com/maps/api/directions/xml?origin=%1&destination=%2&waypoints=optimize:true|%3&key=AIzaSyCg0K1TwMY4PEQ0ZAXIfrUXXQw5tKOEVLw")
                           .arg(origem)
                           .arg(destino)
                           .arg(destinos.join("|"));

      //      qDebug() << url;

      auto *reply = manager->get(QNetworkRequest(url));

      while (not reply->isFinished()) {
        // wait
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
      }

      QString answer = reply->readAll();

      qDebug() << "reply: " << answer.left(200);

      XML_Distance *xmlDistance = new XML_Distance(answer.toUtf8(), ++evento, idVeiculo, modelo, dataEvento);
      qDebug() << "legs: " << xmlDistance->legs;
    }
  }
}

MainWindow::MainWindow() : MainWindow(nullptr) {}

MainWindow::~MainWindow() { delete ui; }

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
  ui->widgetConsistencia->resetTables();

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
  if (currentText == "Galpão") { ui->widgetGalpao->updateTables(); }
  if (currentText == "Financeiro") { ui->widgetFinanceiro->updateTables(); }
  if (currentText == "Relatórios") { ui->widgetRelatorio->updateTables(); }
  if (currentText == "Gráfico") { ui->widgetGraficos->updateTables(); }
  if (currentText == "RH") { ui->widgetRh->updateTables(); }
  if (currentText == "Consistência") { ui->widgetConsistencia->updateTables(); }

  qApp->setUpdating(false);
}

void MainWindow::reconnectDb() {
  const bool conectado = qApp->dbReconnect();

  verifyDb(conectado);
}

void MainWindow::verifyDb(const bool conectado) {
  pushButtonStatus->setText(conectado ? "Conectado: " + UserSession::getSetting("Login/hostname")->toString() : "Desconectado");
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

void MainWindow::on_tabWidget_currentChanged(const int) { updateTables(); }

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

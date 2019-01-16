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
#include "xmlDistanceAPI.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);

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
  connect(ui->actionEstoque, &QAction::triggered, this, &MainWindow::on_actionEstoque_triggered);
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
    const QString hostnameText = qApp->getMapLojas().key(hostname.value().toString());

    setWindowTitle(windowTitle() + " - " + UserSession::nome() + " - " + UserSession::tipoUsuario() + " - " + (hostnameText.isEmpty() ? hostname.value().toString() : hostnameText));
  } else {
    qApp->enqueueError("A chave 'hostname' não está configurada!", this);
  }

  if (UserSession::tipoUsuario() != "ADMINISTRADOR") {
    ui->actionGerenciar_Lojas->setDisabled(true);
    ui->actionGerenciar_Transportadoras->setDisabled(true);
    ui->menuImportar_tabela_fornecedor->setDisabled(true);
    ui->actionCadastrarUsuario->setDisabled(true);
    ui->actionProdutos->setDisabled(true);
    ui->actionCadastrarProfissional->setDisabled(true);
    ui->actionCadastrarFornecedor->setDisabled(true);
    ui->actionGerenciar_preco_estoque->setDisabled(true);
  }

  // -------------------------------------------------------------------------

  QSqlQuery query;
  query.prepare("SELECT * FROM usuario_has_permissao WHERE idUsuario = :idUsuario");
  query.bindValue(":idUsuario", UserSession::idUsuario());

  if (query.exec() and query.first()) {
    // REFAC: dont harcode numbers
    ui->tabWidget->setTabEnabled(0, query.value("view_tab_orcamento").toBool());
    ui->tabWidget->setTabEnabled(1, query.value("view_tab_venda").toBool());
    ui->tabWidget->setTabEnabled(2, query.value("view_tab_compra").toBool());
    ui->tabWidget->setTabEnabled(3, query.value("view_tab_logistica").toBool());
    ui->tabWidget->setTabEnabled(4, query.value("view_tab_nfe").toBool());
    ui->tabWidget->setTabEnabled(5, query.value("view_tab_estoque").toBool());
    ui->tabWidget->setTabEnabled(6, query.value("view_tab_financeiro").toBool());
    ui->tabWidget->setTabEnabled(7, query.value("view_tab_relatorio").toBool());
  } else {
    qApp->enqueueError("Erro lendo permissões: " + query.lastError().text(), this);
  }

  // -------------------------------------------------------------------------

  pushButtonStatus = new QPushButton(this);
  pushButtonStatus->setIcon(QIcon(":/reconnect.png"));
  pushButtonStatus->setText("Conectado: " + UserSession::getSetting("Login/hostname").value().toString());
  pushButtonStatus->setStyleSheet("color: rgb(0, 190, 0);");

  ui->statusBar->addWidget(pushButtonStatus);

  connect(pushButtonStatus, &QPushButton::clicked, this, &MainWindow::verifyDb);

  //---------------------------------------------------------------------------

  ui->tabWidget->setTabEnabled(8, false); // graficos

  const QString nomeUsuario = UserSession::nome();

  if (nomeUsuario == "ADMINISTRADOR" or nomeUsuario == "EDUARDO OLIVEIRA" or nomeUsuario == "GISELY OLIVEIRA") { ui->tabWidget->setTabEnabled(8, true); }

  //  gerarEnviarRelatorio();

  //---------------------------------------------------------------------------

  QStringList origens;
  origens << "Rua+Salesópolis,27,Barueri,SP";
  origens << "Rua+Ceara,157,Barueri,SP";
  origens << "Av.+Santa+Marina,+2716+-+Vila+Albertina,+São+Paulo+-+SP";

  //  QFile file("C:/temp/distance_waypoints.xml");

  //  qDebug() << "open: " << file.open(QFile::ReadOnly);

  //  XML_Distance *xmlDistance = new XML_Distance(file.readAll(), 1, 5, "Carro ABC", QDateTime(QDate(2019, 01, 05), QTime(10, 5)));
  //  qDebug() << "legs: " << xmlDistance->legs;

  //  return;

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

  //------------------------------------------------
}

MainWindow::~MainWindow() { delete ui; }

// TODO: call this after sql error to see if still connected
void MainWindow::verifyDb() {
  const bool conectado = qApp->dbReconnect();

  pushButtonStatus->setText(conectado ? "Conectado: " + UserSession::getSetting("Login/hostname").value().toString() : "Desconectado");
  pushButtonStatus->setStyleSheet(conectado ? "color: rgb(0, 190, 0);" : "color: rgb(255, 0, 0);");

  if (conectado) { resetTables(); }
}

// REFAC: put this in a class
// void MainWindow::gerarEnviarRelatorio() {
//  // REFAC: 0finish
//  // verificar em que etapa eu guardo a linha do dia seguinte no BD

//  QSqlQuery query;
//  // TODO: replace *
//  query.prepare("SELECT * FROM jobs WHERE dataReferente = :dataReferente AND status = 'PENDENTE'");
//  query.bindValue(":dataAgendado", QDate::currentDate());

//  if (not query.exec()) { return qApp->enqueueError("Erro buscando relatórios agendados: " + query.lastError().text()); }

//  while (query.next()) {
//    const QString relatorioPagar = "C:/temp/pagar.xlsx";     // guardar direto no servidor?
//    const QString relatorioReceber = "C:/temp/receber.xlsx"; // e se o computador nao tiver o servidor mapeado?

//    // -------------------------------------------------------------------------

//    QXlsx::Document xlsxPagar(relatorioPagar);

//    //    xlsx.currentWorksheet()->setFitToPage(true);
//    //    xlsx.currentWorksheet()->setFitToHeight(true);
//    //    xlsx.currentWorksheet()->setOrientationVertical(false);

//    QSqlQuery queryView;

//    if (not queryView.exec("SELECT * FROM view_relatorio_pagar")) { return qApp->enqueueError("Erro lendo relatorio pagar: " + queryView.lastError().text()); }

//    xlsxPagar.write("A1", "Data Emissão");
//    xlsxPagar.write("B1", "Data Realizado");
//    xlsxPagar.write("C1", "Valor R$");
//    xlsxPagar.write("D1", "Conta");
//    xlsxPagar.write("E1", "Obs.");
//    xlsxPagar.write("F1", "Contraparte");
//    xlsxPagar.write("G1", "Grupo");
//    xlsxPagar.write("H1", "Subgrupo");

//    int row = 1;

//    while (queryView.next()) {
//      xlsxPagar.write("A" + QString::number(row), queryView.value("dataEmissao"));
//      xlsxPagar.write("B" + QString::number(row), queryView.value("dataRealizado"));
//      xlsxPagar.write("C" + QString::number(row), queryView.value("valorReal"));
//      xlsxPagar.write("D" + QString::number(row), queryView.value("Conta"));
//      xlsxPagar.write("E" + QString::number(row), queryView.value("observacao"));
//      xlsxPagar.write("F" + QString::number(row), queryView.value("contraParte"));
//      xlsxPagar.write("G" + QString::number(row), queryView.value("grupo"));
//      xlsxPagar.write("H" + QString::number(row), queryView.value("subGrupo"));

//      ++row;
//    }

//    // -------------------------------------------------------------------------

//    QXlsx::Document xlsxReceber(relatorioReceber);

//    //    xlsx.currentWorksheet()->setFitToPage(true);
//    //    xlsx.currentWorksheet()->setFitToHeight(true);
//    //    xlsx.currentWorksheet()->setOrientationVertical(false);

//    if (not queryView.exec("SELECT * FROM view_relatorio_receber")) { return qApp->enqueueError("Erro lendo relatorio receber: " + queryView.lastError().text()); }

//    xlsxReceber.write("A1", "dataEmissao");
//    xlsxReceber.write("B1", "dataRealizado");
//    xlsxReceber.write("C1", "valorReal");
//    xlsxReceber.write("D1", "Conta");
//    xlsxReceber.write("E1", "observacao");
//    xlsxReceber.write("F1", "contraParte");
//    xlsxReceber.write("G1", "grupo");
//    xlsxReceber.write("H1", "subGrupo");

//    row = 1;

//    while (queryView.next()) {
//      xlsxReceber.write("A" + QString::number(row), queryView.value("dataEmissao"));
//      xlsxReceber.write("B" + QString::number(row), queryView.value("dataRealizado"));
//      xlsxReceber.write("C" + QString::number(row), queryView.value("valorReal"));
//      xlsxReceber.write("D" + QString::number(row), queryView.value("Conta"));
//      xlsxReceber.write("E" + QString::number(row), queryView.value("observacao"));
//      xlsxReceber.write("F" + QString::number(row), queryView.value("contraParte"));
//      xlsxReceber.write("G" + QString::number(row), queryView.value("grupo"));
//      xlsxReceber.write("H" + QString::number(row), queryView.value("subGrupo"));

//      ++row;
//    }

//    // -------------------------------------------------------------------------

//    QSqlQuery query2;
//    query2.prepare("INSERT INTO jobs (dataEnviado, dataReferente, status) VALUES (:dataEnviado, :dataReferente, 'ENVIADO')");

//    const int diaSemana = QDate::currentDate().dayOfWeek();

//    query2.bindValue(":dataReferente", QDate::currentDate().addDays(diaSemana < 4 ? 5 : diaSemana - 3));
//    query2.bindValue(":dataEnviado", QDate::currentDate());

//    if (not query2.exec()) { return qApp->enqueueError("Erro guardando relatórios financeiro: " + query2.lastError().text()); }

//    //    SendMail *mail = new SendMail(this, anexo, fornecedor);
//    //    mail->setAttribute(Qt::WA_DeleteOnClose);

//    //    mail->exec();
//  }
//}

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

void MainWindow::on_actionEstoque_triggered() {
  auto *importa = new ImportaProdutos(ImportaProdutos::Tipo::Estoque, this);
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
// TASK: arrumar cadastrarNFe para quando guardar a nota pendente associar ela com venda_has_produto para aparecer na tela de consultarNFe (depois disso só vai precisar atualizar a nota com a
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

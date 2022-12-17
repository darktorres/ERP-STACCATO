#include "calculofrete.h"
#include "ui_calculofrete.h"

#include "QtCUrl.h"
#include "application.h"
#include "doubledelegate.h"
#include "porcentagemdelegate.h"
#include "reaisdelegate.h"
#include "sqlquery.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QMessageBox>
#include <QNetworkReply>
#include <QSqlError>
#include <QXmlStreamReader>

CalculoFrete::CalculoFrete(QWidget *parent) : QDialog(parent), ui(new Ui::CalculoFrete) {
  ui->setupUi(this);

  setWindowModality(Qt::NonModal);
  setWindowFlags(Qt::Window);

  ui->itemBoxCliente->setSearchDialog(SearchDialog::cliente(this));

  // -------------------------------------------------------------------------

  SqlQuery queryLoja;
  queryLoja.prepare("SELECT logradouro, numero, complemento, cidade, uf FROM loja_has_endereco WHERE idLoja = :idLoja");
  queryLoja.bindValue(":idLoja", 6);

  if (not queryLoja.exec() or not queryLoja.first()) { throw RuntimeException("Erro buscando endereço do galpão: " + queryLoja.lastError().text(), this); }

  ui->lineEditOrigem->setText(queryLoja.value("logradouro").toString() + " - " + queryLoja.value("numero").toString() + " - " + queryLoja.value("complemento").toString() + " - " +
                              queryLoja.value("cidade").toString() + " - " + queryLoja.value("uf").toString());

  // -------------------------------------------------------------------------

  setupTables();

  // -------------------------------------------------------------------------

  setConnections();
}

CalculoFrete::~CalculoFrete() { delete ui; }

void CalculoFrete::setupTables() {
  modelItem.setTable("view_orcamento_peso");

  modelItem.setHeaderData("peso", "Peso Kg.");
  modelItem.setHeaderData("produto", "Produto");
  modelItem.setHeaderData("fornecedor", "Fornecedor");
  modelItem.setHeaderData("obs", "Obs.");
  modelItem.setHeaderData("prcUnitario", "Preço/Un.");
  modelItem.setHeaderData("kg", "Kg.");
  modelItem.setHeaderData("caixas", "Caixas");
  modelItem.setHeaderData("quant", "Quant.");
  modelItem.setHeaderData("un", "Un.");
  modelItem.setHeaderData("codComercial", "Código");
  modelItem.setHeaderData("formComercial", "Formato");
  modelItem.setHeaderData("quantCaixa", "Quant./Cx.");
  modelItem.setHeaderData("parcial", "Subtotal");
  modelItem.setHeaderData("desconto", "Desc. %");
  modelItem.setHeaderData("parcialDesc", "Total");

  ui->tableProdutos->setModel(&modelItem);

  ui->tableProdutos->hideColumn("idOrcamentoProduto");
  ui->tableProdutos->hideColumn("idProduto");
  ui->tableProdutos->hideColumn("idOrcamento");
  ui->tableProdutos->hideColumn("idLoja");
  ui->tableProdutos->hideColumn("quantCaixa");
  ui->tableProdutos->hideColumn("descUnitario");
  ui->tableProdutos->hideColumn("descGlobal");
  ui->tableProdutos->hideColumn("total");
  ui->tableProdutos->hideColumn("estoque");
  ui->tableProdutos->hideColumn("promocao");
  ui->tableProdutos->hideColumn("mostrarDesconto");

  ui->tableProdutos->setItemDelegate(new DoubleDelegate(this));

  ui->tableProdutos->setItemDelegateForColumn("quant", new DoubleDelegate(4, this));
  ui->tableProdutos->setItemDelegateForColumn("prcUnitario", new ReaisDelegate(this));
  ui->tableProdutos->setItemDelegateForColumn("parcial", new ReaisDelegate(this));
  ui->tableProdutos->setItemDelegateForColumn("parcialDesc", new ReaisDelegate(this));
  ui->tableProdutos->setItemDelegateForColumn("desconto", new PorcentagemDelegate(false, this));
}

void CalculoFrete::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->comboBoxOrcamento, &QComboBox::currentTextChanged, this, &CalculoFrete::on_comboBoxOrcamento_currentTextChanged, connectionType);
  connect(ui->comboBoxVenda, &QComboBox::currentTextChanged, this, &CalculoFrete::on_comboBoxVenda_currentTextChanged, connectionType);
  connect(ui->itemBoxCliente, &ItemBox::textChanged, this, &CalculoFrete::on_itemBoxCliente_textChanged, connectionType);
  connect(ui->pushButtonCalcular, &QPushButton::clicked, this, &CalculoFrete::on_pushButtonCalcular_clicked, connectionType);
}

void CalculoFrete::setCliente(const QVariant &idCliente) { ui->itemBoxCliente->setId(idCliente); }

void CalculoFrete::setOrcamento(const int idEndereco, const double pesoSul, const double pesoTotal) {
    SqlQuery queryEndereco;
    queryEndereco.prepare("SELECT descricao, logradouro, numero, complemento, cidade, uf FROM cliente_has_endereco WHERE idEndereco = :idEndereco");
    queryEndereco.bindValue(":idEndereco", idEndereco);

    if (not queryEndereco.exec()) { throw RuntimeException("Erro buscando endereço do cliente: " + queryEndereco.lastError().text(), this); }

    if (queryEndereco.first()) {
      if (queryEndereco.value("descricao").toString() == "NÃO HÁ/RETIRA") {
        ui->lineEditDestino->setText("NÃO HÁ/RETIRA");
      } else {
        const QString logradouro = queryEndereco.value("logradouro").toString();
        const QString numero = queryEndereco.value("numero").toString();
        const QString complemento = queryEndereco.value("complemento").toString();
        const QString cidade = queryEndereco.value("cidade").toString();
        const QString uf = queryEndereco.value("uf").toString();

        ui->lineEditDestino->setText(logradouro + " - " + numero + " - " + complemento + " - " + cidade + " - " + uf);
      }
    }

    // ------------------------------

    ui->spinBoxPesoSul->setValue(pesoSul);
    ui->spinBoxPesoTotal->setValue(pesoTotal);

    SqlQuery queryCustos;

    if (not queryCustos.exec("SELECT custoTransporteTon, precoCombustivel, capacidadeCaminhaoGrande, custoMotoristaCaminhaoGrande, custoAjudantesCaminhaoGrande, consumoCaminhaoGrande, "
                             "capacidadeCaminhaoPequeno, custoMotoristaCaminhaoPequeno, custoAjudantesCaminhaoPequeno, consumoCaminhaoPequeno "
                             "FROM loja "
                             "WHERE nomeFantasia = 'CENTRO DE DISTRIBUIÇÃO'") or
        not queryCustos.first()) {
      throw RuntimeException("Erro buscando dados do caminhão: " + queryCustos.lastError().text());
    }

    const double custoMotoristaCaminhaoGrande = queryCustos.value("custoMotoristaCaminhaoGrande").toDouble();
    const double custoAjudantesCaminhaoGrande = queryCustos.value("custoAjudantesCaminhaoGrande").toDouble();
    const double custoMotoristaCaminhaoPequeno = queryCustos.value("custoMotoristaCaminhaoPequeno").toDouble();
    const double custoAjudantesCaminhaoPequeno = queryCustos.value("custoAjudantesCaminhaoPequeno").toDouble();
    const int capacidadeCaminhaoGrande = queryCustos.value("capacidadeCaminhaoGrande").toInt();
    const int capacidadeCaminhaoPequeno = queryCustos.value("capacidadeCaminhaoPequeno").toInt();
    const double consumoCaminhaoGrande = queryCustos.value("consumoCaminhaoGrande").toDouble();
    const double consumoCaminhaoPequeno = queryCustos.value("consumoCaminhaoPequeno").toDouble();

    if (capacidadeCaminhaoGrande == 0) { throw RuntimeException("Capacidade do caminhão grande está zerada!"); }

    const int cargas = static_cast<int>(pesoTotal / capacidadeCaminhaoGrande);
    const int resto =  static_cast<int>(pesoTotal) % capacidadeCaminhaoGrande;

    if (resto < capacidadeCaminhaoPequeno) {
      ui->spinBoxCaminhoesGrandes->setSuffix(" - " + QString::number(capacidadeCaminhaoGrande) + " kg - Motorista: R$ " + QString::number(custoMotoristaCaminhaoGrande) + " - Ajudantes: R$ " +
                                             QString::number(custoAjudantesCaminhaoGrande) + " - Km/L: " + QString::number(consumoCaminhaoGrande));
      ui->spinBoxCaminhoesGrandes->setValue(cargas);
      ui->spinBoxCaminhoesPequenos->setSuffix(" - " + QString::number(capacidadeCaminhaoPequeno) + " kg - Motorista: R$ " + QString::number(custoMotoristaCaminhaoPequeno) + " - Ajudantes: R$ " +
                                              QString::number(custoAjudantesCaminhaoPequeno) + " - Km/L: " + QString::number(consumoCaminhaoPequeno));
      ui->spinBoxCaminhoesPequenos->setValue(1);
      const double custoCaminhao = ((custoMotoristaCaminhaoGrande + custoAjudantesCaminhaoGrande) * cargas) + custoMotoristaCaminhaoPequeno + custoAjudantesCaminhaoPequeno;
      ui->doubleSpinBoxMotoristaAjudantes->setValue(custoCaminhao);
    } else {
      ui->spinBoxCaminhoesGrandes->setValue(cargas + 1);
      ui->spinBoxCaminhoesPequenos->setValue(0);
      const double custoCaminhao = (custoMotoristaCaminhaoGrande + custoAjudantesCaminhaoGrande) * (cargas + 1);
      ui->doubleSpinBoxMotoristaAjudantes->setValue(custoCaminhao);
    }

    const double custoTransporteTon = queryCustos.value("custoTransporteTon").toDouble();
    const double custoSul = ui->spinBoxPesoSul->value() / 1000. * custoTransporteTon;

    ui->doubleSpinBoxFreteSul->setValue(custoSul);
    ui->doubleSpinBoxPrecoCombustivel->setValue(queryCustos.value("precoCombustivel").toDouble());
}

double CalculoFrete::getDistancia() {
  const double distancia = ui->lineEditDistancia->text().remove(" km").replace(",", ".").toDouble();
  qDebug() << "double: " << distancia;

  return distancia;
}

double CalculoFrete::getFrete()
{
  qualp();
  return ui->doubleSpinBoxTotal->value();
}

void CalculoFrete::on_pushButtonCalcular_clicked() {
  //. TODO: guardar o valor do pedagio em cliente_has_endereco para não ficar fazendo varias consultas no qualp para o mesmo endereco. se a data da consulta for outro dia refazer a consulta

  //. TODO: quando permitir que o gerente altere o valor:
  //. 1. valor inicial R$1000
  //. 2. valor minimo R$800
  //. 3. até R$900 permitir alterar, abaixo disso pedir um codigo de uso unico

  //. TODO: para entregas dentro da cidade utilizar qMax(valorMinimo, porcentagemFrete)

  //. para entregas fora da cidade:
  //. 1. custo para trazer do sul: R$220/ton.
  //. 2. quantidade de cargas: (ton./4.3). caso o resto da divisao seja menor que 2ton. completar com um caminhao pequeno
  //. 3. qualp
  //. 4. custo motorista e ajudantes
  //. 5. valor final = 1 + 2 + 3 + 4

  //. TODO: quando for frete fora do estado pedir autorizacao/confirmacao da logistica
  //. TODO: pintar de cor diferente o campo do frete no orcamento/venda para indicar se é dentro ou fora da cidade/estado, com ou sem autorizacao
  //. TODO: ao autorizar colocar o valorMinimoFrete no spinBox do frete

  qualp();
}

void CalculoFrete::qualp() {
  ui->lineEditPedagio->clear();
  ui->lineEditDistancia->clear();
  ui->lineEditCombustivel->clear();
  ui->doubleSpinBoxTotal->clear();

  if (ui->lineEditDestino->text().isEmpty() or ui->lineEditDestino->text() == "NÃO HÁ/RETIRA") { throw RuntimeException("Sem endereço de destino!"); }

  // ------------------------------

  SqlQuery query;

  if (not query.exec("SELECT apiQualp, CabecalhosQualp, precoCombustivel, eixosCaminhaoGrande, consumoCaminhaoGrande FROM loja WHERE nomeFantasia = 'CENTRO DE DISTRIBUIÇÃO'") or not query.first()) {
    throw RuntimeException("Erro buscando dados do caminhão: " + query.lastError().text());
  }

  if (query.value("apiQualp").toString().isEmpty() or query.value("cabecalhosQualp").toString().isEmpty()) { throw RuntimeError("QualP não está configurado!"); }

  QString urlString = query.value("apiQualp").toString();

  const QStringList origemSplit = ui->lineEditOrigem->text().split(" - ");
  if (origemSplit.size() != 5) { throw RuntimeException("Erro na formatação do endereço de origem!"); }
  const QString origem = origemSplit.at(0) + ", " + origemSplit.at(1) + ", " + origemSplit.at(3) + " / " + origemSplit.at(4);

  const QStringList destinoSplit = ui->lineEditDestino->text().split(" - ");
  if (destinoSplit.size() != 5) { throw RuntimeException("Erro na formatação do endereço de destino!"); }
  const QString destino = destinoSplit.at(0) + ", " + destinoSplit.at(1) + ", " + destinoSplit.at(3) + " / " + destinoSplit.at(4);

  urlString.replace("_origem_", origem);
  urlString.replace("_destino_", destino);
  urlString.replace("_eixos_", query.value("eixosCaminhaoGrande").toString());
  urlString.replace("_preco_combustivel_", query.value("precoCombustivel").toString());
  urlString.replace("_consumo_combustivel_", query.value("consumoCaminhaoGrande").toString());

  QUrl url(urlString);

  QtCUrl cUrl;
  QtCUrl::Options opt;
  opt[CURLOPT_URL] = url;
  QStringList headers;
  headers = query.value("cabecalhosQualp").toString().split("\n").replaceInStrings("  -H '", "").replaceInStrings("' \\", "");
  opt[CURLOPT_HTTPHEADER] = headers;
  const QString result = cUrl.exec(opt);

  if (cUrl.lastError().isOk()) {
    QJsonParseError errorPtr;
    QJsonDocument json = QJsonDocument::fromJson(result.toUtf8(), &errorPtr);

    QJsonObject rootObj = json.object();

    QJsonValue rotas = rootObj.value("rotas");
    QJsonArray rotasArray = rotas.toArray();
    QJsonObject rotasObj = rotasArray.first().toObject();

    QJsonValue summary = rotasObj.value("summary");
    QJsonObject summaryObj = summary.toObject();

    QJsonValue raw = summaryObj.value("raw");
    QJsonObject rawObj = raw.toObject();

    QJsonValue fmt = summaryObj.value("fmt");
    QJsonObject fmtObj = fmt.toObject();

    const double distance = rawObj.value("distance").toDouble();
    const double fuelConsumption = rawObj.value("total_consumption").toDouble();
    const double tolls = rawObj.value("total_tolls").toDouble();
    const double trip = rawObj.value("total_trip").toDouble();

    const QString distanceFmt = fmtObj.value("distance").toString();
    const QString fuelConsumptionFmt = fmtObj.value("total_consumption").toString();
    const QString tollsFmt = fmtObj.value("total_tolls").toString();
    const QString tripFmt = fmtObj.value("total_trip").toString();

    const int cargas = ui->spinBoxCaminhoesGrandes->value() + ui->spinBoxCaminhoesPequenos->value();
    const QString cargasStr = QString::number(cargas) + "x ";

    ui->lineEditPedagio->setText(cargasStr + tollsFmt + " -> R$ " + QLocale(QLocale::Portuguese).toString(cargas * tolls, 'f', 2));
    ui->lineEditDistancia->setText(cargasStr + distanceFmt + " -> " + QLocale(QLocale::Portuguese).toString(cargas * distance, 'f', 2) + " KM");
    ui->lineEditCombustivel->setText(cargasStr + fuelConsumptionFmt + " -> R$ " + QLocale(QLocale::Portuguese).toString(cargas * fuelConsumption, 'f', 2));

    const double total = ui->doubleSpinBoxFreteSul->value() + ui->doubleSpinBoxMotoristaAjudantes->value() + (cargas * trip);
    ui->doubleSpinBoxTotal->setValue(total * 1.2);
  } else {
    throw RuntimeException("Erro consultando QualP: \n" + cUrl.lastError().text() + "\n" + cUrl.errorBuffer());
  }
}

void CalculoFrete::on_itemBoxCliente_textChanged(const QString &) {
  ui->comboBoxOrcamento->clear();
  ui->comboBoxVenda->clear();
  ui->lineEditDestino->clear();
  ui->lineEditPedagio->clear();
  ui->lineEditDistancia->clear();
  ui->lineEditCombustivel->clear();

  // -------------------------------------------------------------------------

  SqlQuery queryOrcamento;
  queryOrcamento.prepare("SELECT idOrcamento FROM orcamento WHERE idCliente = :idCliente AND status != 'CANCELADO'");
  queryOrcamento.bindValue(":idCliente", ui->itemBoxCliente->getId());

  if (not queryOrcamento.exec()) { throw RuntimeException("Erro buscando orçamentos: " + queryOrcamento.lastError().text()); }

  while (queryOrcamento.next()) { ui->comboBoxOrcamento->addItem(queryOrcamento.value("idOrcamento").toString()); }

  // -------------------------------------------------------------------------

  SqlQuery queryVenda;
  queryVenda.prepare("SELECT idVenda FROM venda WHERE idCliente = :idCliente AND status != 'CANCELADO'");
  queryVenda.bindValue(":idCliente", ui->itemBoxCliente->getId());

  if (not queryVenda.exec()) { throw RuntimeException("Erro buscando vendas: " + queryVenda.lastError().text()); }

  ui->comboBoxVenda->addItem("");

  while (queryVenda.next()) { ui->comboBoxVenda->addItem(queryVenda.value("idVenda").toString()); }
}

void CalculoFrete::on_comboBoxOrcamento_currentTextChanged(const QString &orcamento) {
  ui->lineEditDestino->clear();
  ui->spinBoxPesoSul->clear();
  ui->spinBoxPesoTotal->clear();
  ui->spinBoxCaminhoesGrandes->setSuffix("");
  ui->spinBoxCaminhoesPequenos->setSuffix("");
  ui->spinBoxCaminhoesGrandes->clear();
  ui->spinBoxCaminhoesPequenos->clear();
  ui->doubleSpinBoxFreteSul->clear();
  ui->doubleSpinBoxMotoristaAjudantes->clear();
  ui->doubleSpinBoxPrecoCombustivel->clear();
  ui->lineEditPedagio->clear();
  ui->lineEditDistancia->clear();
  ui->lineEditCombustivel->clear();
  ui->doubleSpinBoxTotal->clear();

  modelItem.setFilter("idOrcamento = '" + orcamento + "'");

  modelItem.select();

  if (modelItem.rowCount() == 0) { return; }

  // ------------------------------

  SqlQuery queryEndereco;
  queryEndereco.prepare("SELECT descricao, logradouro, numero, complemento, cidade, uf FROM cliente_has_endereco WHERE idEndereco IN (SELECT idEnderecoEntrega FROM orcamento WHERE idOrcamento = :idOrcamento)");
  queryEndereco.bindValue(":idOrcamento", ui->comboBoxOrcamento->currentText());

  if (not queryEndereco.exec()) { throw RuntimeException("Erro buscando endereço do cliente: " + queryEndereco.lastError().text(), this); }

  if (queryEndereco.first()) {
    if (queryEndereco.value("descricao").toString() == "NÃO HÁ/RETIRA") {
      ui->lineEditDestino->setText("NÃO HÁ/RETIRA");
    } else {
      const QString logradouro = queryEndereco.value("logradouro").toString();
      const QString numero = queryEndereco.value("numero").toString();
      const QString complemento = queryEndereco.value("complemento").toString();
      const QString cidade = queryEndereco.value("cidade").toString();
      const QString uf = queryEndereco.value("uf").toString();

      ui->lineEditDestino->setText(logradouro + " - " + numero + " - " + complemento + " - " + cidade + " - " + uf);
    }
  }

  // ------------------------------

  double pesoSul = 0.;
  double pesoTotal = 0.;

  for (int row = 0; row < modelItem.rowCount(); ++row) {
    const QString idProduto = modelItem.data(row, "idProduto").toString();

    SqlQuery queryFornecedor;

    if (not queryFornecedor.exec("SELECT vemDoSul FROM fornecedor WHERE idFornecedor = (SELECT idFornecedor FROM produto WHERE idProduto = " + idProduto + ")")) {
      throw RuntimeException("Erro buscando se fornecedor é do sul: " + queryFornecedor.lastError().text());
    }

    if (not queryFornecedor.first()) { throw RuntimeException("Fornecedor não encontrado para produto com id: " + idProduto); }

    if (queryFornecedor.value("vemDoSul").toBool()) { pesoSul += modelItem.data(row, "peso").toDouble(); }

    pesoTotal += modelItem.data(row, "peso").toDouble();
  }

  ui->spinBoxPesoSul->setValue(pesoSul);
  ui->spinBoxPesoTotal->setValue(pesoTotal);

  // ------------------------------

  SqlQuery queryCustos;

  if (not queryCustos.exec("SELECT custoTransporteTon, precoCombustivel, capacidadeCaminhaoGrande, custoMotoristaCaminhaoGrande, custoAjudantesCaminhaoGrande, consumoCaminhaoGrande, "
                           "capacidadeCaminhaoPequeno, custoMotoristaCaminhaoPequeno, custoAjudantesCaminhaoPequeno, consumoCaminhaoPequeno "
                           "FROM loja "
                           "WHERE nomeFantasia = 'CENTRO DE DISTRIBUIÇÃO'") or
      not queryCustos.first()) {
    throw RuntimeException("Erro buscando dados do caminhão: " + queryCustos.lastError().text());
  }

  const double custoMotoristaCaminhaoGrande = queryCustos.value("custoMotoristaCaminhaoGrande").toDouble();
  const double custoAjudantesCaminhaoGrande = queryCustos.value("custoAjudantesCaminhaoGrande").toDouble();
  const double custoMotoristaCaminhaoPequeno = queryCustos.value("custoMotoristaCaminhaoPequeno").toDouble();
  const double custoAjudantesCaminhaoPequeno = queryCustos.value("custoAjudantesCaminhaoPequeno").toDouble();
  const int capacidadeCaminhaoGrande = queryCustos.value("capacidadeCaminhaoGrande").toInt();
  const int capacidadeCaminhaoPequeno = queryCustos.value("capacidadeCaminhaoPequeno").toInt();
  const double consumoCaminhaoGrande = queryCustos.value("consumoCaminhaoGrande").toDouble();
  const double consumoCaminhaoPequeno = queryCustos.value("consumoCaminhaoPequeno").toDouble();

  if (capacidadeCaminhaoGrande == 0) { throw RuntimeException("Capacidade do caminhão grande está zerada!"); }

  const int cargas = static_cast<int>(pesoTotal / capacidadeCaminhaoGrande);
  const int resto =  static_cast<int>(pesoTotal) % capacidadeCaminhaoGrande;

  if (resto < capacidadeCaminhaoPequeno) {
    ui->spinBoxCaminhoesGrandes->setSuffix(" - " + QString::number(capacidadeCaminhaoGrande) + " kg - Motorista: R$ " + QString::number(custoMotoristaCaminhaoGrande) + " - Ajudantes: R$ " +
                                           QString::number(custoAjudantesCaminhaoGrande) + " - Km/L: " + QString::number(consumoCaminhaoGrande));
    ui->spinBoxCaminhoesGrandes->setValue(cargas);
    ui->spinBoxCaminhoesPequenos->setSuffix(" - " + QString::number(capacidadeCaminhaoPequeno) + " kg - Motorista: R$ " + QString::number(custoMotoristaCaminhaoPequeno) + " - Ajudantes: R$ " +
                                            QString::number(custoAjudantesCaminhaoPequeno) + " - Km/L: " + QString::number(consumoCaminhaoPequeno));
    ui->spinBoxCaminhoesPequenos->setValue(1);
    const double custoCaminhao = ((custoMotoristaCaminhaoGrande + custoAjudantesCaminhaoGrande) * cargas) + custoMotoristaCaminhaoPequeno + custoAjudantesCaminhaoPequeno;
    ui->doubleSpinBoxMotoristaAjudantes->setValue(custoCaminhao);
  } else {
    ui->spinBoxCaminhoesGrandes->setValue(cargas + 1);
    ui->spinBoxCaminhoesPequenos->setValue(0);
    const double custoCaminhao = (custoMotoristaCaminhaoGrande + custoAjudantesCaminhaoGrande) * (cargas + 1);
    ui->doubleSpinBoxMotoristaAjudantes->setValue(custoCaminhao);
  }

  const double custoTransporteTon = queryCustos.value("custoTransporteTon").toDouble();
  const double custoSul = ui->spinBoxPesoSul->value() / 1000. * custoTransporteTon;

  ui->doubleSpinBoxFreteSul->setValue(custoSul);
  ui->doubleSpinBoxPrecoCombustivel->setValue(queryCustos.value("precoCombustivel").toDouble());
}

void CalculoFrete::on_comboBoxVenda_currentTextChanged(const QString &venda) {
  if (venda.isEmpty()) { return; }

  SqlQuery query;
  query.prepare("SELECT idOrcamento FROM venda WHERE idVenda = :idVenda");
  query.bindValue(":idVenda", venda);

  if (not query.exec()) { throw RuntimeException("Erro buscando orçamento: " + query.lastError().text()); }

  if (not query.first()) { throw RuntimeException("Orçamento não encontrado para venda com id: " + venda); }

  ui->comboBoxOrcamento->setCurrentText(query.value("idOrcamento").toString());
}

#include "calculofrete.h"
#include "ui_calculofrete.h"

#include "application.h"
#include "file.h"
#include "sqlquery.h"

#include <QFile>
#include <QMessageBox>
#include <QNetworkReply>
#include <QSqlError>
#include <QXmlStreamReader>

CalculoFrete::CalculoFrete(QWidget *parent) : QDialog(parent), ui(new Ui::CalculoFrete) {
  ui->setupUi(this);

  ui->itemBoxCliente->setSearchDialog(SearchDialog::cliente(this));

  setConnections();
}

CalculoFrete::~CalculoFrete() { delete ui; }

void CalculoFrete::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(&networkManager, &QNetworkAccessManager::finished, this, &CalculoFrete::handleNetworkData, connectionType);
  connect(ui->itemBoxCliente, &ItemBox::textChanged, this, &CalculoFrete::on_itemBoxCliente_textChanged, connectionType);
  connect(ui->pushButton, &QPushButton::clicked, this, &CalculoFrete::on_pushButton_clicked, connectionType);
}

void CalculoFrete::setCliente(const QVariant &idCliente) {
  ui->itemBoxCliente->setId(idCliente);
  //  on_itemBoxCliente_textChanged("");
}

double CalculoFrete::getDistancia() {
  const double distancia = ui->lineEditDistancia->text().remove(" km").replace(",", ".").toDouble();
  qDebug() << "double: " << distancia;

  return distancia;
}

void CalculoFrete::on_pushButton_clicked() {
  ui->lineEditDistancia->clear();

  File apiKeyFile("google_api.txt");

  if (not apiKeyFile.open(QFile::ReadOnly)) { throw RuntimeException("Não conseguiu ler chave da API: " + apiKeyFile.errorString(), this); }

  const QString url = searchUrl.arg(ui->comboBoxOrigem->currentText().replace(" ", "+"), ui->comboBoxDestino->currentText().replace(" ", "+"), apiKeyFile.readAll());
  qDebug() << "url: " << url;
  QNetworkRequest request;
  request.setSslConfiguration(QSslConfiguration::defaultConfiguration());
  request.setUrl(url);
  networkManager.get(request);
}

void CalculoFrete::handleNetworkData(QNetworkReply *networkReply) {
  if (networkReply->error() == QNetworkReply::NoError) {
    QByteArray response(networkReply->readAll());
    qDebug() << "response: " << response;
    QXmlStreamReader xml(response);

    while (not xml.atEnd()) {
      xml.readNext();

      if (xml.tokenType() == QXmlStreamReader::StartElement) {
        if (xml.name().toString() == "origin_address") {
          xml.readNext();
          ui->comboBoxOrigem->setCurrentText(xml.text().toString());
        }

        if (xml.name().toString() == "destination_address") {
          xml.readNext();
          ui->comboBoxDestino->setCurrentText(xml.text().toString());
        }

        if (xml.name().toString() == "distance") {
          while (not xml.atEnd()) {
            xml.readNext();

            if (xml.tokenType() == QXmlStreamReader::StartElement) {
              if (xml.name().toString() == "text") {
                xml.readNext();
                qDebug() << "distance: " << xml.text();
                ui->lineEditDistancia->setText(xml.text().toString());
              }
            }
          }
        }
      }
    }
  }

  networkReply->deleteLater();

  getDistancia();
}

void CalculoFrete::on_itemBoxCliente_textChanged(const QString &) {
  ui->comboBoxOrigem->clear();
  ui->comboBoxDestino->clear();
  ui->lineEditDistancia->clear();

  SqlQuery query;
  query.prepare("SELECT logradouro, numero, cidade, uf FROM cliente_has_endereco WHERE idCliente = :idCliente");
  query.bindValue(":idCliente", ui->itemBoxCliente->getId());

  if (not query.exec()) { throw RuntimeException("Erro buscando endereço do cliente: " + query.lastError().text(), this); }

  while (query.next()) {
    ui->comboBoxDestino->addItem(query.value("logradouro").toString() + ", " + query.value("numero").toString() + ", " + query.value("cidade").toString() + ", " + query.value("uf").toString());
  }

  // TODO: don't hardcode this
  ui->comboBoxOrigem->addItem("Rua Salesópolis, 27, Barueri, SP");
}

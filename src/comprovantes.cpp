#include "comprovantes.h"
#include "ui_comprovantes.h"

#include "application.h"
#include "file.h"

#include <QAuthenticator>
#include <QDesktopServices>
#include <QDir>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSqlError>

CustomDelegate::CustomDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

QString CustomDelegate::displayText(const QVariant &value, const QLocale &) const { return value.toString().split("/").last(); }

// --------------------------------------------------------------------------

Comprovantes::Comprovantes(const QString &idVenda, QWidget *parent) : QDialog(parent), idVenda(idVenda), ui(new Ui::Comprovantes) {
  ui->setupUi(this);

  setWindowFlags(Qt::Window);

  setFilter(idVenda);

  setConnections();
}

Comprovantes::~Comprovantes() { delete ui; }

void Comprovantes::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->pushButtonAbrir, &QPushButton::clicked, this, &Comprovantes::on_pushButtonAbrir_clicked, connectionType);
}

void Comprovantes::setFilter(const QString &idVenda) {
  model.setQuery("SELECT DISTINCT fotoEntrega FROM veiculo_has_produto WHERE fotoEntrega IS NOT NULL AND idVenda = '" + idVenda + "'");

  if (model.lastError().isValid()) { throw RuntimeException("Erro procurando comprovantes: " + model.lastError().text(), this); }

  model.setHeaderData("fotoEntrega", "Arquivo");

  ui->table->setModel(&model);

  ui->table->setItemDelegate(new CustomDelegate(this));
}

void Comprovantes::on_pushButtonAbrir_clicked() {
  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!", this); }

  // --------------------------------------------------------------------------

  const QString url = selection.first().data().toString().replace("WEBDAV", "webdav");

  auto *manager = new QNetworkAccessManager(this);
  manager->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);

  connect(manager, &QNetworkAccessManager::authenticationRequired, this, [&](QNetworkReply *reply, QAuthenticator *authenticator) {
    Q_UNUSED(reply)

    File file("webdav.txt");

    if (not file.open(QFile::ReadOnly)) { throw RuntimeException("Erro lendo arquivo webdav: " + file.errorString()); }

    const QStringList lines = QString(file.readAll()).split("\r\n", Qt::SkipEmptyParts);

    authenticator->setRealm(lines.at(0));
    authenticator->setUser(lines.at(1));
    authenticator->setPassword(lines.at(2));
  });

  auto reply = manager->get(QNetworkRequest(QUrl(url)));

  connect(reply, &QNetworkReply::finished, this, [=] {
    if (reply->error() != QNetworkReply::NoError) {
      if (reply->errorString().contains("server replied: Not Found")) { throw RuntimeException("Arquivo não encontrado no servidor!", this); }

      throw RuntimeException("Erro ao baixar arquivo: " + reply->errorString(), this);
    }

    const QString filename = QDir::currentPath() + "/arquivos/" + url.split("/").last();

    File file(filename);

    if (not file.open(QFile::WriteOnly)) { throw RuntimeException("Erro abrindo arquivo para escrita: " + file.errorString(), this); }

    file.write(reply->readAll());

    file.close();

    QDesktopServices::openUrl(QUrl::fromLocalFile(filename));
  });
}

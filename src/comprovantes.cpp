#include "comprovantes.h"
#include "ui_comprovantes.h"

#include "application.h"
#include "file.h"
#include "usersession.h"

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

  model.select();

  model.setHeaderData("fotoEntrega", "Arquivo");

  ui->table->setModel(&model);

  ui->table->setItemDelegate(new CustomDelegate(this));
}

void Comprovantes::on_pushButtonAbrir_clicked() {
  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!", this); }

  // --------------------------------------------------------------------------

  const QString url = selection.first().data().toString();

  auto *manager = new QNetworkAccessManager(this);
  manager->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);

  connect(manager, &QNetworkAccessManager::authenticationRequired, this, [&](QNetworkReply *reply, QAuthenticator *authenticator) {
    Q_UNUSED(reply)

    authenticator->setUser(UserSession::usuario);
    authenticator->setPassword(UserSession::senha);
  });

  auto reply = manager->get(QNetworkRequest(QUrl(url)));

  connect(reply, &QNetworkReply::finished, this, [=] {
    if (reply->error() != QNetworkReply::NoError) {
      if (reply->error() == QNetworkReply::ContentNotFoundError) { throw RuntimeError("Arquivo não encontrado no servidor!"); }

      throw RuntimeException("Erro ao baixar arquivo: " + reply->errorString(), this);
    }

    const auto replyMsg = reply->readAll();

    if (replyMsg.contains("could not be found on this server")) { throw RuntimeException("Arquivo não encontrado no servidor!"); }

    const QString filename = QDir::currentPath() + "/arquivos/" + url.split("/").last();

    File file(filename);

    if (not file.open(QFile::WriteOnly)) { throw RuntimeException("Erro abrindo arquivo para escrita: " + file.errorString(), this); }

    file.write(replyMsg);

    file.close();

    QDesktopServices::openUrl(QUrl::fromLocalFile(filename));
  });
}

#include "comprovantes.h"
#include "ui_comprovantes.h"

#include "application.h"

#include <QDesktopServices>
#include <QDir>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSqlError>

Comprovantes::CustomDelegate::CustomDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

QString Comprovantes::CustomDelegate::displayText(const QVariant &value, const QLocale &) const { return value.toString().split("/").last(); }

// --------------------------------------------------------------------------

Comprovantes::Comprovantes(const QString &idVenda, QWidget *parent) : QDialog(parent), idVenda(idVenda), ui(new Ui::Comprovantes) {
  ui->setupUi(this);

  setWindowFlags(Qt::Window);

  setFilter(idVenda);

  connect(ui->pushButtonAbrir, &QPushButton::clicked, this, &Comprovantes::on_pushButtonAbrir_clicked);
}

Comprovantes::~Comprovantes() { delete ui; }

void Comprovantes::setFilter(const QString &idVenda) {
  model.setQuery("SELECT DISTINCT fotoEntrega FROM veiculo_has_produto WHERE idVenda = '" + idVenda + "'");

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
  auto request = QNetworkRequest(QUrl(url));
  request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
  auto reply = manager->get(request);

  connect(reply, &QNetworkReply::finished, [=] {
    if (reply->error() != QNetworkReply::NoError) { throw RuntimeException("Erro ao baixar arquivo: " + reply->errorString(), this); }

    const QString filename = url.split("/").last();
    const QString path = QDir::currentPath() + "/arquivos/";

    QDir dir;

    if (not dir.exists(path) and not dir.mkpath(path)) { throw RuntimeException("Erro ao criar a pasta dos comprovantes!", this); }

    QFile file(path + filename);

    if (not file.open(QFile::WriteOnly)) { throw RuntimeException("Erro abrindo arquivo para escrita: " + file.errorString(), this); }

    file.write(reply->readAll());

    file.close();

    QDesktopServices::openUrl(QUrl::fromLocalFile(path + filename));
  });
}

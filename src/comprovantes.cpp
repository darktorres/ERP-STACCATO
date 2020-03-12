#include "comprovantes.h"
#include "ui_comprovantes.h"

#include "application.h"

#include <QDesktopServices>
#include <QDir>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSqlError>

CustomDelegate::CustomDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

QString CustomDelegate::displayText(const QVariant &value, const QLocale &) const { return value.toString().split("/").last(); }

// --------------------------------------------------------------------------

Comprovantes::Comprovantes(const QString &idVenda, QWidget *parent) : QDialog(parent), ui(new Ui::Comprovantes), idVenda(idVenda) {
  ui->setupUi(this);

  setWindowFlags(Qt::Window);

  setFilter(idVenda);

  connect(ui->pushButtonAbrir, &QPushButton::clicked, this, &Comprovantes::on_pushButtonAbrir_clicked);
}

Comprovantes::~Comprovantes() { delete ui; }

void Comprovantes::setFilter(const QString &idVenda) {
  model.setQuery("SELECT DISTINCT fotoEntrega FROM veiculo_has_produto WHERE idVenda = '" + idVenda + "'");

  if (model.lastError().isValid()) { return qApp->enqueueError("Erro procurando comprovantes: " + model.lastError().text(), this); }

  model.setHeaderData("fotoEntrega", "Arquivo");

  ui->table->setModel(&model);

  ui->table->setItemDelegate(new CustomDelegate(this));
}

void Comprovantes::on_pushButtonAbrir_clicked() {
  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { return qApp->enqueueError("Nenhuma linha selecionada!", this); }

  // --------------------------------------------------------------------------

  const QString url = selection.first().data().toString();

  auto *manager = new QNetworkAccessManager(this);
  auto reply = manager->get(QNetworkRequest(QUrl(url)));

  connect(reply, &QNetworkReply::finished, [=] {
    if (reply->error() != QNetworkReply::NoError) { return qApp->enqueueError("Erro ao baixar arquivo: " + reply->errorString()); }

    const QString filename = url.split("/").last();
    const QString path = QDir::currentPath() + "/arquivos/";

    QDir dir;

    if (not dir.exists(path)) { dir.mkpath(path); }

    QFile file(path + filename);

    if (not file.open(QFile::WriteOnly)) { return qApp->enqueueError("Erro abrindo arquivo para escrita: " + file.errorString()); }

    file.write(reply->readAll());

    file.close();

    QDesktopServices::openUrl(QUrl::fromLocalFile(path + filename));
  });
}

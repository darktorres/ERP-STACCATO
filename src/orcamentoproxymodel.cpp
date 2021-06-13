#include "orcamentoproxymodel.h"

#include "user.h"

#include <QBrush>
#include <QSqlRecord>

OrcamentoProxyModel::OrcamentoProxyModel(QSqlQueryModel *model, QObject *parent)
    : QIdentityProxyModel(parent), diasRestantesIndex(model->record().indexOf("Dias restantes")), followupIndex(model->record().indexOf("Observação")),
      semaforoIndex(model->record().indexOf("semaforo")), statusIndex(model->record().indexOf("Status")) {
  setSourceModel(model);
}

QVariant OrcamentoProxyModel::data(const QModelIndex &proxyIndex, const int role) const {
  if (role == Qt::BackgroundRole or role == Qt::ForegroundRole) {
    if (statusIndex != -1) {
      const QString status = proxyIndex.siblingAtColumn(statusIndex).data().toString();

      if (status == "FECHADO") {
        if (role == Qt::BackgroundRole) { return QBrush(Qt::green); }
        if (role == Qt::ForegroundRole) { return QBrush(Qt::black); }
      }

      if (status == "CANCELADO") {
        if (role == Qt::BackgroundRole) { return QBrush(Qt::yellow); }
        if (role == Qt::ForegroundRole) { return QBrush(Qt::black); }
      }

      if (status == "PERDIDO") {
        if (role == Qt::BackgroundRole) { return QBrush(Qt::yellow); }
        if (role == Qt::ForegroundRole) { return QBrush(Qt::black); }
      }
    }

    if (proxyIndex.column() == diasRestantesIndex) {
      const int dias = proxyIndex.siblingAtColumn(diasRestantesIndex).data().toInt();
      const QString status = proxyIndex.siblingAtColumn(statusIndex).data().toString();

      if (status == "FECHADO") {
        if (role == Qt::DisplayRole) { return QVariant(); }
      }

      if (dias >= 5) {
        if (role == Qt::BackgroundRole) { return QBrush(Qt::green); }
        if (role == Qt::ForegroundRole) { return QBrush(Qt::black); }
      } else if (dias >= 3) {
        if (role == Qt::BackgroundRole) { return QBrush(Qt::yellow); }
        if (role == Qt::ForegroundRole) { return QBrush(Qt::black); }
      } else {
        if (role == Qt::BackgroundRole) { return QBrush(Qt::red); }
        if (role == Qt::ForegroundRole) { return QBrush(Qt::black); }
      }
    }

    if (proxyIndex.column() == followupIndex) {
      const FieldColors semaforo = static_cast<FieldColors>(proxyIndex.siblingAtColumn(semaforoIndex).data().toInt());

      if (semaforo == FieldColors::Quente) {
        if (role == Qt::BackgroundRole) { return QBrush(QColor(255, 66, 66)); }
        if (role == Qt::ForegroundRole) { return QBrush(Qt::black); }
      }

      if (semaforo == FieldColors::Morno) {
        if (role == Qt::BackgroundRole) { return QBrush(QColor(255, 170, 0)); }
        if (role == Qt::ForegroundRole) { return QBrush(Qt::black); }
      }

      if (semaforo == FieldColors::Frio) {
        if (role == Qt::BackgroundRole) { return QBrush(QColor(70, 113, 255)); }
        if (role == Qt::ForegroundRole) { return QBrush(Qt::black); }
      }
    }

    if (role == Qt::ForegroundRole) {
      const QString tema = User::getSetting("User/tema").toString();

      return (tema == "escuro") ? QBrush(Qt::white) : QBrush(Qt::black);
    }
  }

  return QIdentityProxyModel::data(proxyIndex, role);
}

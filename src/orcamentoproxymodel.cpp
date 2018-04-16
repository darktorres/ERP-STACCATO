#include <QApplication>
#include <QBrush>
#include <QStyle>

#include "orcamentoproxymodel.h"

OrcamentoProxyModel::OrcamentoProxyModel(SqlRelationalTableModel *model, QObject *parent)
    : QIdentityProxyModel(parent), diasRestantesIndex(model->fieldIndex("Dias restantes")), statusIndex(model->fieldIndex("status")), followupIndex(model->fieldIndex("Observação")),
      semaforoIndex(model->fieldIndex("semaforo")) {
  setSourceModel(model);
}

QVariant OrcamentoProxyModel::data(const QModelIndex &proxyIndex, const int role) const {
  if (role == Qt::BackgroundRole) {
    if (proxyIndex.column() == diasRestantesIndex) {
      const int dias = QIdentityProxyModel::data(index(proxyIndex.row(), diasRestantesIndex), Qt::DisplayRole).toInt();
      const QString status = QIdentityProxyModel::data(index(proxyIndex.row(), statusIndex), Qt::DisplayRole).toString();

      if (dias >= 5 or status == "FECHADO") { return QBrush(Qt::green); }
      if (dias >= 3 or status == "CANCELADO") { return QBrush(Qt::yellow); }
      if (dias < 3) { return QBrush(Qt::red); }
    }

    if (proxyIndex.column() == followupIndex) {
      const FieldColors semaforo = static_cast<FieldColors>(QIdentityProxyModel::data(index(proxyIndex.row(), semaforoIndex), Qt::DisplayRole).toInt());

      if (semaforo == FieldColors::Quente) { return QBrush(QColor(255, 66, 66)); }
      if (semaforo == FieldColors::Morno) { return QBrush(QColor(255, 170, 0)); }
      if (semaforo == FieldColors::Frio) { return QBrush(QColor(70, 113, 255)); }
    }

    const QString status = QIdentityProxyModel::data(index(proxyIndex.row(), statusIndex), Qt::DisplayRole).toString();
    if (status == "FECHADO") { return QBrush(Qt::green); }
    if (status == "CANCELADO") { return QBrush(Qt::yellow); }
    if (status == "PERDIDO") { return QBrush(Qt::yellow); }
  }

  if (role == Qt::ForegroundRole) {

    // those paint the text as black if the background is colored

    if (proxyIndex.column() == diasRestantesIndex) {
      const int dias = QIdentityProxyModel::data(index(proxyIndex.row(), diasRestantesIndex), Qt::DisplayRole).toInt();
      const QString status = QIdentityProxyModel::data(index(proxyIndex.row(), statusIndex), Qt::DisplayRole).toString();

      if (dias >= 5 or status == "FECHADO") { return QBrush(Qt::black); }
      if (dias >= 3 or status == "CANCELADO") { return QBrush(Qt::black); }
      if (dias < 3) { return QBrush(Qt::black); }
    }

    if (proxyIndex.column() == followupIndex) {
      const FieldColors semaforo = static_cast<FieldColors>(QIdentityProxyModel::data(index(proxyIndex.row(), semaforoIndex), Qt::DisplayRole).toInt());

      if (semaforo == FieldColors::Quente) { return QBrush(Qt::black); }
      if (semaforo == FieldColors::Morno) { return QBrush(Qt::black); }
      if (semaforo == FieldColors::Frio) { return QBrush(Qt::black); }
    }

    const QString status = QIdentityProxyModel::data(index(proxyIndex.row(), statusIndex), Qt::DisplayRole).toString();
    if (status == "FECHADO") { return QBrush(Qt::black); }
    if (status == "CANCELADO") { return QBrush(Qt::black); }
    if (status == "PERDIDO") { return QBrush(Qt::black); }

    // -------------------------------------------------------------------------

    return qApp->style()->objectName() == "fusion" ? QBrush(Qt::black) : QBrush(Qt::white);
  }

  return QIdentityProxyModel::data(proxyIndex, role);
}

#include <QApplication>
#include <QBrush>
#include <QDebug>
#include <QStyle>

#include "vendaproxymodel.h"

VendaProxyModel::VendaProxyModel(SqlRelationalTableModel *model, QObject *parent)
    : QIdentityProxyModel(parent), diasIndex(model->fieldIndex("Dias restantes")), statusIndex(model->fieldIndex("Status")), followupIndex(model->fieldIndex("Observação")),
      semaforoIndex(model->fieldIndex("semaforo")), financeiroIndex(model->fieldIndex("statusFinanceiro")) {
  setSourceModel(model);
}

QVariant VendaProxyModel::data(const QModelIndex &proxyIndex, const int role) const {
  if (role == Qt::DisplayRole) {
    if (proxyIndex.column() == diasIndex) {
      const QString status = QIdentityProxyModel::data(index(proxyIndex.row(), statusIndex), Qt::DisplayRole).toString();

      if (status == "ENTREGUE") { return QVariant(); }
    }
  }

  if (role == Qt::BackgroundRole) {
    if (proxyIndex.column() == diasIndex) {
      const int dias = QIdentityProxyModel::data(index(proxyIndex.row(), diasIndex), Qt::DisplayRole).toInt();
      const QString status = QIdentityProxyModel::data(index(proxyIndex.row(), statusIndex), Qt::DisplayRole).toString();

      if (dias >= 5 or status == "ENTREGUE") { return QBrush(Qt::green); }
      if (dias >= 3 or status == "CANCELADO" or status == "DEVOLVIDO" or status == "PROCESSADO") { return QBrush(Qt::yellow); }
      if (dias < 3) { return QBrush(Qt::red); }
    }

    if (proxyIndex.column() == followupIndex) {
      const FieldColors semaforo = static_cast<FieldColors>(QIdentityProxyModel::data(index(proxyIndex.row(), semaforoIndex), Qt::DisplayRole).toInt());

      if (semaforo == FieldColors::Quente) { return QBrush(QColor(255, 66, 66)); }
      if (semaforo == FieldColors::Morno) { return QBrush(QColor(255, 170, 0)); }
      if (semaforo == FieldColors::Frio) { return QBrush(QColor(70, 113, 255)); }
    }

    if (proxyIndex.column() == financeiroIndex) {
      const QString financeiro = QIdentityProxyModel::data(index(proxyIndex.row(), financeiroIndex), Qt::DisplayRole).toString();

      if (financeiro == "PENDENTE") { return QBrush(Qt::red); }
      if (financeiro == "CONFERIDO") { return QBrush(Qt::yellow); }
      if (financeiro == "LIBERADO") { return QBrush(Qt::green); }
    }

    const QString status = QIdentityProxyModel::data(index(proxyIndex.row(), statusIndex), Qt::DisplayRole).toString();
    if (status == "ENTREGUE") { return QBrush(Qt::green); }
    if (status == "CANCELADO" or status == "DEVOLVIDO" or status == "PROCESSADO" or status == "PERDIDO") { return QBrush(Qt::yellow); }
  }

  if (role == Qt::ForegroundRole) {

    // those paint the text as black if the background is colored

    if (proxyIndex.column() == diasIndex) {
      const int dias = QIdentityProxyModel::data(index(proxyIndex.row(), diasIndex), Qt::DisplayRole).toInt();
      const QString status = QIdentityProxyModel::data(index(proxyIndex.row(), statusIndex), Qt::DisplayRole).toString();

      if (dias >= 5 or status == "ENTREGUE") { return QBrush(Qt::black); }
      if (dias >= 3 or status == "CANCELADO" or status == "DEVOLVIDO" or status == "PROCESSADO") { return QBrush(Qt::black); }
      if (dias < 3) { return QBrush(Qt::black); }
    }

    if (proxyIndex.column() == followupIndex) {
      const FieldColors semaforo = static_cast<FieldColors>(QIdentityProxyModel::data(index(proxyIndex.row(), semaforoIndex), Qt::DisplayRole).toInt());

      if (semaforo == FieldColors::Quente) { return QBrush(Qt::black); }
      if (semaforo == FieldColors::Morno) { return QBrush(Qt::black); }
      if (semaforo == FieldColors::Frio) { return QBrush(Qt::black); }
    }

    if (proxyIndex.column() == financeiroIndex) {
      const QString financeiro = QIdentityProxyModel::data(index(proxyIndex.row(), financeiroIndex), Qt::DisplayRole).toString();

      if (financeiro == "PENDENTE") { return QBrush(Qt::black); }
      if (financeiro == "CONFERIDO") { return QBrush(Qt::black); }
      if (financeiro == "LIBERADO") { return QBrush(Qt::black); }
    }

    const QString status = QIdentityProxyModel::data(index(proxyIndex.row(), statusIndex), Qt::DisplayRole).toString();
    if (status == "ENTREGUE") { return QBrush(Qt::black); }
    if (status == "CANCELADO" or status == "DEVOLVIDO" or status == "PROCESSADO") { return QBrush(Qt::black); }
    if (status == "PERDIDO") { return QBrush(Qt::black); }

    // -------------------------------------------------------------------------

    return qApp->style()->objectName() == "fusion" ? QBrush(Qt::black) : QBrush(Qt::white);
  }

  return QIdentityProxyModel::data(proxyIndex, role);
}

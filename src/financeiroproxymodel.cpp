#include <QApplication>
#include <QBrush>
#include <QDate>
#include <QDebug>
#include <QStyle>

#include "financeiroproxymodel.h"

FinanceiroProxyModel::FinanceiroProxyModel(SqlRelationalTableModel *model, QObject *parent)
    : QIdentityProxyModel(parent), statusFinanceiro(model->fieldIndex("statusFinanceiro")), prazoEntrega(model->fieldIndex("prazoEntrega")), novoPrazoEntrega(model->fieldIndex("novoPrazoEntrega")) {
  setSourceModel(model);
}

QVariant FinanceiroProxyModel::data(const QModelIndex &proxyIndex, int role) const {
  if (role == Qt::BackgroundRole) {
    if (proxyIndex.column() == statusFinanceiro) {
      const QString status = QIdentityProxyModel::data(index(proxyIndex.row(), statusFinanceiro), Qt::DisplayRole).toString();

      if (status == "PENDENTE") return QBrush(Qt::red);
      if (status == "CONFERIDO") return QBrush(Qt::yellow);
      if (status == "LIBERADO") return QBrush(Qt::green);
    }

    if (proxyIndex.column() == prazoEntrega) {
      const QDate prazo = QIdentityProxyModel::data(index(proxyIndex.row(), prazoEntrega), Qt::DisplayRole).toDate();
      const int dias = QDate::currentDate().daysTo(prazo);

      if (not prazo.isNull() and dias >= 3 and dias < 5) return QBrush(Qt::yellow);
      if (not prazo.isNull() and dias < 3) return QBrush(Qt::red);
    }

    if (proxyIndex.column() == novoPrazoEntrega) {
      const QDate prazo = QIdentityProxyModel::data(index(proxyIndex.row(), novoPrazoEntrega), Qt::DisplayRole).toDate();
      const int dias = QDate::currentDate().daysTo(prazo);

      if (not prazo.isNull() and dias >= 3 and dias < 5) return QBrush(Qt::yellow);
      if (not prazo.isNull() and dias < 3) return QBrush(Qt::red);
    }
  }

  if (role == Qt::ForegroundRole) {

    // those paint the text as black if the background is colored

    if (proxyIndex.column() == statusFinanceiro) {
      const QString status = QIdentityProxyModel::data(index(proxyIndex.row(), statusFinanceiro), Qt::DisplayRole).toString();

      if (status == "PENDENTE") return QBrush(Qt::black);
      if (status == "CONFERIDO") return QBrush(Qt::black);
      if (status == "LIBERADO") return QBrush(Qt::black);
    }

    if (proxyIndex.column() == prazoEntrega) {
      const QDate prazo = QIdentityProxyModel::data(index(proxyIndex.row(), prazoEntrega), Qt::DisplayRole).toDate();
      const int dias = QDate::currentDate().daysTo(prazo);

      if (not prazo.isNull() and dias >= 3 and dias < 5) return QBrush(Qt::black);
      if (not prazo.isNull() and dias < 3) return QBrush(Qt::black);
    }

    if (proxyIndex.column() == novoPrazoEntrega) {
      const QDate prazo = QIdentityProxyModel::data(index(proxyIndex.row(), novoPrazoEntrega), Qt::DisplayRole).toDate();
      const int dias = QDate::currentDate().daysTo(prazo);

      if (not prazo.isNull() and dias >= 3 and dias < 5) return QBrush(Qt::black);
      if (not prazo.isNull() and dias < 3) return QBrush(Qt::black);
    }

    // those paint the text as black if the background is colored

    return qApp->style()->objectName() == "fusion" ? QBrush(Qt::black) : QBrush(Qt::white);
  }

  return QIdentityProxyModel::data(proxyIndex, role);
}

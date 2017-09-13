#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <ciso646>

#include "sqlquerymodel.h"

SqlQueryModel::SqlQueryModel(QObject *parent) : QSqlQueryModel(parent) {}

QVariant SqlQueryModel::data(const int row, const QString &column) const { return QSqlQueryModel::data(QSqlQueryModel::index(row, QSqlQueryModel::record().indexOf(column))); }

bool SqlQueryModel::setHeaderData(const QString &column, const QVariant &value) { return QSqlQueryModel::setHeaderData(QSqlQueryModel::record().indexOf(column), Qt::Horizontal, value); }

bool SqlQueryModel::setData(const int row, const QString &column, const QVariant &value) {
  if (row == -1) {
    QMessageBox::critical(nullptr, "Erro!", "Erro: linha -1 SqlTableModel");
    return false;
  }

  if (QSqlQueryModel::record().indexOf(column) == -1) {
    QMessageBox::critical(nullptr, "Erro!", "Chave " + column + " não encontrada na query " + QSqlQueryModel::query().executedQuery());
    return false;
  }

  if (not QSqlQueryModel::setData(QSqlQueryModel::index(row, QSqlQueryModel::record().indexOf(column)), value)) {
    QMessageBox::critical(nullptr, "Erro!", "Erro inserindo " + column + " na query " + QSqlQueryModel::query().executedQuery() + ": " + QSqlQueryModel::lastError().text() + " - linha: " +
                                                QString::number(row) + " - valor: " + value.toString());
    return false;
  }

  return true;
}

bool SqlQueryModel::setData(const int row, const int column, const QVariant &value) {
  if (not QSqlQueryModel::setData(QSqlQueryModel::index(row, column), value)) {
    QMessageBox::critical(nullptr, "Erro!",
                          "Erro inserindo " + QSqlQueryModel::record().fieldName(column) + " na query " + QSqlQueryModel::query().executedQuery() + ": " + QSqlQueryModel::lastError().text());
    return false;
  }

  return true;
}

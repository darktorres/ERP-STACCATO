#include <QSqlError>
#include <QSqlRecord>

#include "application.h"
#include "sqlrelationaltablemodel.h"

SqlRelationalTableModel::SqlRelationalTableModel(const int limit, QObject *parent) : QSqlRelationalTableModel(parent), limit(limit) {}

QVariant SqlRelationalTableModel::data(const int row, const int column) const { return QSqlRelationalTableModel::data(QSqlTableModel::index(row, column)); }

QVariant SqlRelationalTableModel::data(const int row, const QString &column) const {
  if (QSqlTableModel::fieldIndex(column) == -1) {
    qApp->enqueueError("Chave '" + column + "' não encontrada na tabela " + QSqlTableModel::tableName());
    return QVariant();
  }

  return QSqlRelationalTableModel::data(QSqlTableModel::index(row, QSqlTableModel::fieldIndex(column)));
}

bool SqlRelationalTableModel::setData(const int row, const int column, const QVariant &value) {
  if (not QSqlRelationalTableModel::setData(QSqlTableModel::index(row, column), value)) {
    return qApp->enqueueError(false, "Erro inserindo " + QSqlTableModel::record().fieldName(column) + " na tabela: " + QSqlTableModel::lastError().text());
  }

  return true;
}

bool SqlRelationalTableModel::setData(const int row, const QString &column, const QVariant &value) {
  if (row == -1) { return qApp->enqueueError(false, "Erro: linha -1 SqlTableModel"); }

  if (QSqlTableModel::fieldIndex(column) == -1) { return qApp->enqueueError(false, "Chave " + column + " não encontrada na tabela " + QSqlTableModel::tableName()); }

  if (not QSqlRelationalTableModel::setData(QSqlTableModel::index(row, QSqlTableModel::fieldIndex(column)), value)) {
    return qApp->enqueueError(false, "Erro inserindo '" + column + "' na tabela '" + tableName() + "': " + QSqlTableModel::lastError().text() + " - linha: " + QString::number(row) +
                                         " - valor: " + value.toString());
  }

  return true;
}

bool SqlRelationalTableModel::setHeaderData(const QString &column, const QVariant &value) { return QSqlTableModel::setHeaderData(QSqlTableModel::fieldIndex(column), Qt::Horizontal, value); }

Qt::DropActions SqlRelationalTableModel::supportedDropActions() const { return Qt::MoveAction; }

bool SqlRelationalTableModel::submitAll() {
  if (not QSqlTableModel::submitAll()) { return qApp->enqueueError(false, "Erro salvando tabela '" + QSqlTableModel::tableName() + "': " + QSqlTableModel::lastError().text()); }

  return true;
}

QString SqlRelationalTableModel::selectStatement() const { return QSqlRelationalTableModel::selectStatement() + (limit > 0 ? " LIMIT " + QString::number(limit) : ""); }

QModelIndexList SqlRelationalTableModel::match(const QString &column, const QVariant &value, int hits, Qt::MatchFlags flags) const {
  return QSqlRelationalTableModel::match(QSqlRelationalTableModel::index(0, QSqlRelationalTableModel::fieldIndex(column)), Qt::DisplayRole, value, hits, flags);
}

bool SqlRelationalTableModel::select() {
  if (not QSqlRelationalTableModel::select()) { return qApp->enqueueError(false, "Erro lendo tabela '" + QSqlTableModel::tableName() + "': " + QSqlTableModel::lastError().text()); }

  return true;
}

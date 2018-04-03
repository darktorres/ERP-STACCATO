#include <QDebug>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlRecord>

#include "application.h"
#include "sqlrelationaltablemodel.h"

SqlRelationalTableModel::SqlRelationalTableModel(const int limit, QObject *parent) : QSqlRelationalTableModel(parent), limit(limit) {
  connect(this, &SqlRelationalTableModel::errorSignal, qApp, &Application::enqueueError);
}

QVariant SqlRelationalTableModel::data(const int row, const int column) const { return QSqlRelationalTableModel::data(QSqlTableModel::index(row, column)); }

QVariant SqlRelationalTableModel::data(const int row, const QString &column) const {
  if (QSqlTableModel::fieldIndex(column) == -1) {
    emit errorSignal("Chave " + column + " não encontrada na tabela " + QSqlTableModel::tableName());
    return QVariant();
  }

  return QSqlRelationalTableModel::data(QSqlTableModel::index(row, QSqlTableModel::fieldIndex(column)));
}

bool SqlRelationalTableModel::setData(const int row, const int column, const QVariant &value) {
  if (not QSqlRelationalTableModel::setData(QSqlTableModel::index(row, column), value)) {
    emit errorSignal("Erro inserindo " + QSqlTableModel::record().fieldName(column) + " na tabela: " + QSqlTableModel::lastError().text());
    return false;
  }

  return true;
}

bool SqlRelationalTableModel::setData(const int row, const QString &column, const QVariant &value) {
  if (row == -1) {
    emit errorSignal("Erro: linha -1 SqlTableModel");
    return false;
  }

  if (QSqlTableModel::fieldIndex(column) == -1) {
    emit errorSignal("Chave " + column + " não encontrada na tabela " + QSqlTableModel::tableName());
    return false;
  }

  if (not QSqlRelationalTableModel::setData(QSqlTableModel::index(row, QSqlTableModel::fieldIndex(column)), value)) {
    emit errorSignal("Erro inserindo " + column + " na tabela " + tableName() + ": " + QSqlTableModel::lastError().text() + " - linha: " + QString::number(row) + " - valor: " + value.toString());
    return false;
  }

  return true;
}

bool SqlRelationalTableModel::setHeaderData(const QString &column, const QVariant &value) { return QSqlTableModel::setHeaderData(QSqlTableModel::fieldIndex(column), Qt::Horizontal, value); }

Qt::ItemFlags SqlRelationalTableModel::flags(const QModelIndex &index) const { return QSqlRelationalTableModel::flags(index); }

Qt::DropActions SqlRelationalTableModel::supportedDropActions() const { return Qt::MoveAction; }

QString SqlRelationalTableModel::selectStatement() const {
  QString stmt = QSqlRelationalTableModel::selectStatement();

  if (limit != 0) stmt.append(" LIMIT " + QString::number(50));

  return stmt;
QModelIndexList SqlRelationalTableModel::match(const QString &column, const QVariant &value, int hits, Qt::MatchFlags flags) const {
  return QSqlRelationalTableModel::match(QSqlRelationalTableModel::index(0, QSqlRelationalTableModel::fieldIndex(column)), Qt::DisplayRole, value, hits, flags);
}

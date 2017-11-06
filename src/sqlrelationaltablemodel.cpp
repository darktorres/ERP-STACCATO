#include <QDebug>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlRecord>

#include "sqlrelationaltablemodel.h"

SqlRelationalTableModel::SqlRelationalTableModel(QObject *parent) : QSqlRelationalTableModel(parent) {}

QVariant SqlRelationalTableModel::data(const int row, const int column) const { return QSqlTableModel::data(QSqlTableModel::index(row, column)); }

QVariant SqlRelationalTableModel::data(const int row, const QString &column) const {
  if (QSqlTableModel::fieldIndex(column) == -1) {
    QMessageBox::critical(nullptr, "Erro!", "Chave " + column + " não encontrada na tabela " + QSqlTableModel::tableName());
    return QVariant();
  }

  return QSqlTableModel::data(QSqlTableModel::index(row, QSqlTableModel::fieldIndex(column)));
}

bool SqlRelationalTableModel::setData(const int row, const int column, const QVariant &value) {
  if (not QSqlTableModel::setData(QSqlTableModel::index(row, column), value)) {
    QMessageBox::critical(nullptr, "Erro!", "Erro inserindo " + QSqlTableModel::record().fieldName(column) + " na tabela: " + QSqlTableModel::lastError().text());
    return false;
  }

  return true;
}

bool SqlRelationalTableModel::setData(const int row, const QString &column, const QVariant &value) {
  if (row == -1) {
    QMessageBox::critical(nullptr, "Erro!", "Erro: linha -1 SqlTableModel");
    return false;
  }

  if (QSqlTableModel::fieldIndex(column) == -1) {
    QMessageBox::critical(nullptr, "Erro!", "Chave " + column + " não encontrada na tabela " + QSqlTableModel::tableName());
    return false;
  }

  if (not QSqlTableModel::setData(QSqlTableModel::index(row, QSqlTableModel::fieldIndex(column)), value)) {
    QMessageBox::critical(nullptr, "Erro!",
                          "Erro inserindo " + column + " na tabela " + tableName() + ": " + QSqlTableModel::lastError().text() + " - linha: " + QString::number(row) + " - valor: " + value.toString());
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
}

void SqlRelationalTableModel::setLimit(int value) { limit = value; }

// REFAC: redo this to use MainWindow showErrors

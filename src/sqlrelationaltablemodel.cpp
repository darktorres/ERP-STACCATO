#include <QDebug>
#include <QSqlError>
#include <QSqlRecord>

#include "application.h"
#include "sqlrelationaltablemodel.h"

SqlRelationalTableModel::SqlRelationalTableModel(const int limit, QObject *parent) : QSqlRelationalTableModel(parent), limit(limit) {}

QVariant SqlRelationalTableModel::data(const int row, const int column) const { return QSqlRelationalTableModel::data(QSqlTableModel::index(row, column)); }

QVariant SqlRelationalTableModel::data(const QModelIndex &index, const QString &column) const { return data(index.row(), column); }

QVariant SqlRelationalTableModel::data(const int row, const QString &column) const {
  const int columnIndex = fieldIndex(column);

  if (columnIndex == -1) { return QVariant(); }

  return QSqlRelationalTableModel::data(QSqlTableModel::index(row, columnIndex));
}

bool SqlRelationalTableModel::setData(const int row, const int column, const QVariant &value) {
  if (row == -1) { return qApp->enqueueError(false, "Erro: linha -1 SqlTableModel"); }

  if (not QSqlRelationalTableModel::setData(QSqlTableModel::index(row, column), value)) {
    return qApp->enqueueError(false, "Erro inserindo " + QSqlTableModel::record().fieldName(column) + " na tabela: " + QSqlTableModel::lastError().text());
  }

  return true;
}

bool SqlRelationalTableModel::setData(const QModelIndex &index, const QString &column, const QVariant &value) { return setData(index.row(), column, value); }

bool SqlRelationalTableModel::setData(const int row, const QString &column, const QVariant &value) {
  if (row == -1) { return qApp->enqueueError(false, "Erro: linha -1 SqlTableModel"); }

  const int columnIndex = fieldIndex(column);

  if (not QSqlRelationalTableModel::setData(QSqlTableModel::index(row, columnIndex), value)) {
    return qApp->enqueueError(false, "Erro inserindo '" + column + "' na tabela '" + tableName() + "': " + QSqlTableModel::lastError().text() + " - linha: " + QString::number(row) +
                                         " - valor: " + value.toString());
  }

  return true;
}

bool SqlRelationalTableModel::setHeaderData(const QString &column, const QVariant &value) { return QSqlTableModel::setHeaderData(fieldIndex(column), Qt::Horizontal, value); }

Qt::DropActions SqlRelationalTableModel::supportedDropActions() const { return Qt::MoveAction; }

int SqlRelationalTableModel::insertRowAtEnd() {
  const int row = rowCount();
  insertRow(row);

  return row;
}

bool SqlRelationalTableModel::submitAll() {
  if (not QSqlTableModel::submitAll()) { return qApp->enqueueError(false, "Erro salvando tabela '" + QSqlTableModel::tableName() + "': " + QSqlTableModel::lastError().text()); }

  return true;
}

QString SqlRelationalTableModel::selectStatement() const { return QSqlRelationalTableModel::selectStatement() + (limit > 0 ? " LIMIT " + QString::number(limit) : ""); }

QModelIndexList SqlRelationalTableModel::match(const QString &column, const QVariant &value, int hits, Qt::MatchFlags flags) const {
  return QSqlRelationalTableModel::match(QSqlRelationalTableModel::index(0, fieldIndex(column)), Qt::DisplayRole, value, hits, flags);
}

QVector<int> SqlRelationalTableModel::multiMatch(const QVector<Condition> conditions, bool allHits) const {
  QVector<int> result;

  for (int row = 0; row < rowCount(); ++row) {
    bool ok = true;

    for (const auto &condition : conditions) {
      const QVariant value = data(row, condition.column);
      const QVariant condition_ = condition.condition;

      if ((condition.equal and value != condition_) or (not condition.equal and value == condition_)) { ok = false; }
    }

    if (ok) {
      result << row;

      if (not allHits) { break; }
    }
  }

  return result;
}

bool SqlRelationalTableModel::select() {
  //  qDebug() << "selecting " << tableName();
  //  qDebug() << "filter: " << filter();
  //  qDebug() << "stmt: " << selectStatement() << "\n";

  if (not QSqlRelationalTableModel::select()) { return qApp->enqueueError(false, "Erro lendo tabela '" + QSqlTableModel::tableName() + "': " + QSqlTableModel::lastError().text()); }

  return true;
}

void SqlRelationalTableModel::setFilter(const QString &filter) {
  //  qDebug() << "table: " << tableName();
  //  qDebug() << "filter: " << filter << "\n";

  // NOTE: use model()->query()->isActive() to know if the filter will be applied now or later

  QSqlRelationalTableModel::setFilter(filter);
}

void SqlRelationalTableModel::setSort(const QString &column, Qt::SortOrder order) { QSqlRelationalTableModel::setSort(fieldIndex(column), order); }

void SqlRelationalTableModel::setTable(const QString &tableName) {
  QSqlRelationalTableModel::setTable(tableName);
  setEditStrategy(QSqlTableModel::OnManualSubmit);
  setFilter("0");
}

int SqlRelationalTableModel::fieldIndex(const QString &fieldName, const bool silent) const {
  const int field = QSqlRelationalTableModel::fieldIndex(fieldName);

  if (field == -1 and not silent) { qApp->enqueueError(fieldName + " não encontrado na tabela " + tableName()); }

  return field;
}

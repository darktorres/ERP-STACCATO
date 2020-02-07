#include "sqltablemodel.h"

#include "application.h"

#include <QDebug>
#include <QSqlError>
#include <QSqlRecord>
#include <cmath>

SqlTableModel::SqlTableModel(const int limit, QObject *parent) : QSqlTableModel(parent), limit(limit) {}

QVariant SqlTableModel::data(const int row, const int column) const {
  if (row == -1 or column == -1) { return qApp->enqueueError(false, "Erro: linha/coluna -1 SqlTableModel"); }

  if (proxyModel) { return proxyModel->data(proxyModel->index(row, column)); }

  return QSqlTableModel::data(QSqlTableModel::index(row, column));
}

QVariant SqlTableModel::data(const int row, const QString &column) const { return data(row, fieldIndex(column)); }

bool SqlTableModel::setData(const int row, const int column, const QVariant &value) {
  if (row == -1 or column == -1) { return qApp->enqueueError(false, "Erro: linha/coluna -1 SqlTableModel"); }

  QVariant adjustedValue = value;

  if (adjustedValue.type() == QVariant::Double) { adjustedValue.setValue(roundDouble(adjustedValue.toDouble())); }

  if (proxyModel) { return proxyModel->setData(proxyModel->index(row, column), adjustedValue); }

  if (not QSqlTableModel::setData(QSqlTableModel::index(row, column), adjustedValue)) {
    return qApp->enqueueError(false, "Erro inserindo " + QSqlTableModel::record().fieldName(column) + " na tabela: " + QSqlTableModel::lastError().text());
  }

  return true;
}

bool SqlTableModel::setData(const int row, const QString &column, const QVariant &value) { return setData(row, fieldIndex(column), value); }

bool SqlTableModel::setHeaderData(const QString &column, const QVariant &value) { return QSqlTableModel::setHeaderData(fieldIndex(column), Qt::Horizontal, value); }

Qt::DropActions SqlTableModel::supportedDropActions() const { return Qt::MoveAction; }

int SqlTableModel::insertRowAtEnd() {
  const int row = rowCount();
  insertRow(row);

  return row;
}

bool SqlTableModel::submitAll() {
  if (not QSqlTableModel::submitAll()) { return qApp->enqueueError(false, "Erro salvando tabela '" + QSqlTableModel::tableName() + "': " + QSqlTableModel::lastError().text()); }

  return true;
}

QString SqlTableModel::selectStatement() const { return QSqlTableModel::selectStatement() + (limit > 0 ? " LIMIT " + QString::number(limit) : ""); }

double SqlTableModel::roundDouble(const double value) { return std::round(value * 10000.) / 10000.; }

QModelIndexList SqlTableModel::match(const QString &column, const QVariant &value, int hits, Qt::MatchFlags flags) const {
  if (proxyModel) { return proxyModel->match(QSqlTableModel::index(0, fieldIndex(column)), Qt::DisplayRole, value, hits, flags); }

  return QSqlTableModel::match(QSqlTableModel::index(0, fieldIndex(column)), Qt::DisplayRole, value, hits, flags);
}

QVector<int> SqlTableModel::multiMatch(const QVector<Condition> conditions, bool allHits) const {
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

bool SqlTableModel::select() {
  //  qDebug() << "selecting " << tableName();
  //  qDebug() << "filter: " << filter();
  //  qDebug() << "stmt: " << selectStatement() << "\n";

  if (not QSqlTableModel::select()) { return qApp->enqueueError(false, "Erro lendo tabela '" + QSqlTableModel::tableName() + "': " + QSqlTableModel::lastError().text()); }

  return true;
}

void SqlTableModel::setFilter(const QString &filter) {
  //  qDebug() << "table: " << tableName();
  //  qDebug() << "filter: " << filter << "\n";

  // NOTE: use model()->query()->isActive() to know if the filter will be applied now or later

  QSqlTableModel::setFilter(filter);
}

void SqlTableModel::setSort(const QString &column, Qt::SortOrder order) { QSqlTableModel::setSort(fieldIndex(column), order); }

void SqlTableModel::setTable(const QString &tableName) {
  QSqlTableModel::setTable(tableName);
  setEditStrategy(QSqlTableModel::OnManualSubmit);
  setFilter("0");
}

int SqlTableModel::fieldIndex(const QString &fieldName, const bool silent) const {
  const int field = QSqlTableModel::fieldIndex(fieldName);

  if (field == -1 and not silent) { qApp->enqueueError(fieldName + " não encontrado na tabela " + tableName()); }

  return field;
}

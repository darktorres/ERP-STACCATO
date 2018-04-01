#include <QSqlRecord>

#include "sqlquerymodel.h"

SqlQueryModel::SqlQueryModel(QObject *parent) : QSqlQueryModel(parent) {}

QVariant SqlQueryModel::data(const int row, const QString &column) const { return QSqlQueryModel::data(QSqlQueryModel::index(row, QSqlQueryModel::record().indexOf(column))); }

bool SqlQueryModel::setHeaderData(const QString &column, const QVariant &value) { return QSqlQueryModel::setHeaderData(QSqlQueryModel::record().indexOf(column), Qt::Horizontal, value); }

QVariant SqlQueryModel::data(const QModelIndex &index, int role) const { return QSqlQueryModel::data(index, role); }

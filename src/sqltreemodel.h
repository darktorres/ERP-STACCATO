/**************************************************************************
 * This file is part of the WebIssues Desktop Client program
 * Copyright (C) 2006 Michał Męciński
 * Copyright (C) 2007-2017 WebIssues Team
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **************************************************************************/

#pragma once

#include <QAbstractProxyModel>
#include <QSqlQueryModel>

class SqlTreeModelPrivate;

// Item model which builds a tree from multiple SQL models.
// Items at the root of the tree correspond to items from the first SQL model.
// The next nesting levels of the tree are built from the following SQL models
// by matching the foreign keys of their items with the primary keys of parent
// items. In the first SQL model, the first column must be the primary key.
// In the following SQL models, the first column must be the primary key
// and the second column must be the foreign key.

class SqlTreeModel final : public QAbstractItemModel {
  Q_OBJECT

public:
  explicit SqlTreeModel(QObject *parent);
  explicit SqlTreeModel();

  ~SqlTreeModel() final;

  auto appendModel(QSqlQueryModel *model) -> void; // Append the SQL model as the next level of the tree.
  auto columnCount(const QModelIndex &parent = QModelIndex()) const -> int final;
  auto data(const QModelIndex &index, const int role = Qt::DisplayRole) const -> QVariant final;
  auto fieldIndex(const QString &fieldName) const -> int;
  auto findIndex(const int level, const int id, const int column) const -> QModelIndex; // Return the index of an item with given level and primary key.
  auto flags(const QModelIndex &index) const -> Qt::ItemFlags final;
  auto headerData(const int section, const Qt::Orientation orientation, const int role = Qt::DisplayRole) const -> QVariant final;
  auto index(const int row, const int column, const QModelIndex &parent = QModelIndex()) const -> QModelIndex final;
  auto insertModel(QSqlQueryModel *model, const int parentLevel = -1) -> void; // Insert the SQL model at the specified parent level.
  auto levelOf(const QModelIndex &index) const -> int;                         // Return the level of the given item.
  auto mappedColumn(const QModelIndex &index) const -> int;                    // Return the column number in the SQL model of the given item.
  auto mappedRow(const QModelIndex &index) const -> int;                       // Return the row number in the SQL model of the given item.
  auto modelAt(const int level) const -> QSqlQueryModel *;                     // Return the SQL model at the given level.
  auto parent(const QModelIndex &index) const -> QModelIndex final;
  auto rawData(const int level, const int row, const int column, const int role = Qt::DisplayRole) const -> QVariant; // Return the data from the SQL model.
  auto rowCount(const QModelIndex &parent = QModelIndex()) const -> int final;
  auto rowId(const QModelIndex &index) const -> int;                               // Return the primary key of the given item.
  auto rowParentId(const QModelIndex &index) const -> int;                         // Return the foreign key of the given item.
  auto setColumnMapping(const int level, const QList<int> &columnMapping) -> void; // Map tree model columns to SQL model columns. By default all columns are mapped except the primary and foreign
                                                                                   // keys. Use -1 to indicate that a column is calculated.
  auto setHeaderData(const QString &column, const QVariant &value) -> bool;
  auto setHeaderData(const int section, const Qt::Orientation orientation, const QVariant &value, const int role = Qt::EditRole) -> bool final;
  auto setSort(const int column, const Qt::SortOrder order = Qt::AscendingOrder) -> void; // Set the sort order without updating the model.
  auto sort(const int column, const Qt::SortOrder order = Qt::AscendingOrder) -> void final;
  auto sortColumn() const -> int;          // Return the index of the current sort column.
  auto sortOrder() const -> Qt::SortOrder; // Return the current sort order.
  auto updateData() -> void;               // Rebuild the tree model after updating SQL models.

  QAbstractProxyModel *proxyModel = nullptr;

protected:
  auto updateQueries() -> void; // Called to update the queries of child SQL models after changing sort order.

private:
  // attributes
  SqlTreeModelPrivate *d = nullptr;
  // methods
  auto setData(const QModelIndex &index, const QVariant &value, const int role = Qt::DisplayRole) -> bool final;
  auto setRawData(const int level, const int row, const int column, const QVariant &value, const int role = Qt::DisplayRole) -> bool;
};

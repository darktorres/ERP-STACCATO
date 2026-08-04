#pragma once

#include <QSqlQueryModel>
#include <QTableView>

class TableView final : public QTableView {
  Q_OBJECT

public:
  explicit TableView(QWidget *parent);
  ~TableView() final = default;

  auto closePersistentEditors() -> void;
  auto columnCount() const -> int;
  auto columnIndex(const QString &column) const -> int;
  auto columnIndex(const QString &column, const bool silent) const -> int;
  // Forca o layout que o QTableView adia apos inserir/remover linhas (QTableView::rowCountChanged ->
  // doDelayedItemsLayout). Sem isso a faixa do scrollbar ainda e a antiga logo depois de uma carga de
  // pagina, e reposicionar o scroll pode ser recortado no range obsoleto.
  auto executarLayoutPendente() -> void { executeDelayedItemsLayout(); }
  auto hideColumn(const QString &column) -> void;
  auto redoView() -> void;
  auto resort() -> void;
  auto rowCount() const -> int;
  auto setAutoResize(const bool value) -> void;
  auto setCopyHeaders(const bool newCopyHeaders) -> void;
  auto setItemDelegateForColumn(const QString &column, QAbstractItemDelegate *delegate) -> void;
  auto setModel(QAbstractItemModel *model) -> void final;
  auto setPersistentColumns(const QStringList &value) -> void;
  auto showColumn(const QString &column) -> void;
  auto sortByColumn(const QString &column, Qt::SortOrder order = Qt::AscendingOrder) -> void;

protected:
  auto keyPressEvent(QKeyEvent *event) -> void final;
  auto mousePressEvent(QMouseEvent *event) -> void final;
  auto resizeEvent(QResizeEvent *event) -> void final;

private:
  // attributes
  bool autoResize = true;
  bool copyHeaders = true;
  QSqlQueryModel *baseModel = nullptr;
  QStringList persistentColumns;
  // methods
  auto openPersistentEditor(const int row, const QString &column) -> void;
  auto resizeColumnsToContents() -> void;
  auto setConnections() -> void;
  auto showContextMenu(const QPoint pos) -> void;
};

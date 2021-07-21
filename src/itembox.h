#pragma once

#include "registerdialog.h"
#include "searchdialog.h"

#include <QLineEdit>
#include <QPushButton>

class ItemBox final : public QLineEdit {
  Q_OBJECT

public:
  explicit ItemBox(QWidget *parent);
  ~ItemBox() = default;

  auto changeItem(const QVariant &newId) -> void;
  auto clear() -> void;
  auto getId() const -> QVariant;
  auto setFilter(const QString &filter) -> void;
  auto setFornecedorRep(const QString &fornecedor) -> void;
  auto setId(const QVariant &newId) -> void;
  auto setReadOnlyItemBox(const bool isReadOnly) -> void;
  auto setRegisterDialog(const QString &type) -> void;
  auto setRepresentacao(const bool isRepresentacao) -> void;
  auto setSearchDialog(SearchDialog *dialog) -> void;

signals:
  void idChanged(const QVariant &changedId);

private:
  Q_PROPERTY(QVariant id READ getId WRITE setId STORED false)
  // attributes
  bool readOnlyItemBox = false;
  QPushButton *plusButton;
  QPushButton *searchButton;
  QString registerDialogType;
  QVariant id;
  RegisterDialog *registerDialog = nullptr;
  SearchDialog *searchDialog = nullptr;
  // methods
  auto edit() -> void;
  auto mouseDoubleClickEvent(QMouseEvent *event) -> void final;
  auto resetCursor() -> void;
  auto resizeEvent(QResizeEvent *event) -> void final;
  auto search() -> void;
  auto setConnections() -> void;
  auto setIcons() -> void;
  auto sizeHint() const -> QSize override;
};

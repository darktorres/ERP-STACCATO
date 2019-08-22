#pragma once

#include <QLineEdit>
#include <QPushButton>

#include "registerdialog.h"
#include "searchdialog.h"

class ItemBox final : public QLineEdit {
  Q_OBJECT

public:
  explicit ItemBox(QWidget *parent = nullptr);
  ~ItemBox() = default;
  auto changeItem(const QVariant &newId) -> void;
  auto clear() -> void;
  auto getId() const -> QVariant;
  auto setFilter(const QString &filter) -> void;
  auto setFornecedorRep(const QString &fornecedor) -> void;
  auto setReadOnlyItemBox(const bool isReadOnly) -> void;
  auto setRegisterDialog(RegisterDialog *dialog) -> void;
  auto setRepresentacao(const bool isRepresentacao) -> void;
  auto setSearchDialog(SearchDialog *dialog) -> void;
  auto setId(const QVariant &newId) -> void;

signals:
  void idChanged(const QVariant &changedId);

private:
  Q_PROPERTY(QVariant id READ getId WRITE setId STORED false)
  // attributes
  bool readOnlyItemBox = false;
  QPushButton *searchButton;
  QPushButton *plusButton;
  QVariant id;
  RegisterDialog *registerDialog = nullptr;
  SearchDialog *searchDialog = nullptr;
  // methods
  auto edit() -> void;
  auto mouseDoubleClickEvent(QMouseEvent *event) -> void final;
  auto resetCursor() -> void;
  auto resizeEvent(QResizeEvent *event) -> void final;
  auto search() -> void;
  auto setIcons() -> void;
};

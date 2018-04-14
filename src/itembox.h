#ifndef ITEMBOX_H
#define ITEMBOX_H

#include <QLineEdit>
#include <QPushButton>

#include "registerdialog.h"
#include "searchdialog.h"

class ItemBox final : public QLineEdit {
  Q_OBJECT

public:
  explicit ItemBox(QWidget *parent);
  ~ItemBox() = default;
  auto changeItem(const QVariant &value) -> void;
  auto clear() -> void;
  auto getSearchDialog() -> SearchDialog *;
  auto getValue() const -> QVariant;
  auto setReadOnlyItemBox(const bool isReadOnly) -> void;
  auto setRegisterDialog(RegisterDialog *value) -> void;
  auto setSearchDialog(SearchDialog *value) -> void;
  auto setValue(const QVariant &value) -> void;

signals:
  void valueChanged(const QVariant &value);

private:
  Q_PROPERTY(QVariant value READ getValue WRITE setValue STORED false)
  // attributes
  bool readOnlyItemBox = false;
  QPushButton *searchButton;
  QPushButton *plusButton;
  QVariant value;
  RegisterDialog *registerDialog = nullptr;
  SearchDialog *searchDialog = nullptr;
  // methods
  auto edit() -> void;
  auto mouseDoubleClickEvent(QMouseEvent *event) -> void final;
  auto resetCursor() -> void;
  auto resizeEvent(QResizeEvent *event) -> void final;
  auto search() -> void;
  void setIcons();
};

#endif // ITEMBOX_H

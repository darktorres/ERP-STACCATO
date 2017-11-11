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
  QVariant getValue() const;
  SearchDialog *getSearchDialog();
  void changeItem(const QVariant &value);
  void clear();
  void setReadOnlyItemBox(const bool isReadOnly);
  void setRegisterDialog(RegisterDialog *value);
  void setSearchDialog(SearchDialog *value);
  void setValue(const QVariant &value);

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
  void edit();
  void mouseDoubleClickEvent(QMouseEvent *event) final;
  void resetCursor();
  void resizeEvent(QResizeEvent *event) final;
  void search();
};

#endif // ITEMBOX_H

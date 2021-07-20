#pragma once

#include <QStyledItemDelegate>

class DateFormatDelegate final : public QStyledItemDelegate {

public:
  explicit DateFormatDelegate(const int vencimentoColumn, const int tipoColumn, const bool recebimento, QObject *parent);
  explicit DateFormatDelegate(QObject *parent);
  ~DateFormatDelegate() = default;

private:
  // attributes
  bool const recebimento = false;
  int const tipoColumn = -1;
  int const vencimentoColumn = -1;
  // methods
  auto createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const -> QWidget * final;
  auto displayText(const QVariant &value, const QLocale &) const -> QString final;
};

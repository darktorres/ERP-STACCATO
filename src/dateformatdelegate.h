#pragma once

#include <QStyledItemDelegate>

class DateFormatDelegate final : public QStyledItemDelegate {
  Q_OBJECT

public:
  explicit DateFormatDelegate(const int vencimentoColumn, const int tipoColumn, const bool recebimento, QObject *parent);
  explicit DateFormatDelegate(QObject *parent);
  ~DateFormatDelegate() = default;

private:
  // attributes
  bool const recebimento;
  int const tipoColumn;
  int const vencimentoColumn;
  // methods
  auto createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const -> QWidget * final;
  auto displayText(const QVariant &value, const QLocale &locale) const -> QString final;
};

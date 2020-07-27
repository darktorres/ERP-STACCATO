#pragma once

#include <QDate>
#include <QStyledItemDelegate>

class DateFormatDelegate final : public QStyledItemDelegate {

public:
  explicit DateFormatDelegate(const int vencimentoColumn, const int tipoColumn, const bool recebimento, QObject *parent);
  explicit DateFormatDelegate(QObject *parent);
  ~DateFormatDelegate() = default;
  auto createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const -> QWidget * final;

private:
  // attributes
  const int vencimentoColumn = -1;
  const int tipoColumn = -1;
  const bool recebimento = false;
  // methods
  auto displayText(const QVariant &value, const QLocale &) const -> QString final;
};

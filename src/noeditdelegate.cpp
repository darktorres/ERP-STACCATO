#include "noeditdelegate.h"

#include <ciso646>

NoEditDelegate::NoEditDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

QWidget *NoEditDelegate::createEditor(QWidget *, const QStyleOptionViewItem &, const QModelIndex &) const { return nullptr; }

#include "qtreeviewgriddelegate.h"
#include "usersession.h"

#include <QPainter>
#include <QTreeView>

QTreeViewGridDelegate::QTreeViewGridDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

void QTreeViewGridDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const {
  QStyledItemDelegate::paint(painter, option, index);

  const auto temaOptional = UserSession::getSetting("User/tema");

  const QString tema = temaOptional ? temaOptional.value().toString() : "claro";

  const QColor color = (tema == "escuro") ? QColor(44, 44, 44) : QColor(200, 200, 200);
  painter->setPen(color);
  painter->drawRect(option.rect);
}

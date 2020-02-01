#include "qtreeviewgriddelegate.h"

#include "usersession.h"

#include <QPainter>
#include <QTreeView>

QTreeViewGridDelegate::QTreeViewGridDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

void QTreeViewGridDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const {
  QStyledItemDelegate::paint(painter, option, index);

  const QString tema = UserSession::getSetting("User/tema").value_or("claro").toString();

  const QColor color = (tema == "escuro") ? QColor(44, 44, 44) : QColor(200, 200, 200);
  painter->setPen(color);
  painter->drawRect(option.rect);
}

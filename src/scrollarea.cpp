#include "scrollarea.h"

#include <QDebug>
#include <QScrollBar>

ScrollArea::ScrollArea(QWidget *parent) : QScrollArea(parent) {}

QSize ScrollArea::sizeHint() const { return QScrollArea::sizeHint() + QSize(verticalScrollBar()->sizeHint().width(), 0); }

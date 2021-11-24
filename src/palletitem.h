#pragma once

#include "estoqueitem.h"

#include <QGraphicsItem>

class PalletItem final : public QGraphicsObject {
  Q_OBJECT

public:
  explicit PalletItem(const QString &idBloco, const QString &label, const QPointF posicao, const QRectF &size, QGraphicsItem *parent = nullptr);

  //  auto addEstoque(const QString &estoqueText) -> void;
  auto boundingRect() const -> QRectF final;
  auto getEstoques() const -> QString;
  auto getFlagHighlight() const -> bool;
  auto getIdBloco() const -> QString;
  auto getLabel() const -> QString;
  auto getPosicao() const -> QString;
  auto getSize() const -> const QRectF &;
  auto getTamanho() const -> QString;
  auto paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) -> void final;
  //  auto reorderChildren() -> void;
  auto select() -> void;
  auto setFlagHighlight(const bool value) -> void;
  auto setIdBloco(const QString &newIdBloco) -> void;
  auto setLabel(const QString &value) -> void;
  auto setSize(const QRectF &newSize) -> void;
  auto unselect() -> void;

signals:
  void selectBloco(PalletItem *item);
  void unselectBloco();

protected:
  //  auto dragEnterEvent(QGraphicsSceneDragDropEvent *event) -> void final;
  //  auto dropEvent(QGraphicsSceneDragDropEvent *event) -> void final;
  auto mousePressEvent(QGraphicsSceneMouseEvent *event) -> void final;
  auto mouseReleaseEvent(QGraphicsSceneMouseEvent *event) -> void final;

private:
  // attributes
  bool flagHighlight = false;
  bool selected = false;
  QRectF size;
  QString estoques;
  QString idBloco;
  QString label;
  // methods
  auto unselectAll() -> void;
};

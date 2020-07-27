#pragma once

#include <QGraphicsScene>

class GraphicsScene : public QGraphicsScene {

public:
  explicit GraphicsScene(QObject *parent = nullptr);
};

#pragma once

#include "xml.h"

#include <QDialog>

namespace Ui {
class XML_Viewer;
}

class XML_Viewer final : public QDialog {
  Q_OBJECT

public:
  explicit XML_Viewer(const QString &content, QWidget *parent);
  ~XML_Viewer() final;

  auto on_pushButtonDanfe_clicked() -> void;

private:
  // attributes
  XML xml;
  Ui::XML_Viewer *ui;
  // methods
  auto setConnections() -> void;
};

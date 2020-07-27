#pragma once

#include "xml.h"

#include <QDialog>

namespace Ui {
class XML_Viewer;
}

class XML_Viewer final : public QDialog {
  Q_OBJECT

public:
  explicit XML_Viewer(const QByteArray &content, QWidget *parent);
  ~XML_Viewer();

private:
  // attributes
  const QByteArray fileContent;
  XML xml;
  Ui::XML_Viewer *ui;
  // methods
  auto on_pushButtonDanfe_clicked() -> void;
};

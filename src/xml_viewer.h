#ifndef XML_VIEWER_H
#define XML_VIEWER_H

#include "dialog.h"

#include <QStandardItemModel>

namespace Ui {
class XML_Viewer;
}

class XML_Viewer final : public Dialog {
  Q_OBJECT

public:
  explicit XML_Viewer(QWidget *parent = nullptr);
  ~XML_Viewer();
  auto exibirXML(const QByteArray &content) -> void;

private:
  // attributes
  QByteArray fileContent;
  QStandardItemModel model;
  Ui::XML_Viewer *ui;
  // methods
  auto on_pushButtonDanfe_clicked() -> void;
};

#endif // XML_VIEWER_H

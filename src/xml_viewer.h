#ifndef XML_VIEWER_H
#define XML_VIEWER_H

#include <QDialog>
#include <QStandardItemModel>

#include "xml.h"

namespace Ui {
class XML_Viewer;
}

class XML_Viewer final : public QDialog {
  Q_OBJECT

public:
  explicit XML_Viewer(const QByteArray &content, QWidget *parent = nullptr);
  ~XML_Viewer();

private:
  // attributes
  const QByteArray fileContent;
  XML xml;
  Ui::XML_Viewer *ui;
  // methods
  auto on_pushButtonDanfe_clicked() -> void;
};

#endif // XML_VIEWER_H

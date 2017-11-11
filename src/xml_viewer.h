#ifndef XML_VIEWER_H
#define XML_VIEWER_H

#include <QDialog>
#include <QStandardItemModel>

namespace Ui {
class XML_Viewer;
}

class XML_Viewer final : public QDialog {
  Q_OBJECT

public:
  explicit XML_Viewer(QWidget *parent = 0);
  ~XML_Viewer();
  void exibirXML(const QByteArray &content);

private slots:
  void on_pushButtonDanfe_clicked();

private:
  // attributes
  QByteArray fileContent;
  QStandardItemModel model;
  Ui::XML_Viewer *ui;
};

#endif // XML_VIEWER_H

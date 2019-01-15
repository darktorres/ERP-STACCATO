#ifndef COLLAPSIBLEWIDGET_H
#define COLLAPSIBLEWIDGET_H

#include <QWidget>

namespace Ui {
class CollapsibleWidget;
}

class CollapsibleWidget : public QWidget {
  Q_OBJECT

public:
  explicit CollapsibleWidget(QWidget *parent = nullptr);
  ~CollapsibleWidget();
  auto setHtml(const QString text) -> void;
  auto getHtml() -> QString;

signals:
  void toggled();

private:
  // attributes
  Ui::CollapsibleWidget *ui;
  // methods
  void on_pushButton_clicked();
};

#endif // COLLAPSIBLEWIDGET_H

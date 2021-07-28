#pragma once

#include <QWidget>

namespace Ui {
class CollapsibleWidget;
}

class CollapsibleWidget : public QWidget {
  Q_OBJECT

public:
  explicit CollapsibleWidget(QWidget *parent);
  ~CollapsibleWidget();

  auto getHtml() -> QString;
  auto setHtml(const QString &text) -> void;

signals:
  void toggled();

private:
  // attributes
  Ui::CollapsibleWidget *ui;
  // methods
  auto on_pushButton_clicked() -> void;
  auto setConnections() -> void;
};

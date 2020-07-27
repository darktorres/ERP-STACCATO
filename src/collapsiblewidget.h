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
  //  auto setHtml(const QString text) -> void;
  //  auto getHtml() -> QString;
  void setDestinos(const QString &value);
  void addButton();

signals:
  void toggled();
  void resized(QSize size);

private:
  // attributes
  Ui::CollapsibleWidget *ui;
  QString destinos;
  // methods
  void on_pushButton_clicked();

  // QWidget interface
protected:
  virtual void resizeEvent(QResizeEvent *event) override;
};

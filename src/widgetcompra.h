#ifndef WIDGETCOMPRA_H
#define WIDGETCOMPRA_H

#include <QWidget>

namespace Ui {
class WidgetCompra;
}

class WidgetCompra final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetCompra(QWidget *parent = 0);
  ~WidgetCompra();
  bool updateTables();

signals:
  void errorSignal(const QString &error);
  void transactionEnded();
  void transactionStarted();

private slots:
  void on_tabWidget_currentChanged(const int &);

private:
  Ui::WidgetCompra *ui;
  void setConnections();
};

#endif // WIDGETCOMPRA_H

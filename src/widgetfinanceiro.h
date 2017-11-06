#ifndef WIDGETFINANCEIRO_H
#define WIDGETFINANCEIRO_H

#include <QWidget>

namespace Ui {
class WidgetFinanceiro;
}

class WidgetFinanceiro : public QWidget {
  Q_OBJECT

public:
  explicit WidgetFinanceiro(QWidget *parent = 0);
  ~WidgetFinanceiro();
  bool updateTables();

signals:
  void errorSignal(const QString &error);
  void transactionEnded();
  void transactionStarted();

private:
  Ui::WidgetFinanceiro *ui;
  void setConnections();
};

#endif // WIDGETFINANCEIRO_H

#ifndef WIDGETFINANCEIRO_H
#define WIDGETFINANCEIRO_H

#include <QWidget>

namespace Ui {
class WidgetFinanceiro;
}

class WidgetFinanceiro final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetFinanceiro(QWidget *parent = 0);
  ~WidgetFinanceiro();
  bool updateTables();
  void setHasError(const bool value);

signals:
  void errorSignal(const QString &error);
  void transactionEnded();
  void transactionStarted();

private:
  // attributes
  bool hasError = false;
  Ui::WidgetFinanceiro *ui;
  // methods
  void setConnections();
};

#endif // WIDGETFINANCEIRO_H

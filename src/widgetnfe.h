#ifndef WIDGETNFE_H
#define WIDGETNFE_H

#include <QWidget>

namespace Ui {
class WidgetNfe;
}

class WidgetNfe final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetNfe(QWidget *parent = 0);
  ~WidgetNfe();
  bool updateTables();
  void setHasError(const bool value);

signals:
  void errorSignal(const QString &error);
  void transactionEnded();
  void transactionStarted();

private slots:
  void on_tabWidgetNfe_currentChanged(const int);

private:
  // attributes
  bool hasError = false;
  Ui::WidgetNfe *ui;
  // methods
  void setConnections();
};

#endif // WIDGETNFE_H

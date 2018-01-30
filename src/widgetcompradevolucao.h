#ifndef WIDGETCOMPRADEVOLUCAO_H
#define WIDGETCOMPRADEVOLUCAO_H

#include <QWidget>

#include "sqlrelationaltablemodel.h"

namespace Ui {
class WidgetCompraDevolucao;
}

class WidgetCompraDevolucao final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetCompraDevolucao(QWidget *parent = nullptr);
  ~WidgetCompraDevolucao();
  bool updateTables();

signals:
  void errorSignal(const QString &error);
  void transactionEnded();
  void transactionStarted();

private slots:
  void on_pushButtonDevolucaoFornecedor_clicked();
  void on_pushButtonRetornarEstoque_clicked();
  void on_radioButtonFiltroPendente_toggled(bool checked);
  void on_table_entered(const QModelIndex &);

private:
  // attributes
  SqlRelationalTableModel model;
  Ui::WidgetCompraDevolucao *ui;
  // methods
  bool retornarEstoque(const QModelIndexList &list);
  void setupTables();
};

#endif // WIDGETCOMPRADEVOLUCAO_H

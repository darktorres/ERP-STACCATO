#ifndef WIDGETLOGISTICACOLETA_H
#define WIDGETLOGISTICACOLETA_H

#include <QWidget>

#include "sqlrelationaltablemodel.h"

namespace Ui {
class WidgetLogisticaColeta;
}

class WidgetLogisticaColeta final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetLogisticaColeta(QWidget *parent = 0);
  ~WidgetLogisticaColeta();
  bool updateTables();
  void tableFornLogistica_activated(const QString &fornecedor);

signals:
  void errorSignal(const QString &error);
  void transactionEnded();
  void transactionStarted();

private slots:
  void on_checkBoxMarcarTodos_clicked(const bool);
  void on_lineEditBusca_textChanged(const QString &);
  void on_pushButtonCancelar_clicked();
  void on_pushButtonMarcarColetado_clicked();
  void on_pushButtonReagendar_clicked();
  void on_pushButtonVenda_clicked();
  void on_table_entered(const QModelIndex &);

private:
  // attributes
  QString fornecedor;
  SqlRelationalTableModel model;
  Ui::WidgetLogisticaColeta *ui;
  // methods
  bool cadastrar(const QModelIndexList &list, const QDate &dataColeta, const QDate &dataPrevReceb);
  bool cancelar(const QModelIndexList &list);
  bool reagendar(const QModelIndexList &list, const QDate &dataPrevColeta);
  void setupTables();
};

#endif // WIDGETLOGISTICACOLETA_H

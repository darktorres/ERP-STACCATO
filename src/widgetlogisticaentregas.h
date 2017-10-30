#ifndef WIDGETLOGISTICAENTREGAS_H
#define WIDGETLOGISTICAENTREGAS_H

#include <QWidget>

#include "sqltablemodel.h"

namespace Ui {
class WidgetLogisticaEntregas;
}

class WidgetLogisticaEntregas : public QWidget {
  Q_OBJECT

public:
  explicit WidgetLogisticaEntregas(QWidget *parent = 0);
  ~WidgetLogisticaEntregas();
  bool updateTables();

signals:
  void errorSignal(const QString &error);

private slots:
  void on_lineEditBuscar_textChanged(const QString &text);
  void on_pushButtonCancelarEntrega_clicked();
  void on_pushButtonConfirmarEntrega_clicked();
  void on_pushButtonConsultarNFe_clicked();
  void on_pushButtonGerarNFeEntregar_clicked();
  void on_pushButtonImprimirDanfe_clicked();
  void on_pushButtonProtocoloEntrega_clicked();
  void on_pushButtonReagendar_clicked();
  void on_tableCalendario_clicked(const QModelIndex &index);
  void on_tableCarga_clicked(const QModelIndex &index);
  void on_tableCarga_entered(const QModelIndex &);

private:
  // attributes
  QString error;
  SqlTableModel modelCalendario;
  SqlTableModel modelCarga;
  SqlTableModel modelProdutos;
  Ui::WidgetLogisticaEntregas *ui;
  // methods
  bool cancelarEntrega(const QModelIndexList &list);
  bool confirmarEntrega(const QDateTime &dataRealEnt, const QString &entregou, const QString &recebeu);
  bool reagendar(const QModelIndexList &list, const QDate &dataPrevEnt);
  void setupTables();
  bool consultarNFe(const int idNFe, const QString &xml);
};

#endif // WIDGETLOGISTICAENTREGAS_H

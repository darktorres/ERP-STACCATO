#ifndef WIDGETLOGISTICAAGENDARENTREGA_H
#define WIDGETLOGISTICAAGENDARENTREGA_H

#include <QWidget>

#include "sqlquerymodel.h"
#include "sqlrelationaltablemodel.h"

namespace Ui {
class WidgetLogisticaAgendarEntrega;
}

class WidgetLogisticaAgendarEntrega final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetLogisticaAgendarEntrega(QWidget *parent = 0);
  ~WidgetLogisticaAgendarEntrega();
  bool updateTables();

signals:
  void errorSignal(const QString &error);
  void transactionEnded();
  void transactionStarted();

private slots:
  void on_dateTimeEdit_dateChanged(const QDate &date);
  void on_itemBoxVeiculo_textChanged(const QString &);
  void on_pushButtonAdicionarParcial_clicked();
  void on_pushButtonAdicionarProduto_clicked();
  void on_pushButtonAgendarCarga_clicked();
  void on_pushButtonReagendarPedido_clicked();
  void on_pushButtonRemoverProduto_clicked();
  void on_tableProdutos_entered(const QModelIndex &);
  void on_tableTransp2_entered(const QModelIndex &);
  void on_tableVendas_clicked(const QModelIndex &index);
  void on_tableVendas_doubleClicked(const QModelIndex &index);
  void on_tableVendas_entered(const QModelIndex &);

private:
  // attributes
  //  SqlTableModel modelConsumo;
  SqlRelationalTableModel modelProdutos;
  SqlRelationalTableModel modelTransp;
  SqlRelationalTableModel modelTransp2;
  SqlRelationalTableModel modelVendas;
  SqlQueryModel modelViewProdutos;
  Ui::WidgetLogisticaAgendarEntrega *ui;
  // methods
  bool adicionarProduto(const QModelIndexList &list);
  bool adicionarProdutoParcial(const int row, const int quantAgendar, const int quantTotal);
  bool processRows();
  bool quebrarProduto(const int row, const int quantAgendar, const int quantTotal);
  bool reagendar(const QModelIndexList &list, const QDate &dataPrev, const QString &observacao);
  void calcularDisponivel();
  void calcularPeso();
  void montaFiltro();
  void setupTables();
};

#endif // WIDGETLOGISTICAAGENDARENTREGA_H

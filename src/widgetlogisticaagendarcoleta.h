#ifndef WIDGETLOGISTICAAGENDARCOLETA_H
#define WIDGETLOGISTICAAGENDARCOLETA_H

#include <QStandardItemModel>
#include <QWidget>

#include "sqlrelationaltablemodel.h"

namespace Ui {
class WidgetLogisticaAgendarColeta;
}

class WidgetLogisticaAgendarColeta final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetLogisticaAgendarColeta(QWidget *parent = nullptr);
  ~WidgetLogisticaAgendarColeta();
  bool updateTables();
  void tableFornLogistica_activated(const QString &fornecedor);

signals:
  void errorSignal(const QString &error);
  void transactionEnded();
  void transactionStarted();

private slots:
  void calcularPeso();
  void on_checkBoxEstoque_toggled(bool checked);
  void on_checkBoxSul_toggled(bool checked);
  void on_dateTimeEdit_dateChanged(const QDate &date);
  void on_itemBoxVeiculo_textChanged(const QString &);
  void on_lineEditBusca_textChanged(const QString &);
  void on_pushButtonAdicionarProduto_clicked();
  void on_pushButtonAgendarColeta_clicked();
  void on_pushButtonCancelarCarga_clicked();
  void on_pushButtonDanfe_clicked();
  void on_pushButtonMontarCarga_clicked();
  void on_pushButtonRemoverProduto_clicked();
  void on_pushButtonVenda_clicked();
  void on_tableEstoque_entered(const QModelIndex &);

private:
  // attributes
  QString fornecedor; // REFAC: make this not class member
  SqlRelationalTableModel modelEstoque;
  SqlRelationalTableModel modelTransp;
  SqlRelationalTableModel modelTransp2;
  Ui::WidgetLogisticaAgendarColeta *ui;
  // methods
  bool adicionarProduto(const QModelIndexList &list);
  bool processRows(const QModelIndexList &list, const QDate &dataPrevColeta, const bool montarCarga = false);
  void setupTables();
  void montaFiltro();
};

#endif // WIDGETLOGISTICAAGENDARCOLETA_H

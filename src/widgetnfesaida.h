#ifndef WIDGETNFESAIDA_H
#define WIDGETNFESAIDA_H

#include <QWidget>

#include "sqlrelationaltablemodel.h"

namespace Ui {
class WidgetNfeSaida;
}

class WidgetNfeSaida final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetNfeSaida(QWidget *parent = 0);
  ~WidgetNfeSaida();
  bool updateTables();

signals:
  void errorSignal(const QString &error);
  void transactionEnded();
  void transactionStarted();

private slots:
  void on_groupBoxStatus_toggled(const bool enabled);
  void on_pushButtonCancelarNFe_clicked();
  void on_pushButtonConsultarNFe_clicked();
  void on_pushButtonExportar_clicked();
  void on_pushButtonRelatorio_clicked();
  void on_table_activated(const QModelIndex &index);
  void on_table_entered(const QModelIndex &);

private:
  // attributes
  SqlRelationalTableModel model;
  Ui::WidgetNfeSaida *ui;
  // methods
  bool atualizarNFe(const int idNFe, const QString &xml);
  void montaFiltro();
  void setupTables();
};

#endif // WIDGETNFESAIDA_H

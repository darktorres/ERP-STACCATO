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
  explicit WidgetNfeSaida(QWidget *parent = nullptr);
  ~WidgetNfeSaida();
  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool modelIsSet = false;
  SqlRelationalTableModel modelViewNFeSaida;
  Ui::WidgetNfeSaida *ui;
  // methods
  auto atualizarNFe(const int idNFe, const QString &xml) -> bool;
  auto cancelarNFe(const QString &chaveAcesso, const int row) -> bool;
  auto gravarArquivo(const QString &resposta) -> bool;
  auto montaFiltro() -> void;
  auto on_groupBoxStatus_toggled(const bool enabled) -> void;
  auto on_pushButtonCancelarNFe_clicked() -> void;
  auto on_pushButtonConsultarNFe_clicked() -> void;
  auto on_pushButtonExportar_clicked() -> void;
  auto on_pushButtonRelatorio_clicked() -> void;
  auto on_table_activated(const QModelIndex &index) -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
  auto unsetConnections() -> void;
};

#endif // WIDGETNFESAIDA_H

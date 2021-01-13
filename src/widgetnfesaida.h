#pragma once

#include "sqltablemodel.h"

#include <QTimer>
#include <QWidget>

namespace Ui {
class WidgetNfeSaida;
}

class WidgetNfeSaida final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetNfeSaida(QWidget *parent);
  ~WidgetNfeSaida();
  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool modelIsSet = false;
  QTimer timer;
  SqlTableModel modelViewNFeSaida;
  Ui::WidgetNfeSaida *ui;
  // methods
  auto atualizarNFe(const QString &resposta, const int idNFe, const QString &xml) -> void;
  auto cancelarNFe(const QString &chaveAcesso, const int row) -> void;
  auto delayFiltro() -> void;
  auto gravarArquivo(const QString &resposta, const QString &chaveAcesso) -> void;
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

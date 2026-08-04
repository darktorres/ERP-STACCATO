#pragma once

#include "sqlpaginatedmodel.h"
#include "sqlquerymodel.h"

#include <QStack>
#include <QTimer>
#include <QWidget>

namespace Ui {
class WidgetNfeSaida;
}

class WidgetNfeSaida final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetNfeSaida(QWidget *parent);
  ~WidgetNfeSaida() final;

  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool carregandoPagina = false;
  QStack<int> blockingSignals;
  SqlPaginatedModel model;
  SqlQueryModel modelResumo;
  Ui::WidgetNfeSaida *ui;
  // methods
  auto ajustarGroupBoxStatus() -> void;
  auto atualizarNFe(const QString &resposta, const int idNFe, const QString &xml) -> void;
  auto cancelarNFe(const QString &chaveAcesso, const QVariant &idNFe) -> void;
  auto gravarArquivo(const QString &resposta, const QString &chaveAcesso) -> void;
  auto montaFiltro() -> void;
  auto onTableScrolled(const int value) -> void;
  auto on_dateEditDe_dateChanged(const QDate date) -> void;
  auto on_groupBoxMes_toggled(const bool enabled) -> void;
  auto on_groupBoxStatus_toggled(const bool enabled) -> void;
  auto on_pushButtonCancelarNFe_clicked() -> void;
  auto on_pushButtonConsultarNFe_clicked() -> void;
  auto on_pushButtonExportar_clicked() -> void;
  auto on_pushButtonFollowup_clicked() -> void;
  auto on_pushButtonRelatorio_clicked() -> void;
  auto on_table_activated(const QModelIndex &index) -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
  auto unsetConnections() -> void;
};

#pragma once

#include "sqlquerymodel.h"
#include "sqltablemodel.h"

#include <QTimer>
#include <QWidget>

namespace Ui {
class WidgetLogisticaEntregas;
}

class WidgetLogisticaEntregas final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetLogisticaEntregas(QWidget *parent);
  ~WidgetLogisticaEntregas();

  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool modelIsSet = false;
  QTimer timer;
  SqlTableModel modelCalendario;
  SqlTableModel modelCarga;
  SqlTableModel modelProdutos;
  Ui::WidgetLogisticaEntregas *ui;
  // methods
  auto cancelarEntrega(const QModelIndexList &list) -> void;
  auto confirmarEntrega(const QDate dataRealEnt, const QString &entregou, const QString &recebeu) -> void;
  auto delayFiltro() -> void;
  auto gerarChecklist(const QString &folderKey, const QString &idEvento, const QString &idVenda, const QString &cliente, const QString &endereco, const QString &cep,
                      const SqlQueryModel &modelProdutosAgrupado) -> QString;
  auto gerarProtocolo(const QString &folderKey, const QString &idEvento, const QString &idVenda, const QString &cliente, const QString &telefones, const QString &endereco, const QString &cep,
                      const SqlQueryModel &modelProdutosAgrupado) -> QString;
  auto montaFiltro() -> void;
  auto on_lineEditBuscar_textChanged() -> void;
  auto on_pushButtonCancelarEntrega_clicked() -> void;
  auto on_pushButtonConfirmarEntrega_clicked() -> void;
  auto on_pushButtonConsultarNFe_clicked() -> void;
  auto on_pushButtonFollowup_clicked() -> void;
  auto on_pushButtonGerarNFe_clicked() -> void;
  auto on_pushButtonImprimirDanfe_clicked() -> void;
  auto on_pushButtonProtocoloEntrega_clicked() -> void;
  auto on_pushButtonReagendar_clicked() -> void;
  auto on_tableCalendario_clicked(const QModelIndex &index) -> void;
  auto on_tableCarga_clicked(const QModelIndex &index) -> void;
  auto processarConsultaNFe(const int idNFe, const QString &xml) -> void;
  auto reagendar(const QModelIndexList &list, const QDateTime dataVeiculo, const int idVeiculo) -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
};

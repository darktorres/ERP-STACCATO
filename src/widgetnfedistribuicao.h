#pragma once

#include "acbr.h"
#include "sqltablemodel.h"

#include <QStack>
#include <QTimer>
#include <QWidget>

namespace Ui {
class WidgetNFeDistribuicao;
}

using namespace std::chrono_literals;

class WidgetNFeDistribuicao final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetNFeDistribuicao(QWidget *parent);
  ~WidgetNFeDistribuicao() final;

  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  auto inline static tempoTimer = 15min;
  bool isSet = false;
  bool modelIsSet = false;
  int maximoNSU = 0;
  int ultimoNSU = 0;
  ACBr acbrRemoto;
  QStack<int> blockingSignals;
  QString cnpjDest;
  QString idLoja;
  QTimer timer;
  SqlTableModel model;
  Ui::WidgetNFeDistribuicao *ui;
  // methods
  auto agendarOperacao() -> void;
  auto ajustarGroupBoxStatus() -> void;
  auto buscarNFes(const QString &cnpjRaiz) -> void;
  auto buscarNSU() -> void;
  auto confirmar(const bool silent) -> void;
  auto darCiencia(const bool silent) -> void;
  auto desconhecer(const bool silent) -> void;
  auto downloadAutomatico() -> void;
  auto encontraInfCpl(const QString &xml) -> QString;
  auto encontraTransportadora(const QString &xml) -> QString;
  auto enviarComando() -> void;
  auto enviarEvento(const QString &operacao, const QVector<int> &selection) -> bool;
  auto houveConsultaEmOutroPc() -> bool;
  auto montaFiltro() -> void;
  auto naoRealizar(const bool silent) -> void;
  auto on_groupBoxStatus_toggled(const bool enabled) -> void;
  auto on_pushButtonCiencia_clicked() -> void;
  auto on_pushButtonConfirmacao_clicked() -> void;
  auto on_pushButtonDesconhecimento_clicked() -> void;
  auto on_pushButtonNaoRealizada_clicked() -> void;
  auto on_pushButtonPesquisar_clicked() -> void;
  auto on_table_activated(const QModelIndex &index) -> void;
  auto processarEventoInformacao(const QString &evento) -> void;
  auto processarEventoNFe(const QString &evento) -> void;
  auto processarEventoPrincipal(const QString &evento, const QString &idLoja) -> void;
  auto processarResposta(const QString &resposta, const QString &idLoja) -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
  auto unsetConnections() -> void;
};

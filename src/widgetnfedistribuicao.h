#pragma once

#include "acbr.h"
#include "sqltablemodel.h"

#include <QMutex>
#include <QStack>
#include <QTimer>
#include <QWidget>

namespace Ui {
class WidgetNFeDistribuicao;
}

class WidgetNFeDistribuicao final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetNFeDistribuicao(QWidget *parent);
  ~WidgetNFeDistribuicao() final;

  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  int maximoNSU = 0;
  int ultimoNSU = 0;
  QMutex mutexConsulta; // Mutex to prevent concurrent consultarSefaz() calls
  QStack<int> blockingSignals;
  QString cnpjDest;
  QString idLoja;
  QTimer timer;
  SqlTableModel model;
  Ui::WidgetNFeDistribuicao *ui;
  // methods
  auto agendarOperacao() -> void;
  auto ajustarGroupBoxStatus() -> void;
  auto atualizarNSUDoBanco() -> void;
  auto buscarNFes(const QString &cnpjRaiz, const QString &servidor, const QString &porta) -> void;
  auto buscarNSU() -> void;
  auto confirmar(ACBr &acbr, const bool silent) -> void;
  auto consultarSefaz() -> void;
  auto darCiencia(ACBr &acbr, const bool silent) -> void;
  auto desconhecer(ACBr &acbr, const bool silent) -> void;
  auto downloadAutomatico() -> void;
  auto encontraInfCpl(const QString &xml) -> QString;
  auto encontraTransportadora(const QString &xml) -> QString;
  auto enviarComando(ACBr &acbr) -> bool;
  auto enviarEvento(ACBr &acbr, const QString &operacao, const QVector<int> &selection) -> bool;
  auto podeConsultarAgora() -> bool;
  auto calcularMinutosProximaConsulta() -> int;
  auto calcularMinutosProximaConsultaGeral() -> int;
  auto montaFiltro() -> void;
  auto naoRealizar(ACBr &acbr, const bool silent) -> void;
  auto on_groupBoxStatus_toggled(const bool enabled) -> void;
  auto on_pushButtonCiencia_clicked() -> void;
  auto on_pushButtonConfirmacao_clicked() -> void;
  auto on_pushButtonDesconhecimento_clicked() -> void;
  auto on_pushButtonNaoRealizada_clicked() -> void;
  auto on_pushButtonBaixarNFe_clicked() -> void;
  auto on_table_activated(const QModelIndex &index) -> void;
  auto processarEventoInformacao(const QString &evento) -> void;
  auto processarEventoNFe(const QString &evento) -> void;
  auto processarEventoPrincipal(const QString &evento, const QString &idLoja) -> void;
  auto processarResposta(const QString &resposta, const QString &idLoja) -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
  auto unsetConnections() -> void;
};

#pragma once

#include "acbr.h"
#include "sqltablemodel.h"

#include <QTimer>
#include <QWidget>

namespace Ui {
class NFeDistribuicao;
}

class NFeDistribuicao : public QWidget {
  Q_OBJECT

public:
  explicit NFeDistribuicao(QWidget *parent = nullptr);
  ~NFeDistribuicao();
  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool modelIsSet = false;
  ACBr acbrRemoto;
  QTimer timer;
  SqlTableModel model;
  Ui::NFeDistribuicao *ui;
  // methods
  auto buscarNSU() -> void;
  auto confirmar() -> void;
  auto darCiencia() -> bool;
  auto downloadAutomatico() -> void;
  auto encontraInfCpl(const QString &xml) -> QString;
  auto encontraTransportadora(const QString &xml) -> QString;
  auto enviarEvento(const QString &codigoEvento, const QString &operacao) -> bool;
  auto on_pushButtonCiencia_clicked() -> void;
  auto on_pushButtonConfirmacao_clicked() -> void;
  auto on_pushButtonDesconhecimento_clicked() -> void;
  auto on_pushButtonNaoRealizada_clicked() -> void;
  auto on_pushButtonPesquisar_clicked() -> void;
  auto on_table_activated(const QModelIndex &index) -> void;
  auto pesquisarNFes(const QString &resposta, const QString &idLoja) -> bool;
  auto setConnections() -> void;
  auto setupTables() -> void;
};

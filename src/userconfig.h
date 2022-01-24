#pragma once

#include <QDialog>

namespace Ui {
class UserConfig;
}

class UserConfig final : public QDialog {
  Q_OBJECT

public:
  explicit UserConfig(QWidget *parent);
  ~UserConfig();

private:
  // attributes
  Ui::UserConfig *ui;
  // methods
  auto getSettings() -> void;
  auto hideWidgets() -> void;
  auto on_pushButtonAlterarDados_clicked() -> void;
  auto on_pushButtonComprasFolder_clicked() -> void;
  auto on_pushButtonEmailTeste_clicked() -> void;
  auto on_pushButtonEntregasPdfFolder_clicked() -> void;
  auto on_pushButtonEntregasXmlFolder_clicked() -> void;
  auto on_pushButtonOrcamentosFolder_clicked() -> void;
  auto on_pushButtonSalvar_clicked() -> void;
  auto on_pushButtonVendasFolder_clicked() -> void;
  auto preencherComboBoxMonitorar() -> void;
  auto salvarDadosEmail() -> void;
  auto salvarDadosNFe() -> void;
  auto setConnections() -> void;
};

#ifndef USERCONFIG_H
#define USERCONFIG_H

#include "dialog.h"

namespace Ui {
class UserConfig;
}

class UserConfig final : public Dialog {
  Q_OBJECT

public:
  explicit UserConfig(QWidget *parent = nullptr);
  ~UserConfig();

private:
  // attributes
  Ui::UserConfig *ui;
  // methods
  auto on_pushButtonAlterarDados_clicked() -> void;
  auto on_pushButtonComprasFolder_clicked() -> void;
  auto on_pushButtonEntregasPdfFolder_clicked() -> void;
  auto on_pushButtonEntregasXmlFolder_clicked() -> void;
  auto on_pushButtonOrcamentosFolder_clicked() -> void;
  auto on_pushButtonSalvar_clicked() -> void;
  auto on_pushButtonVendasFolder_clicked() -> void;
};

#endif // USERCONFIG_H

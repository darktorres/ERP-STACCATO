#pragma once

#include "sqlquerymodel.h"
#include "sqltablemodel.h"

#include <QDialog>

namespace Ui {
class Estoque;
}

class Estoque final : public QDialog {
  Q_OBJECT

public:
  enum class FieldColors {
    White = 0,     // Não processado
    Green = 1,     // Ok
    Yellow = 2,    // Quant difere
    Red = 3,       // Não encontrado
    DarkGreen = 4, // Consumo
    Cyan = 5       // Devolução
  };
  Q_ENUM(FieldColors)

  explicit Estoque(const QVariant &idEstoque, QWidget *parent);
  ~Estoque();

  auto criarConsumo(const int idVendaProduto2, const double quant = 0) -> void;
  static auto desfazerConsumo(const int idVendaProduto2) -> void;

private:
  // attributes
  QString const idEstoque;
  SqlQueryModel modelEstoque;
  SqlTableModel modelConsumo;
  Ui::Estoque *ui;
  // methods
  auto dividirCompra(const int idVendaProduto2, const double quant) -> void;
  auto exibirNota() -> void;
  auto limitarAlturaTabela() -> void;
  auto on_pushButtonExibirNfe_clicked() -> void;
  auto preencherRestante() -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
};

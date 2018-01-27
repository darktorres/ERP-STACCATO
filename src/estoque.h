#ifndef ESTOQUE_H
#define ESTOQUE_H

#include <QDialog>

#include "sqlrelationaltablemodel.h"

namespace Ui {
class Estoque;
}

class Estoque final : public QDialog {
  Q_OBJECT

public:
  // REFAC: turn showWindow into a enum
  Estoque(const QString &idEstoque, const bool showWindow = true, QWidget *parent = 0);
  ~Estoque();
  bool criarConsumo(const int idVendaProduto, const double quant = 0);

private slots:
  void on_pushButtonExibirNfe_clicked();
  void on_tableConsumo_entered(const QModelIndex &);
  void on_tableEstoque_activated(const QModelIndex &);
  void on_tableEstoque_entered(const QModelIndex &);

private:
  // attributes
  const QString idEstoque;
  SqlRelationalTableModel model;
  SqlRelationalTableModel modelConsumo;
  SqlRelationalTableModel modelViewConsumo;
  SqlRelationalTableModel modelCompra;
  Ui::Estoque *ui;

  enum class FieldColors {
    White = 0,     // Não processado
    Green = 1,     // Ok
    Yellow = 2,    // Quant difere
    Red = 3,       // Não encontrado
    DarkGreen = 4, // Consumo
    Cyan = 5       // Devolução
  };

  // methods
  // REFAC: remove idEstoque and just use the class one?
  bool quebrarCompra(const int idVendaProduto, const double quant);
  bool viewRegisterById(const bool showWindow);
  void calcularRestante();
  void exibirNota();
  void setupTables();
  bool atualizaQuantEstoque(const int idVendaProduto);
};

#endif // ESTOQUE_H

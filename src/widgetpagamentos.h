#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QDoubleSpinBox>
#include <QHBoxLayout>

namespace Ui {
class WidgetPagamentos;
}

class WidgetPagamentos final : public QWidget {
  Q_OBJECT

public:
  enum class Tipo { Nulo, Compra, Venda };

  explicit WidgetPagamentos(QWidget *parent);
  ~WidgetPagamentos();

  auto getCredito() const -> double;
  auto getTotalPag() -> double;
  auto resetarPagamentos() -> void;
  auto setCredito(const double creditoCliente) -> void;
  auto setFrete(double value) -> void;
  auto setFretePagoLoja() -> void;
  auto setIdOrcamento(const QString &value) -> void;
  auto setRepresentacao(const bool isRepresentacao) -> void;
  auto setTipo(const Tipo &novoTipo) -> void;
  auto setTotal(double value) -> void;
  auto verifyFields() -> void;

  // attributes
  int pagamentos = 0; // TODO: rename this to count
  QList<QCheckBox *> listCheckBoxRep;
  QList<QComboBox *> listTipoPgt;
  QList<QComboBox *> listTipoData;
  QList<QComboBox *> listParcela;
  QList<QDoubleSpinBox *> listValorPgt;
  QList<QDateEdit *> listDataPgt;
  QList<QLineEdit *> listObservacao;

signals:
  void montarFluxoCaixa();

private:
  // attributes
  bool representacao = false;
  bool fretePagoLoja = false;
  double frete = 0;
  double total = 0;
  double credito = 0;
  double creditoRestante = 0;
  QString idOrcamento;
  Tipo tipo = Tipo::Nulo;
  Ui::WidgetPagamentos *ui;
  // methods
  auto calculaCreditoRestante() -> void;
  auto calcularTotal() -> void;
  auto checkBoxRep(QFrame *frame, QHBoxLayout *layout) -> void;
  auto comboBoxData(QHBoxLayout *layout) -> QComboBox *;
  auto comboBoxParc(QHBoxLayout *layout) -> void;
  auto comboBoxPgtCompra(QHBoxLayout *layout) -> void;
  auto comboBoxPgtVenda(QFrame *frame, QHBoxLayout *layout) -> void;
  auto dateEditPgt(QHBoxLayout *layout) -> QDateEdit *;
  auto doubleSpinBoxPgt(QHBoxLayout *layout) -> void;
  auto labelPagamento(QHBoxLayout *layout) -> void;
  auto lineEditPgt(QHBoxLayout *layout) -> void;
  auto on_comboBoxPgt_currentTextChanged(const int index, const QString &text) -> void;
  auto on_doubleSpinBoxPgt_valueChanged(const int index) -> void;
  auto on_pushButtonAdicionarPagamento_clicked(const bool addFrete = true) -> void;
  auto on_pushButtonFreteLoja_clicked() -> void;
  auto on_pushButtonLimparPag_clicked() -> void;
  auto on_pushButtonPgtLoja_clicked() -> void;
  auto prepararPagamentosRep() -> void;
  auto setConnections() -> void;
};

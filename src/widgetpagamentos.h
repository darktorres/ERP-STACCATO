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
  Q_ENUM(Tipo)

  explicit WidgetPagamentos(QWidget *parent);
  ~WidgetPagamentos();

  auto getCredito() const -> double;
  auto getTotalPag() -> double;
  auto resetarPagamentos() -> void;
  auto setCredito(const double creditoCliente) -> void;
  auto setFrete(const double value) -> void;
  auto setFretePagoLoja() -> void;
  auto setIdOrcamento(const QString &value) -> void;
  auto setRepresentacao(const bool isRepresentacao) -> void;
  auto setTipo(const Tipo novoTipo) -> void;
  auto setTotal(const double value) -> void;
  auto verifyFields() -> void;

  // TODO: make private?
  // attributes
  int qtdPagamentos = 0;
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
  bool fretePagoLoja = false;
  bool representacao = false;
  double credito = 0;
  double creditoRestante = 0;
  double frete = 0;
  double total = 0;
  QString idOrcamento;
  Tipo tipo = Tipo::Nulo;
  Ui::WidgetPagamentos *ui;
  // methods
  auto calculaCreditoRestante() -> void;
  auto calcularTotal() -> void;
  auto checkBoxRep(QFrame *frame, QLayout *layout) -> void;
  auto comboBoxParc(QLayout *layout) -> void;
  auto comboBoxPgtCompra(QLayout *layout) -> void;
  auto comboBoxPgtVenda(QFrame *frame, QLayout *layout) -> void;
  auto comboBoxTipoData(QLayout *layout) -> void;
  auto dateEditPgt(QLayout *layout) -> void;
  auto deleteButton(QLayout *layout) -> void;
  auto doubleSpinBoxPgt(QLayout *layout) -> void;
  auto labelPagamento(QLayout *layout) -> void;
  auto lineEditPgt(QLayout *layout) -> void;
  auto on_comboBoxPgt_currentTextChanged(const QString &text) -> void;
  auto on_comboBoxTipoData_currentTextChanged(const QString &text) -> void;
  auto on_doubleSpinBoxPgt_valueChanged(const int index) -> void;
  auto on_pushButtonAdicionarPagamento_clicked(const bool addFrete = true) -> void;
  auto on_pushButtonFreteLoja_clicked() -> void;
  auto on_pushButtonLimparPag_clicked() -> void;
  auto on_pushButtonPgtLoja_clicked() -> void;
  auto prepararPagamentosRep() -> void;
  auto setConnections() -> void;
};

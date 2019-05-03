#ifndef WIDGETPAGAMENTOS_H
#define WIDGETPAGAMENTOS_H

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
  enum class Tipo { Compra, Venda };
  explicit WidgetPagamentos(QWidget *parent = nullptr);
  ~WidgetPagamentos();
  auto getCredito() const -> double;
  auto getTotalPag() -> double;
  auto resetarPagamentos() -> void;
  auto setCredito(const double creditoCliente) -> void;
  auto setFrete(double value) -> void;
  auto setIdOrcamento(const QString &value) -> void;
  auto setRepresentacao(const bool isRepresentacao) -> void;
  auto setTipo(const Tipo &value) -> void;
  auto setTotal(double value) -> void;
  auto verifyFields() -> bool;

  // REFAC: make this private
  QList<QCheckBox *> listCheckBoxRep;
  QList<QComboBox *> listComboData;
  QList<QComboBox *> listComboParc;
  QList<QComboBox *> listComboPgt;
  QList<QDateEdit *> listDatePgt;
  QList<QDoubleSpinBox *> listDoubleSpinPgt;
  QList<QLineEdit *> listLinePgt;

signals:
  void montarFluxoCaixa();

private:
  // attributes
  bool representacao = false;
  double frete = 0;
  double total = 0;
  double credito = 0;
  QString idOrcamento;
  Tipo tipo;
  Ui::WidgetPagamentos *ui;
  // methods
  auto calcularTotal() -> void;
  auto checkBoxRep(QFrame *frame, QHBoxLayout *layout) -> void;
  auto comboBoxData(QHBoxLayout *layout) -> QComboBox *;
  auto comboBoxParc(QHBoxLayout *layout) -> void;
  auto comboBoxPgtCompra(QHBoxLayout *layout) -> bool;
  auto comboBoxPgtVenda(QFrame *frame, QHBoxLayout *layout) -> bool;
  auto dateEditPgt(QHBoxLayout *layout) -> QDateEdit *;
  auto doubleSpinBoxPgt(QHBoxLayout *layout) -> void;
  auto labelPagamento(QHBoxLayout *layout) -> void;
  auto lineEditPgt(QHBoxLayout *layout) -> void;
  auto on_comboBoxPgt_currentTextChanged(const int index, const QString &text) -> void;
  auto on_pushButtonAdicionarPagamento_clicked() -> void;
  auto on_pushButtonFreteLoja_clicked() -> void;
  auto on_pushButtonLimparPag_clicked() -> void;
  auto on_pushButtonPgtLoja_clicked() -> void;
};

#endif // WIDGETPAGAMENTOS_H

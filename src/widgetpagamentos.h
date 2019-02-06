#ifndef WIDGETPAGAMENTOS_H
#define WIDGETPAGAMENTOS_H

#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QDoubleSpinBox>
#include <QScrollArea>

namespace Ui {
class WidgetPagamentos;
}

// TODO: refactor this to Widget and implement stuff directly in ui?

class WidgetPagamentos final : public QWidget {
  Q_OBJECT

public:
  enum class Tipo { Compra, Venda };
  explicit WidgetPagamentos(QWidget *parent = nullptr);
  ~WidgetPagamentos();
  auto adicionarPagamentoCompra(const double restante) -> bool;
  auto adicionarPagamentoVenda(const bool representacao, const QString &idOrcamento, const double creditoTotal, const double restante) -> bool;
  auto resetarPagamentos() -> void;

  // REFAC: make this private
  double total = 0;
  QList<QCheckBox *> listCheckBoxRep;
  QList<QComboBox *> listComboData;
  QList<QComboBox *> listComboParc;
  QList<QComboBox *> listComboPgt;
  QList<QDateEdit *> listDatePgt;
  QList<QDoubleSpinBox *> listDoubleSpinPgt;
  QList<QLineEdit *> listLinePgt;

  void setTipo(const Tipo &value);

signals:
  void montarFluxoCaixa();
  //  void valueChanged();

private:
  // attributes
  Ui::WidgetPagamentos *ui;
  Tipo tipo;
  // methods
  auto on_comboBoxPgt_currentTextChanged(const int index, const QString &text, const double creditoTotal) -> void;
  auto on_comboBoxPgt_currentTextChanged(const int index, const QString &text) -> void;
  void on_pushButtonAdicionarPagamento_clicked();
  void on_pushButtonLimparPag_clicked();
  void calcularTotal();
};

#endif // WIDGETPAGAMENTOS_H

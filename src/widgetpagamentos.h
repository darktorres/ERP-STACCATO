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

class WidgetPagamentos final : public QScrollArea {
  Q_OBJECT

public:
  explicit WidgetPagamentos(QWidget *parent = nullptr);
  ~WidgetPagamentos();
  auto adicionarPagamentoCompra(const double restante) -> void;
  auto adicionarPagamentoVenda(const bool representacao, const QString &idOrcamento, const double creditoTotal, const double restante) -> void;
  auto resetarPagamentos() -> void;

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
  void valueChanged();

private:
  // attributes
  Ui::WidgetPagamentos *ui;
  // methods
  auto on_comboBoxPgt_currentTextChanged(const int index, const QString &text, const double creditoTotal) -> void;
  auto on_comboBoxPgt_currentTextChanged(const int index, const QString &text) -> void;
};

#endif // WIDGETPAGAMENTOS_H

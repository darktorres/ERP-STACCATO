#ifndef WIDGETPAGAMENTOS_H
#define WIDGETPAGAMENTOS_H

#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QDoubleSpinBox>
#include <QScrollArea>
#include <QWidget>

namespace Ui {
class WidgetPagamentos;
}

class WidgetPagamentos final : public QScrollArea {
  Q_OBJECT

public:
  explicit WidgetPagamentos(QWidget *parent = 0);
  ~WidgetPagamentos();
  void adicionarPagamentoCompra(const double restante);
  void adicionarPagamentoVenda(const bool representacao, const QString &idOrcamento, const double creditoTotal, const double restante);
  void resetarPagamentos();

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
  void on_comboBoxPgt_currentTextChanged(const int index, const QString &text, const double creditoTotal);
  void on_comboBoxPgt_currentTextChanged(const int index, const QString &text);
};

#endif // WIDGETPAGAMENTOS_H
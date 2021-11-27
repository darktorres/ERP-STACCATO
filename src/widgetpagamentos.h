#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QLabel>

namespace Ui {
class WidgetPagamentos;
}

class Pagamento : public QWidget {
  Q_OBJECT

public:
  enum class TipoPgt { Normal, Frete, ST };
  Q_ENUM(TipoPgt)

  explicit Pagamento(TipoPgt tipoPgt, QWidget *parent) : QWidget(parent), tipoPgt(tipoPgt) {}

  int posicao = 0;
  QLabel *label = nullptr;
  QCheckBox *checkBoxRep = nullptr;
  QComboBox *comboTipoPgt = nullptr;
  QComboBox *comboTipoData = nullptr;
  QComboBox *comboParcela = nullptr;
  QDoubleSpinBox *valorPgt = nullptr;
  QDateEdit *dataPgt = nullptr;
  QLineEdit *observacao = nullptr;
  TipoPgt tipoPgt;
};

class WidgetPagamentos final : public QWidget {
  Q_OBJECT

public:
  enum class Tipo { Nulo, Compra, Venda };
  Q_ENUM(Tipo)

  explicit WidgetPagamentos(QWidget *parent);
  ~WidgetPagamentos();

  auto getTotalPag() -> double;
  auto resetarPagamentos() -> void;
  auto setCredito(const double creditoCliente) -> void;
  auto setFrete(const double newFrete) -> void;
  auto setFretePagoLoja() -> void;
  auto setIdOrcamento(const QString &newIdOrcamento) -> void;
  auto setRepresentacao(const bool isRepresentacao) -> void;
  auto setST(const double newSt) -> void;
  auto setTipo(const Tipo novoTipo) -> void;
  auto setTotal(const double newTotal) -> void;
  auto verifyFields() -> void;

  QList<Pagamento *> pagamentos;
  Pagamento *pgtFrete = nullptr;
  Pagamento *pgtSt = nullptr;

signals:
  void montarFluxoCaixa();

private:
  // attributes
  bool fretePagoLoja = false; // rename to isFretePagoLoja
  bool representacao = false; // rename to isRepresentacao
  double credito = 0;
  double creditoRestante = 0;
  double frete = 0;
  double st = 0;
  double total = 0;
  QString idOrcamento;
  Tipo tipo = Tipo::Nulo;
  Ui::WidgetPagamentos *ui;
  // methods
  auto buscarTiposPgtsCompra(QComboBox *comboBoxPgt) -> void;
  auto buscarTiposPgtsVenda(QComboBox *comboBoxPgt) -> void;
  auto calculaCreditoRestante() -> void;
  auto calcularRestante() -> void;
  auto calcularTotal() -> void;
  auto checkBoxRep(Pagamento *pgt) const -> void;
  auto comboBoxParc(Pagamento *pgt) const -> void;
  auto comboBoxPgtCompra(Pagamento *pgt) -> void;
  auto comboBoxPgtVenda(Pagamento *pgt) -> void;
  auto comboBoxTipoData(Pagamento *pgt) const -> void;
  auto dateEditPgt(Pagamento *pgt) const -> void;
  //  auto deleteButton(QLayout *layout) -> void;
  auto doubleSpinBoxPgt(Pagamento *pgt) -> void;
  auto insertPgtInScrollArea(Pagamento *pgt) -> void;
  auto labelPagamento(Pagamento *pgt) -> void;
  auto lineEditPgt(Pagamento *pgt) const -> void;
  auto on_comboBoxPgt_currentTextChanged(const QString &text) -> void;
  auto on_comboBoxTipoData_currentTextChanged(const QString &text) -> void;
  auto on_doubleSpinBoxPgt_valueChanged() -> void;
  auto on_pushButtonAdicionarPagamento_clicked(const Pagamento::TipoPgt tipoPgt = Pagamento::TipoPgt::Normal) -> void;
  //  auto on_pushButtonDelete_clicked() -> void;
  auto on_pushButtonFreteLoja_clicked() -> void;
  auto on_pushButtonLimparPag_clicked() -> void;
  auto on_pushButtonPgtLoja_clicked() -> void;
  //  auto prepararPagamentosRep() -> void;
  auto setConnections() -> void;
};

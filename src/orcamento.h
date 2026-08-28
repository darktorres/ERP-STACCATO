#pragma once

#include "log.h"
#include "registerdialog.h"

#include <QStack>

#include <optional>

namespace Ui {
class Orcamento;
}

// Item-entry sub-form: caixas is the canonical driver, quant/totalItem are derived from it.
struct ItemFormState {
  double caixas = 0.;
  double stepQt = 0.;  // ui->doubleSpinBoxQuant->singleStep(), set when a produto is selected
  double stepCx = 0.;  // ui->doubleSpinBoxCaixas->singleStep(), set when a produto is selected
  double prcUn = 0.;   // set when a produto is selected
  double descPct = 0.; // desconto do item, 0-100

  auto quant() const -> double { return caixas * stepQt; }
  auto totalItem() const -> double { return quant() * prcUn * (1. - descPct / 100.); }
};

// Header totals: descontoReais is the canonical driver (matches the on-disk source of truth), total is always derived.
struct OrcamentoTotais {
  double subTotalBruto = 0.;
  double subTotalLiq = 0.;
  double frete = 0.;
  double descontoReais = 0.;

  auto descontoPorc() const -> double { return qFuzzyIsNull(subTotalLiq) ? 0. : descontoReais / subTotalLiq * 100.; }
  auto total() const -> double { return subTotalLiq - descontoReais + frete; }
};

struct FreteResultado {
  double valor = 0.;
  double minimo = 0.;
  bool forcado = false; // true only for the "serviços especiais" branch, which overrides even a floor-only request
};

class Orcamento final : public RegisterDialog {
  Q_OBJECT

public:
  explicit Orcamento(QWidget *parent);
  ~Orcamento();

  auto show() -> void;

private:
  // attributes
  bool replicando = false;
  bool canChangeFrete = false;
  bool carregando = false; // true durante viewRegister/select — separa "já estava errado no disco" de "esta edição quebrou"
  bool currentItemIsEstoque = false;
  bool isReadOnly = false;
  double minimoFrete = 0.;
  double freteMinimoAtual = 0.;
  double porcFrete = 0.;
  int currentItemIsPromocao = 0;
  int currentRowItem = -1;
  ItemFormState itemFormState;
  OrcamentoTotais totais;
  QDataWidgetMapper mapperItem;
  QList<QSqlRecord> backupItem;
  QStack<int> blockingSignals;
  SqlTableModel modelItem;
  TotaisTrace totaisTrace;
  Ui::Orcamento *ui;
  // methods
  auto adicionarItem(const Tipo tipoItem = Tipo::Cadastrar) -> void;
  auto aplicarDescontoAosItens(const double descontoPorc) -> void;
  auto aplicarFreteCalculado(const FreteResultado &resultado) -> void;
  // Ponto único de escrita em 'totais': atribui e registra no trace. Toda mutação passa por aqui.
  auto aplicarTotais(const QString &origem, const QString &argumento, const OrcamentoTotais &novo) -> void;
  auto atualizaReplica() -> void;
  auto atualizarItem() -> void;
  auto buscarConsultor() -> void;
  auto buscarIdVenda() -> void;
  auto buscarParametrosFrete() -> void;
  auto cadastrar() -> void final;
  auto calcPrecoGlobalTotal() -> void;
  auto calcularFrete() -> std::optional<FreteResultado>;
  auto calcularPeso() -> double;
  auto calcularPesoTotal() -> void;
  auto calcularTotais() -> std::tuple<double, double, double>;
  auto clearFields() -> void final;
  auto connectLineEditsToDirty() -> void final;
  auto dataItem(const QString &key) const -> QVariant;
  auto eventFilter(QObject *obj, QEvent *event) -> bool final;
  auto generateId() -> void;
  auto montarChecks() -> QVector<TotaisCheck>;
  auto montarDiagnostico(const QString &contexto, const QVector<TotaisCheck> &checks) -> TotaisDiagnostico;
  auto montarFlags() const -> QString;
  auto montarItensSujos() -> QString;
  auto newRegister() -> bool final;
  auto novoItem() -> void;
  auto on_checkBoxFreteManual_clicked(const bool checked) -> void;
  auto on_checkBoxRepresentacao_toggled(const bool checked) -> void;
  auto on_dataEmissao_dateChanged(const QDate date) -> void;
  auto on_doubleSpinBoxCaixas_valueChanged(const double caixas) -> void;
  auto on_doubleSpinBoxDescontoGlobalReais_valueChanged(const double descontoReais) -> void;
  auto on_doubleSpinBoxDescontoGlobal_valueChanged(const double descontoPorc) -> void;
  auto on_doubleSpinBoxDesconto_valueChanged(const double desconto) -> void;
  auto on_doubleSpinBoxFrete_valueChanged(const double frete) -> void;
  auto on_doubleSpinBoxQuant_valueChanged(const double quant) -> void;
  auto on_doubleSpinBoxTotalItem_valueChanged() -> void;
  auto on_doubleSpinBoxTotal_valueChanged(const double total) -> void;
  auto on_itemBoxCliente_textChanged() -> void;
  auto on_itemBoxEndereco_idChanged() -> void;
  auto on_itemBoxProduto_idChanged() -> void;
  auto on_itemBoxProfissional_idChanged() -> void;
  auto on_itemBoxVendedor_textChanged() -> void;
  auto on_pushButtonAbrirReplicaDe_clicked() -> void;
  auto on_pushButtonAbrirReplicadoEm_clicked() -> void;
  auto on_pushButtonAbrirVenda_clicked() -> void;
  auto on_pushButtonAdicionarItem_clicked() -> void;
  auto on_pushButtonApagarOrc_clicked() -> void;
  auto on_pushButtonAtualizarItem_clicked() -> void;
  auto on_pushButtonAtualizarOrcamento_clicked() -> void;
  auto on_pushButtonCadastrarOrcamento_clicked() -> void;
  auto on_pushButtonCalculadora_clicked() -> void;
  auto on_pushButtonGerarExcel_clicked() -> void;
  auto on_pushButtonGerarPdf_clicked() -> void;
  auto on_pushButtonGerarVenda_clicked() -> void;
  auto on_pushButtonModelo3d_clicked() -> void;
  auto on_pushButtonDescerItem_clicked() -> void;
  auto on_pushButtonRemoverItem_clicked() -> void;
  auto on_pushButtonReplicar_clicked() -> void;
  auto on_pushButtonSubirItem_clicked() -> void;
  auto on_tableProdutos_selectionChanged() -> void;
  auto redoBackupItem() -> void;
  auto registerMode() -> void final;
  // Registra no trace uma operação que mexeu no agregado dos ITENS sem tocar em 'totais'.
  auto registrarItens(const QString &origem, const QString &argumento) -> void;
  auto removeItem() -> void;
  auto renderItemForm() -> void;
  auto renderTotais() -> void;
  auto resizeSpinBoxes() -> void;
  auto savingProcedures() -> void final;
  auto setConnections() -> void;
  auto setDataItem(const QString &key, const QVariant &value, const bool adjustValue = true) -> void;
  auto setItemBoxes() -> void;
  auto setarParametrosProduto() -> void;
  auto setupMapper() -> void final;
  auto setupTables() -> void;
  auto successMessage() -> void final;
  auto swapItens(const int rowA, const int rowB) -> void;
  auto unsetConnections() -> void;
  auto updateMode() -> void final;
  auto verificaCadastroCliente() -> void;
  auto verificaDisponibilidadeEstoque() -> void;
  auto verificaSeFoiAlterado() -> void;
  auto verificaServicosEspeciais() -> bool;
  // Mesma checagem de verificarTotais(), mas só grava log — nunca lança. Roda a cada render, para
  // pegar o instante em que o invariante quebra em vez de só no save, muito depois.
  auto verificarInvariante(const QString &contexto) -> void;
  auto verificarTotais() -> void;
  auto verifyFields() -> void final;
  auto viewRegister() -> bool final;
  // pure reducers — no Qt, no side effects, one per possible user edit
  static auto reduceSetCaixas(ItemFormState state, const double caixasRaw) -> ItemFormState;
  static auto reduceSetQuant(ItemFormState state, const double quantRaw) -> ItemFormState;
  static auto reduceSetDesconto(ItemFormState state, const double descPct) -> ItemFormState;
  static auto reduceSetTotalItem(ItemFormState state, const double totalItemValor) -> ItemFormState;
  static auto reduceSetFrete(OrcamentoTotais totaisAtuais, const double frete) -> OrcamentoTotais;
  static auto reduceSetDescontoReais(OrcamentoTotais totaisAtuais, const double descontoReais) -> OrcamentoTotais;
  static auto reduceSetDescontoPorc(OrcamentoTotais totaisAtuais, const double descontoPorc) -> OrcamentoTotais;
  static auto reduceSetTotal(OrcamentoTotais totaisAtuais, const double total) -> OrcamentoTotais;
};

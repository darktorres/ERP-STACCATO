#pragma once

#include "acbr.h"
#include "sqlquery.h"
#include "sqltablemodel.h"

#include <QDataWidgetMapper>
#include <QDate>
#include <QDialog>
#include <QStack>
#include <QTextStream>

namespace Ui {
class CadastrarNFe;
}

// Reforma Tributária 2025 - Progressive tax rates
// Based on LC 214/2025 transition schedule
struct AliquotasReformaTributaria {
  double pIBSUF;      // IBS State rate
  double pIBSMun;     // IBS Municipal rate
  double pCBS;        // CBS rate
  double fatorNovosTributos;  // Percentage of new taxes to apply (0.0 to 1.0)
  double fatorAntigosTributos; // Percentage of old taxes to apply (1.0 to 0.0)

  // Calculate rates based on NFe emission date
  static AliquotasReformaTributaria calcular(const QDate &dataEmissao) {
    const int ano = dataEmissao.year();

    // Final rates (2033+)
    constexpr double IBSUF_FINAL = 12.0;
    constexpr double IBSMUN_FINAL = 5.7;
    constexpr double CBS_FINAL = 8.8;

    // Transition schedule per LC 214/2025
    double fator = 0.0;
    switch (ano) {
      case 2026: fator = 0.0; break;  // Test period - use fixed rates below
      case 2027: fator = 0.10; break;
      case 2028: fator = 0.20; break;
      case 2029: fator = 0.30; break;
      case 2030: fator = 0.40; break;
      case 2031: fator = 0.50; break;
      case 2032: fator = 0.90; break;
      default:
        if (ano >= 2033) fator = 1.0;
        else fator = 0.0;  // Before 2026
        break;
    }

    AliquotasReformaTributaria aliq;
    aliq.fatorNovosTributos = fator;
    aliq.fatorAntigosTributos = 1.0 - fator;

    if (ano == 2026) {
      // 2026 Test period: fixed low rates for system testing (LC 214/2025 Art. 343)
      // Total: ~1% (IBS 0.1% + CBS 0.9%) - verify against latest official rates
      aliq.pIBSUF = 0.1;
      aliq.pIBSMun = 0.0;  // Municipalities not yet participating in 2026
      aliq.pCBS = 0.9;
    } else if (ano < 2026) {
      // Before 2026: new taxes not yet in effect
      aliq.pIBSUF = 0.0;
      aliq.pIBSMun = 0.0;
      aliq.pCBS = 0.0;
    } else {
      aliq.pIBSUF = IBSUF_FINAL * fator;
      aliq.pIBSMun = IBSMUN_FINAL * fator;
      aliq.pCBS = CBS_FINAL * fator;
    }

    return aliq;
  }
};

class CadastrarNFe final : public QDialog {
  Q_OBJECT

public:
  // TODO: adicionar tipo Frete
  // TODO: adicionar tipo Serviço
  // TODO: separar Entrada em DevolucaoCliente, DevolucaoFornecedor
  enum class Tipo { Entrada, Saida, Futura, SaidaAposFutura };
  Q_ENUM(Tipo)

  explicit CadastrarNFe(const QString &idVenda, const QStringList &items, const Tipo tipo, QWidget *parent);
  ~CadastrarNFe();

private:
  // attributes
  bool manterAberto = false;
  QDataWidgetMapper mapper;
  QStack<int> blockingSignals;
  QString arquivo;
  QString chaveAcesso;
  QString const idVenda;
  QString emailContabilidade;
  QString emailLogistica;
  QString xml;
  SqlQuery queryIBGEDest;
  SqlQuery queryIBGEEmit;
  SqlQuery queryPartilhaInter;
  SqlQuery queryPartilhaIntra;
  SqlTableModel modelLoja;
  SqlTableModel modelVenda;
  SqlTableModel modelProduto;
  Tipo const tipo;
  Ui::CadastrarNFe *ui;
  // methods
  auto atualizarNFe(const int idNFe) -> void;
  auto buscarAliquotas() -> void;
  auto calculaCofins() -> void;
  auto calculaDigitoVerificador() -> void;
  auto calculaIcms() -> void;
  auto calculaIBS() -> void;
  auto calculaCBS() -> void;
  auto calculaIS() -> void;
  auto calculaPis() -> void;
  auto calculaSt() -> void;
  auto validarClassTrib(const QString &cClassTrib, const QString &tipo) -> void;
  auto carregarArquivo(ACBr &acbr, const QString &filePath) -> void;
  auto clearStr(const QString &str) const -> QString;
  auto criarChaveAcesso() -> void;
  auto enviarEmail(ACBr &acbr, const QString &filePath) -> void;
  auto enviarNFe(ACBr &acbr, const QString &filePath, const int idNFe) -> void;
  auto gerarNota(ACBr &acbr) -> QString;
  auto listarCfop() -> void;
  auto montarXML() -> QString;
  auto on_checkBoxFrete_toggled(const bool checked) -> void;
  auto on_comboBoxCOFINScst_currentTextChanged(const QString &text) -> void;
  auto on_comboBoxCfop_currentTextChanged(const QString &text) -> void;
  auto on_comboBoxDestinoOperacao_currentTextChanged(const QString &text) -> void;
  auto on_comboBoxICMSModBcSt_currentIndexChanged(const int index) -> void;
  auto on_comboBoxICMSModBc_currentIndexChanged(const int index) -> void;
  auto on_comboBoxICMSOrig_currentIndexChanged(const int index) -> void;
  auto on_comboBoxIPIcst_currentTextChanged(const QString &text) -> void;
  auto on_comboBoxPIScst_currentTextChanged(const QString &text) -> void;
  auto on_comboBoxRegime_currentTextChanged(const QString &text) -> void;
  auto on_comboBoxSituacaoTributaria_currentTextChanged(const QString &text) -> void;
  auto on_doubleSpinBoxCOFINSpcofins_valueChanged() -> void;
  auto on_doubleSpinBoxCOFINSvbc_valueChanged() -> void;
  auto on_doubleSpinBoxCOFINSvcofins_valueChanged() -> void;
  auto on_doubleSpinBoxICMSpicms_valueChanged() -> void;
  auto on_doubleSpinBoxICMSpicmsst_valueChanged() -> void;
  auto on_doubleSpinBoxICMSvbc_valueChanged() -> void;
  auto on_doubleSpinBoxICMSvbcst_valueChanged() -> void;
  auto on_doubleSpinBoxICMSvicms_valueChanged() -> void;
  auto on_doubleSpinBoxICMSvicmsst_valueChanged() -> void;
  auto on_doubleSpinBoxPISppis_valueChanged() -> void;
  auto on_doubleSpinBoxPISvbc_valueChanged() -> void;
  auto on_doubleSpinBoxPISvpis_valueChanged() -> void;
  auto on_doubleSpinBoxValorFrete_valueChanged(const double value) -> void;
  auto on_itemBoxCliente_textChanged() -> void;
  auto on_itemBoxEnderecoEntrega_textChanged() -> void;
  auto on_itemBoxEnderecoFaturamento_textChanged() -> void;
  auto on_itemBoxLoja_textChanged() -> void;
  auto on_itemBoxVeiculo_textChanged() -> void;
  auto on_pushButtonConsultarCadastro_clicked() -> void;
  auto on_pushButtonEnviarNFE_clicked() -> void;
  auto on_pushButtonPrevia_clicked() -> void;
  auto on_tableItens_dataChanged(const QModelIndex &index) -> void;
  auto on_tableItens_selectionChanged() -> void;
  auto preCadastrarNota() -> int;
  auto preencherDadosNFe() -> void;
  auto preencherDestinatario() -> void;
  auto preencherEmitente() -> void;
  auto preencherImpostos() -> void;
  auto preencherNumeroNFe() -> void;
  auto preencherTotais() -> void;
  auto preencherTransportadora() -> void;
  auto preencherTransporte() -> void;
  auto preencherVolumes() -> void;
  auto prepararNFe(const QStringList &items) -> void;
  auto processarResposta(const QString &resposta, const QString &filePath, const int idNFe, ACBr &acbr) -> void;
  auto removerNota(const int idNFe) -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
  auto unsetConnections() -> void;
  auto updateComplemento() -> void;
  auto updateTotais() -> void;
  auto validarDados() -> void;
  auto validarRegras(ACBr &acbr, const QString &filePath) -> bool;
  auto validarSchema(ACBr &acbr, const QString &filePath) -> void;
  auto writeComplemento(QTextStream &stream) const -> void;
  auto writeDestinatario(QTextStream &stream) const -> void;
  auto writeEmitente(QTextStream &stream) const -> void;
  auto writeIdentificacao(QTextStream &stream) -> void;
  auto writePagamento(QTextStream &stream) -> void;
  auto writeProduto(QTextStream &stream) const -> void;
  auto writeTotal(QTextStream &stream) const -> void;
  auto writeTransportadora(QTextStream &stream) const -> void;
  auto writeVolume(QTextStream &stream) const -> void;
};

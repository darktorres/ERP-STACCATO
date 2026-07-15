#pragma once

#include "acbr.h"
#include "application.h"
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

// Reforma Tributária 2025 - alíquotas de IBS/CBS por ano de emissão.
// Fonte: NT 2025.002-RTC v1.40 (SEFAZ), regras UB22/UB40/UB56 - ver NT/NT_2025.002_v1.40_RTC_NF-e_IBS_CBS_IS Final.pdf
// 2025/2026 (Art. 343/346 da LC 214/2025) e 2027/2028 (Art. 344, só IBS) já têm alíquota fixa
// publicada. A alíquota de referência da CBS para 2027-2035 depende de resolução do Senado
// Federal ainda não publicada (Decreto 12.955/2026, que regulamenta a CBS, só define a
// METODOLOGIA de cálculo, não o percentual) - lança exceção em vez de arriscar um valor
// inventado. Idem para IBS a partir de 2029 (NT diz "alíquota a ser publicada").
struct AliquotasReformaTributaria {
  double pIBSUF;  // IBS Estadual
  double pIBSMun; // IBS Municipal
  double pCBS;    // CBS

  static AliquotasReformaTributaria calcular(const QDate &dataEmissao) {
    const int ano = dataEmissao.year();

    AliquotasReformaTributaria aliq;

    if (ano < 2025) {
      aliq.pIBSUF = 0.0;
      aliq.pIBSMun = 0.0;
      aliq.pCBS = 0.0;
      return aliq;
    }

    if (ano <= 2026) {
      aliq.pIBSUF = 0.1;  // Art. 343 da LC 214/2025
      aliq.pIBSMun = 0.0; // Art. 343 da LC 214/2025
      aliq.pCBS = 0.9;    // Art. 346 da LC 214/2025
      return aliq;
    }

    if (ano <= 2028) {
      // Art. 344 da LC 214/2025 só define IBS. A alíquota de referência da CBS para
      // 2027-2035 depende de resolução do Senado Federal ainda não publicada (Decreto
      // 12.955/2026 define só a metodologia de cálculo, não o percentual em si).
      throw RuntimeException("Reforma Tributária: alíquota da CBS para " + QString::number(ano) +
                              " ainda não foi fixada por resolução do Senado Federal (Decreto 12.955/2026 define só a "
                              "metodologia, não o percentual). Confirme o valor vigente com a contabilidade/"
                              "consultoria tributária antes de emitir NF-e neste período.");
    }

    throw RuntimeException("Reforma Tributária: alíquotas de IBS/CBS para " + QString::number(ano) +
                            " ainda não publicadas na NT 2025.002 ('alíquota de referência a ser publicada'). "
                            "Confirme com a contabilidade/consultoria tributária antes de emitir NF-e neste período.");
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

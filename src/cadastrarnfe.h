#pragma once

#include "acbr.h"
#include "sqlquery.h"
#include "sqltablemodel.h"

#include <QDataWidgetMapper>
#include <QDialog>
#include <QTextStream>

namespace Ui {
class CadastrarNFe;
}

class CadastrarNFe final : public QDialog {
  Q_OBJECT

public:
  enum class Tipo { Entrada, Saida, Futura, SaidaAposFutura };
  explicit CadastrarNFe(const QString &idVenda, const QStringList &items, const Tipo tipo, QWidget *parent);
  ~CadastrarNFe();
  auto show() -> void;

private:
  // attributes
  const Tipo tipo;
  const QString idVenda;
  bool manterAberto = false;
  QDataWidgetMapper mapper;
  SqlQuery queryIBGEDest;
  SqlQuery queryIBGEEmit;
  SqlQuery queryPartilhaInter;
  SqlQuery queryPartilhaIntra;
  QString arquivo;
  QString chaveAcesso;
  QString xml;
  SqlTableModel modelLoja;
  SqlTableModel modelViewProdutoEstoque;
  SqlTableModel modelVenda;
  Ui::CadastrarNFe *ui;
  // methods
  auto alterarCertificado(const QString &text) -> void;
  auto buscarAliquotas() -> void;
  auto cadastrar(const int &idNFe) -> void;
  auto calculaCofins() -> void;
  auto calculaDigitoVerificador(QString &chave) -> void;
  auto calculaIcms() -> void;
  auto calculaPis() -> void;
  auto calculaSt() -> void;
  auto carregarArquivo(ACBr &acbrRemoto, const QString &filePath) -> void;
  auto clearStr(const QString &str) const -> QString;
  auto criarChaveAcesso() -> void;
  auto enviarEmail(ACBr &acbrRemoto) -> void;
  auto enviarNFe(ACBr &acbrRemoto, const QString &filePath, const int idNFe) -> void;
  auto gerarNota(ACBr &acbrRemoto) -> QString;
  auto listarCfop() -> void;
  auto montarXML() -> QString;
  auto on_checkBoxFrete_toggled(bool checked) -> void;
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
  auto on_doubleSpinBoxCOFINSpcofins_valueChanged(const double) -> void;
  auto on_doubleSpinBoxCOFINSvbc_valueChanged(const double) -> void;
  auto on_doubleSpinBoxCOFINSvcofins_valueChanged(const double) -> void;
  auto on_doubleSpinBoxICMSpicms_valueChanged(const double) -> void;
  auto on_doubleSpinBoxICMSpicmsst_valueChanged(const double) -> void;
  auto on_doubleSpinBoxICMSvbc_valueChanged(const double) -> void;
  auto on_doubleSpinBoxICMSvbcst_valueChanged(const double) -> void;
  auto on_doubleSpinBoxICMSvicms_valueChanged(const double) -> void;
  auto on_doubleSpinBoxICMSvicmsst_valueChanged(const double) -> void;
  auto on_doubleSpinBoxPISppis_valueChanged(const double) -> void;
  auto on_doubleSpinBoxPISvbc_valueChanged(const double) -> void;
  auto on_doubleSpinBoxPISvpis_valueChanged(const double) -> void;
  auto on_doubleSpinBoxValorFrete_valueChanged(const double value) -> void;
  auto on_itemBoxCliente_textChanged(const QString &) -> void;
  auto on_itemBoxEnderecoEntrega_textChanged(const QString &) -> void;
  auto on_itemBoxEnderecoFaturamento_textChanged(const QString &) -> void;
  auto on_itemBoxVeiculo_textChanged(const QString &) -> void;
  auto on_pushButtonConsultarCadastro_clicked() -> void;
  auto on_pushButtonEnviarNFE_clicked() -> void;
  auto on_tableItens_clicked(const QModelIndex &index) -> void;
  auto on_tableItens_dataChanged(const QModelIndex index) -> void;
  auto preCadastrarNota() -> int;
  auto preencherDadosNFe() -> void;
  auto preencherDestinatario() -> void;
  auto preencherEmitente() -> void;
  auto preencherImpostos() -> void;
  auto preencherNumeroNFe() -> void;
  auto preencherTotais() -> void;
  auto preencherTransporte(const QStringList &items) -> void;
  auto prepararNFe(const QStringList &items) -> void;
  auto processarResposta(const QString &resposta, const QString &filePath, const int &idNFe, ACBr &acbrRemoto) -> void;
  auto removerNota(const int idNFe) -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
  auto unsetConnections() -> void;
  auto updateComplemento() -> void;
  auto updateTotais() -> void;
  auto validar() -> bool;
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

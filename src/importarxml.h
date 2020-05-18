#pragma once

#include "estoqueproxymodel.h"
#include "sqltablemodel.h"
#include "xml.h"

#include <QDate>
#include <QDialog>

namespace Ui {
class ImportarXML;
}

class ImportarXML final : public QDialog {
  Q_OBJECT

  struct NCM {
    double mva4;
    double mva12;
    double aliq;
  };

  struct ProdutoEstoque {
    int idProduto;
    int idEstoque;
    double estoqueRestante;
    double valorUnid;
  };

public:
  explicit ImportarXML(const QStringList &idsCompra, const QDate &dataFaturamento, QWidget *parent = nullptr);
  ~ImportarXML();

private:
  // attributes
  const QDate dataFaturamento;
  const QStringList idsCompra;
  SqlTableModel modelCompra;
  SqlTableModel modelConsumo;
  SqlTableModel modelEstoque;
  SqlTableModel modelVenda;
  SqlTableModel modelEstoque_compra;
  SqlTableModel modelNFe;
  SqlTableModel modelPagamento;
  Ui::ImportarXML *ui;

  enum class FieldColors {
    None = 0,      // Não processado
    Green = 1,     // Ok
    Yellow = 2,    // Quant difere
    Red = 3,       // Não encontrado
    DarkGreen = 4, // Consumo
  };

  // methods
  auto associarDiferente(const int rowCompra, const int rowEstoque, double &estoquePareado, bool &repareado) -> bool;
  auto associarIgual(const int rowCompra, const int rowEstoque) -> bool;
  auto buscaNCM(const QString &ncm) -> std::optional<ImportarXML::NCM>;
  auto buscarCaixas(const int rowEstoque) -> std::optional<double>;
  auto cadastrarNFe(XML &xml, const double gare) -> bool;
  auto cadastrarProdutoEstoque(const QVector<ProdutoEstoque> &tuples) -> bool;
  auto calculaGare(const XML &xml) -> std::optional<double>;
  auto criarConsumo(const int rowCompra, const int rowEstoque) -> bool;
  auto criarPagamentoGare(const double valor, const XML &xml) -> bool;
  auto dividirCompra(const int rowCompra, const double quantAdicionar) -> bool;
  auto dividirVenda(const int rowVenda, const double quantAdicionar) -> std::optional<int>;
  auto importar() -> bool;
  auto lerXML() -> bool;
  auto limparAssociacoes() -> bool;
  auto mapTuples() -> QVector<ImportarXML::ProdutoEstoque>;
  auto on_checkBoxSemLote_toggled(const bool checked) -> void;
  auto on_pushButtonCancelar_clicked() -> void;
  auto on_pushButtonImportar_clicked() -> void;
  auto on_pushButtonProcurar_clicked() -> void;
  auto parear() -> bool;
  auto percorrerXml(XML &xml) -> bool;
  auto perguntarLocal(XML &xml) -> bool;
  auto reparear(const QModelIndex &index) -> void;
  auto salvarDadosCompra() -> bool;
  auto salvarDadosVenda() -> bool;
  auto setConnections() -> void;
  auto setupTables() -> void;
  auto unsetConnections() -> void;
  auto updateTableData(const QModelIndex &topLeft) -> void;
  auto verificaExiste(const QString &chaveAcesso) -> bool;
  auto verifyFields() -> bool;
};

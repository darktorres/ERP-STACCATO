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

public:
  explicit ImportarXML(const QStringList &idsCompra, const QDate &dataReal, QWidget *parent = nullptr);
  ~ImportarXML();

private:
  struct ProdutoEstoque {
    int idProduto;
    int idEstoque;
    double estoqueRestante;
    double valorUnid;
  };

  // attributes
  const QDate dataReal;
  const QStringList idsCompra;
  SqlTableModel modelCompra;
  SqlTableModel modelConsumo;
  SqlTableModel modelEstoque;
  SqlTableModel modelVenda;
  SqlTableModel modelEstoque_compra;
  SqlTableModel modelNFe;
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
  auto salvarDadosCompra() -> bool;
  auto buscarCaixas(const int rowEstoque) -> std::optional<double>;
  auto cadastrarNFe(XML &xml) -> bool;
  auto cadastrarProdutoEstoque(const QVector<ProdutoEstoque> &tuples) -> bool;
  auto criarConsumo(const int rowCompra, const int rowEstoque) -> bool;
  auto dividirCompra(const int rowCompra, const double quantAdicionar) -> bool;
  auto dividirVenda(const int rowVenda, const double quantAdicionar) -> std::optional<int>;
  auto importar() -> bool;
  auto inserirItemModel(const XML &xml) -> bool;
  auto lerXML() -> bool;
  auto limparAssociacoes() -> bool;
  auto mapTuples() -> QVector<ProdutoEstoque>;
  auto on_checkBoxSemLote_toggled(const bool checked) -> void;
  auto on_pushButtonCancelar_clicked() -> void;
  auto on_pushButtonImportar_clicked() -> void;
  auto on_pushButtonProcurar_clicked() -> void;
  auto parear() -> bool;
  auto percorrerXml(XML &xml, const QStandardItem *item) -> bool;
  auto perguntarLocal(XML &xml) -> bool;
  auto reparear(const QModelIndex &index) -> void;
  auto salvarDadosVenda() -> bool;
  auto setConnections() -> void;
  auto setupTables() -> void;
  auto unsetConnections() -> void;
  auto updateTableData(const QModelIndex &topLeft) -> void;
  auto verificaExiste(const QString &chaveAcesso) -> bool;
  auto verifyFields() -> bool;
};

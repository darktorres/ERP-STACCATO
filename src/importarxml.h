#ifndef IMPORTARXML_H
#define IMPORTARXML_H

#include <QDataWidgetMapper>
#include <QDate>
#include <QFileDialog>

#include "dialog.h"
#include "sqlrelationaltablemodel.h"
#include "xml.h"

namespace Ui {
class ImportarXML;
}

class ImportarXML final : public Dialog {
  Q_OBJECT

public:
  explicit ImportarXML(const QStringList &idsCompra, const QDateTime &dataReal, QWidget *parent = nullptr);
  ~ImportarXML();

private:
  // attributes
  const QDateTime dataReal;
  const QStringList idsCompra;
  SqlRelationalTableModel modelCompra;
  SqlRelationalTableModel modelConsumo;
  SqlRelationalTableModel modelEstoque;
  SqlRelationalTableModel modelEstoque_nfe;
  SqlRelationalTableModel modelEstoque_compra;
  SqlRelationalTableModel modelNFe;
  Ui::ImportarXML *ui;

  enum class FieldColors {
    White = 0,     // Não processado
    Green = 1,     // Ok
    Yellow = 2,    // Quant difere
    Red = 3,       // Não encontrado
    DarkGreen = 4, // Consumo
  };

  // methods
  auto associarItens(const int rowCompra, const int rowEstoque, double &estoqueConsumido) -> bool;
  auto buscarCaixas(const int rowEstoque) -> std::optional<double>;
  auto cadastrarNFe(XML &xml) -> bool;
  auto cadastrarProdutoEstoque() -> bool;
  auto criarConsumo(const int rowCompra, const int rowEstoque, const double quantAdicionar) -> bool;
  auto importar() -> bool;
  auto inserirItemSql(XML &xml) -> bool;
  auto inserirNoSqlModel(XML &xml, const QStandardItem *item) -> bool;
  auto lerXML(QFile &file) -> bool;
  auto limparAssociacoes() -> bool;
  auto on_pushButtonCancelar_clicked() -> void;
  auto on_pushButtonImportar_clicked() -> void;
  auto on_pushButtonProcurar_clicked() -> void;
  auto on_tableCompra_entered(const QModelIndex &) -> void;
  auto on_tableConsumo_entered(const QModelIndex &) -> void;
  auto on_tableEstoque_entered(const QModelIndex &) -> void;
  auto parear() -> bool;
  auto perguntarLocal(XML &xml) -> bool;
  auto procurar() -> void;
  auto produtoCompativel(const int rowCompra, const QString &codComercialEstoque) -> bool;
  auto setupTables(const QStringList &idsCompra) -> void;
  auto verificaCNPJ(const XML &xml) -> bool;
  auto verificaExiste(const XML &xml) -> bool;
  auto wrapParear() -> void; // REFAC: simplify this
};

#endif // IMPORTARXML_H

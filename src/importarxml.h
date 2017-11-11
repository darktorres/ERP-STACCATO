#ifndef IMPORTARXML_H
#define IMPORTARXML_H

#include <QDataWidgetMapper>
#include <QDate>
#include <QFileDialog>

#include "sqlrelationaltablemodel.h"
#include "xml.h"

namespace Ui {
class ImportarXML;
}

class ImportarXML final : public QDialog {
  Q_OBJECT

public:
  explicit ImportarXML(const QStringList &idsCompra, const QDateTime &dataReal, QWidget *parent = 0);
  ~ImportarXML();

signals:
  void errorSignal(const QString &error);
  void transactionEnded();
  void transactionStarted();

private slots:
  void on_pushButtonCancelar_clicked();
  void on_pushButtonImportar_clicked();
  void on_pushButtonProcurar_clicked();
  void on_tableCompra_entered(const QModelIndex &);
  void on_tableConsumo_entered(const QModelIndex &);
  void on_tableEstoque_entered(const QModelIndex &);

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
  bool associarItens(const int rowCompra, const int rowEstoque, double &estoqueConsumido);
  bool cadastrarNFe(XML &xml);
  bool cadastrarProdutoEstoque();
  bool criarConsumo(const int rowCompra, const int rowEstoque, const double quantAdicionar);
  bool importar();
  bool inserirItemSql(XML &xml);
  bool inserirNoSqlModel(XML &xml, const QStandardItem *item);
  bool lerXML(QFile &file);
  bool limparAssociacoes();
  bool parear();
  bool perguntarLocal(XML &xml);
  bool verificaCNPJ(const XML &xml);
  bool verificaExiste(const XML &xml);
  void procurar();
  void setupTables(const QStringList &idsCompra);
  void WrapParear(); // REFAC: simplify this
};

#endif // IMPORTARXML_H

#pragma once

#include "sqltablemodel.h"

#include <QDialog>
#include <QTimer>

namespace Ui {
class SearchDialog;
}

class SearchDialog final : public QDialog {
  Q_OBJECT

  struct FullTextIndex {
    QString index;
    QString placeHolder;
  };

  // TODO: criar um enum identificando o tipo, no lugar de usar o nome das tabelas para identificar

public:
  // Factory Methods
  static auto cliente(QWidget *parent) -> SearchDialog *;
  static auto conta(QWidget *parent) -> SearchDialog *;
  static auto enderecoCliente(QWidget *parent) -> SearchDialog *;
  static auto fornecedor(QWidget *parent) -> SearchDialog *;
  static auto loja(QWidget *parent) -> SearchDialog *;
  static auto nfe(const bool todasNFes, const bool somenteCD, QWidget *parent) -> SearchDialog *;
  static auto produto(const bool permitirDescontinuados, const bool silent, const bool showAllProdutos, const bool compraAvulsa, QWidget *parent) -> SearchDialog *;
  static auto profissional(const bool mostrarNaoHa, QWidget *parent) -> SearchDialog *;
  static auto transportadora(QWidget *parent) -> SearchDialog *;
  static auto usuario(QWidget *parent) -> SearchDialog *;
  static auto veiculo(QWidget *parent) -> SearchDialog *;
  static auto vendedor(QWidget *parent) -> SearchDialog *;

  static auto getCacheLoja() -> SearchDialog *;
  static auto getCacheConta() -> SearchDialog *;
  static auto clearCache() -> void;

  auto getText(const QVariant &id) -> QString;
  auto setFilter(const QString &newFilter) -> void;
  auto setFornecedorRep(const QString &newFornecedorRep) -> void;
  auto setRepresentacao(const bool newValue) -> void;
  auto show() -> void;

signals:
  void itemSelected(const QVariant &id);

private:
  explicit SearchDialog(const QString &title, const QString &table, const QString &primaryKey, const QStringList &textKeys, const QList<FullTextIndex> &fullTextIndexes, const QString &filter,
                        const QString &sortColumn, const bool naoListar, QWidget *parent);
  ~SearchDialog() final;

  // attributes
  bool caching = false;
  bool compraAvulsa = false;
  bool isRepresentacao = false;
  bool naoListarBuscaVazia = true;
  bool permitirDescontinuados = false;
  bool showAllProdutos = false;
  bool silent = false;
  QList<FullTextIndex> const fullTextIndexes;
  QString const primaryKey;
  QString filter;
  QString fornecedorRep;
  QStringList const textKeys;
  SqlTableModel model;
  Ui::SearchDialog *ui;
  // methods
  auto buscaProduto(const QString &searchFilter) -> void;
  auto hideColumns(const QStringList &columns) -> void;
  auto on_lineEditBusca_textChanged() -> void;
  auto on_pushButtonModelo3d_clicked() -> void;
  auto on_pushButtonSelecionar_clicked() -> void;
  auto on_table_doubleClicked() -> void;
  auto on_table_selectionChanged() -> void;
  auto sendUpdateMessage(const QModelIndex &index) -> void;
  auto setConnections() -> void;
  auto setHeaderData(const QString &column, const QString &newHeader) -> void;
  auto setupSearchWidgets() -> void;
  auto setupTables(const QString &table, const QString &sortColumn) -> void;

  inline static SearchDialog *cacheLoja = nullptr;
  inline static SearchDialog *cacheConta = nullptr;

  inline static QHash<QString, QString> cacheSearchDialog;
};

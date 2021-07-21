#pragma once

#include "sqltablemodel.h"

#include <QDialog>
#include <QTimer>

namespace Ui {
class SearchDialog;
}

class SearchDialog final : public QDialog {
  Q_OBJECT

public:
  // Factory Methods
  static auto cliente(QWidget *parent) -> SearchDialog *;
  static auto conta(QWidget *parent) -> SearchDialog *;
  static auto enderecoCliente(QWidget *parent) -> SearchDialog *;
  static auto fornecedor(QWidget *parent) -> SearchDialog *;
  static auto loja(QWidget *parent) -> SearchDialog *;
  static auto nfe(QWidget *parent) -> SearchDialog *;
  static auto produto(const bool permitirDescontinuados, const bool silent, const bool showAllProdutos, const bool compraAvulsa, QWidget *parent) -> SearchDialog *;
  static auto profissional(const bool mostrarNaoHa, QWidget *parent) -> SearchDialog *;
  static auto transportadora(QWidget *parent) -> SearchDialog *;
  static auto usuario(QWidget *parent) -> SearchDialog *;
  static auto veiculo(QWidget *parent) -> SearchDialog *;
  static auto vendedor(QWidget *parent) -> SearchDialog *;

  static auto getCacheLoja() -> SearchDialog *;
  static auto getCacheConta() -> SearchDialog *;
  static auto clearCache() -> void;

  auto getFilter() const -> QString;
  auto getText(const QVariant &id) -> QString;
  auto setFilter(const QString &newFilter) -> void;
  auto setFornecedorRep(const QString &newFornecedorRep) -> void;
  auto setRepresentacao(const bool isRepresentacao) -> void;

signals:
  void itemSelected(const QVariant &id);

private:
  explicit SearchDialog(const QString &title, const QString &table, const QString &primaryKey, const QStringList &textKeys, const QString &fullTextIndex, const QString &filter,
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
  QString const fullTextIndex;
  QString const primaryKey;
  QString filter;
  QString fornecedorRep;
  QStringList const textKeys;
  QTimer timer;
  SqlTableModel model;
  Ui::SearchDialog *ui;
  // methods
  auto buscaProduto(const QString &searchFilter) -> void;
  auto delayFiltro() -> void;
  auto hideColumns(const QStringList &columns) -> void;
  auto on_lineEditBusca_textChanged() -> void;
  auto on_pushButtonModelo3d_clicked() -> void;
  auto on_pushButtonSelecionar_clicked() -> void;
  auto on_radioButtonProdAtivos_toggled(const bool) -> void;
  auto on_radioButtonProdDesc_toggled(const bool) -> void;
  auto on_table_clicked(const QModelIndex &index) -> void;
  auto on_table_doubleClicked(const QModelIndex &) -> void;
  auto sendUpdateMessage(const QModelIndex &index) -> void;
  auto setConnections() -> void;
  auto setHeaderData(const QString &column, const QString &newHeader) -> void;
  auto setupTables(const QString &table, const QString &sortColumn) -> void;

  inline static SearchDialog *cacheLoja;
  inline static SearchDialog *cacheConta;

  inline static QHash<QString, QString> cacheSearchDialog;
};

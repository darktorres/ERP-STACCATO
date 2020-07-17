#pragma once

#include "sqltablemodel.h"

#include <QDialog>

namespace Ui {
class SearchDialog;
}

class SearchDialog final : public QDialog {
  Q_OBJECT

public:
  ~SearchDialog() final;
  auto getFilter() const -> QString;
  auto getText(const QVariant &id) -> QString;
  auto setFilter(const QString &newFilter) -> void;
  auto setFornecedorRep(const QString &newFornecedorRep) -> void;
  auto setRepresentacao(const bool isRepresentacao) -> void;
  auto show() -> void;
  auto showMaximized() -> void;

  // Factory Methods
  static auto cliente(QWidget *parent) -> SearchDialog *;
  static auto conta(QWidget *parent) -> SearchDialog *;
  static auto enderecoCliente(QWidget *parent) -> SearchDialog *;
  static auto fornecedor(QWidget *parent) -> SearchDialog *;
  static auto loja(QWidget *parent) -> SearchDialog *;
  static auto produto(const bool permitirDescontinuados, const bool silent, const bool showAllProdutos, const bool compraAvulsa, QWidget *parent) -> SearchDialog *;
  static auto profissional(const bool mostrarNaoHa, QWidget *parent) -> SearchDialog *;
  static auto transportadora(QWidget *parent) -> SearchDialog *;
  static auto usuario(QWidget *parent) -> SearchDialog *;
  static auto veiculo(QWidget *parent) -> SearchDialog *;
  static auto vendedor(QWidget *parent) -> SearchDialog *;

signals:
  void itemSelected(const QVariant &id);

private:
  // attributes
  const QString primaryKey;
  const QString fullTextIndex;
  const QStringList textKeys;
  bool permitirDescontinuados = false;
  bool silent = false;
  bool isRepresentacao = false;
  bool showAllProdutos = false;
  bool compraAvulsa = false;
  bool isSet = false;
  QString filter;
  QString fornecedorRep;
  SqlTableModel model;
  Ui::SearchDialog *ui;
  // methods
  explicit SearchDialog(const QString &title, const QString &table, const QString &primaryKey, const QStringList &textKeys, const QString &fullTextIndex, const QString &filter, QWidget *parent);
  auto hideColumns(const QStringList &columns) -> void;
  auto on_lineEditBusca_textChanged(const QString &) -> void;
  auto on_pushButtonSelecionar_clicked() -> void;
  auto on_radioButtonProdAtivos_toggled(const bool) -> void;
  auto on_radioButtonProdDesc_toggled(const bool) -> void;
  auto on_table_doubleClicked(const QModelIndex &) -> void;
  auto prepare_show() -> bool;
  auto sendUpdateMessage(const QModelIndex &index) -> void;
  auto setHeaderData(const QString &column, const QString &newHeader) -> void;
  auto setupTables(const QString &table) -> void;
};

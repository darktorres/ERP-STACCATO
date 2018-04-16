#ifndef SEARCHDIALOG_H
#define SEARCHDIALOG_H

#include "dialog.h"
#include "sqlrelationaltablemodel.h"

namespace Ui {
class SearchDialog;
}

class SearchDialog final : public Dialog {
  Q_OBJECT

public:
  explicit SearchDialog(const QString &title, const QString &table, const QStringList &indexes, const QString &filter, const bool permitirDescontinuados, QWidget *parent = nullptr);
  ~SearchDialog() final;
  auto getFilter() const -> QString;
  auto getText(const QVariant &value) -> QString;
  auto setFilter(const QString &value) -> void;
  auto setFornecedorRep(const QString &value) -> void;
  auto setRepresentacao(const bool isRepresentacao) -> void;
  auto show() -> void;
  auto showMaximized() -> void;

  // Factory Methods
  static auto cliente(QWidget *parent) -> SearchDialog *;
  static auto conta(QWidget *parent) -> SearchDialog *;
  static auto enderecoCliente(QWidget *parent) -> SearchDialog *;
  static auto fornecedor(QWidget *parent) -> SearchDialog *;
  static auto loja(QWidget *parent) -> SearchDialog *;
  static auto produto(const bool permitirDescontinuados, QWidget *parent) -> SearchDialog *;
  static auto profissional(QWidget *parent) -> SearchDialog *;
  static auto transportadora(QWidget *parent) -> SearchDialog *;
  static auto usuario(QWidget *parent) -> SearchDialog *;
  static auto veiculo(QWidget *parent) -> SearchDialog *;
  static auto vendedor(QWidget *parent) -> SearchDialog *;

signals:
  void itemSelected(const QVariant &value);

private:
  // attributes
  const QStringList indexes;
  const bool permitirDescontinuados;
  QString filter;
  QString fornecedorRep; // REFAC: verificar se isso não é a mesma coisa de 'representacao'
  QString primaryKey;
  QString representacao;
  QStringList textKeys;
  SqlRelationalTableModel model;
  Ui::SearchDialog *ui;
  // methods
  auto hideColumns(const QStringList &columns) -> void;
  auto on_lineEditBusca_textChanged(const QString &) -> void;
  auto on_pushButtonSelecionar_clicked() -> void;
  auto on_radioButtonProdAtivos_toggled(const bool) -> void;
  auto on_radioButtonProdDesc_toggled(const bool) -> void;
  auto on_table_doubleClicked(const QModelIndex &) -> void;
  auto on_table_entered(const QModelIndex &) -> void;
  auto sendUpdateMessage() -> void;
  auto setHeaderData(const QString &column, const QString &value) -> void;
  auto setPrimaryKey(const QString &value) -> void;
  auto setTextKeys(const QStringList &value) -> void;
  auto setupTables(const QString &table, const QString &filter) -> void;
};

#endif // SEARCHDIALOG_H

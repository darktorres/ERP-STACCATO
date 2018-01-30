#ifndef SEARCHDIALOG_H
#define SEARCHDIALOG_H

#include <QDataWidgetMapper>
#include <QDialog>

#include "sqlrelationaltablemodel.h"

namespace Ui {
class SearchDialog;
}

class SearchDialog final : public QDialog {
  Q_OBJECT

public:
  explicit SearchDialog(const QString &title, const QString &table, const QStringList &indexes, const QString &filter, const bool permitirDescontinuados, QWidget *parent = nullptr);
  ~SearchDialog() final;
  auto show() -> void;
  auto showMaximized() -> void;
  auto setFilter(const QString &value) -> void;
  auto getFilter() const -> QString;
  auto setRepresentacao(const QString &value) -> void;
  auto getText(const QVariant &value) -> QString;
  auto setFornecedorRep(const QString &value) -> void;

  // Factory Methods
  static SearchDialog *cliente(QWidget *parent);
  static SearchDialog *conta(QWidget *parent);
  static SearchDialog *enderecoCliente(QWidget *parent);
  static SearchDialog *fornecedor(QWidget *parent);
  static SearchDialog *loja(QWidget *parent);
  static SearchDialog *produto(const bool permitirDescontinuados, QWidget *parent);
  static SearchDialog *profissional(QWidget *parent);
  static SearchDialog *transportadora(QWidget *parent);
  static SearchDialog *usuario(QWidget *parent);
  static SearchDialog *veiculo(QWidget *parent);
  static SearchDialog *vendedor(QWidget *parent);

signals:
  void itemSelected(const QVariant &value);

private slots:
  void on_lineEditBusca_textChanged(const QString &);
  void on_pushButtonSelecionar_clicked();
  void on_radioButtonProdAtivos_toggled(const bool);
  void on_radioButtonProdDesc_toggled(const bool);
  void on_table_doubleClicked(const QModelIndex &);
  void on_table_entered(const QModelIndex &);

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
  auto sendUpdateMessage() -> void;
  auto setHeaderData(const QString &column, const QString &value) -> void;
  auto setPrimaryKey(const QString &value) -> void;
  auto setTextKeys(const QStringList &value) -> void;
  auto setupTables(const QString &table, const QString &filter) -> void;
};

#endif // SEARCHDIALOG_H

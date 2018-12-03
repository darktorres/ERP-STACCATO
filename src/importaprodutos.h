#ifndef IMPORTAPRODUTOS_H
#define IMPORTAPRODUTOS_H

#include <QProgressDialog>

#include "sqlrelationaltablemodel.h"

namespace Ui {
class ImportaProdutos;
}

class ImportaProdutos final : public QDialog {
  Q_OBJECT

public:
  enum class Tipo { Produto = 0, Estoque = 1, Promocao = 2 };
  explicit ImportaProdutos(const Tipo tipo, QWidget *parent = nullptr);
  ~ImportaProdutos();
  auto importarTabela() -> void;

private:
  enum class FieldColors {
    White = 0,  // no change
    Green = 1,  // new value
    Yellow = 2, // value changed
    Gray = 3,   // wrong value but accepted
    Red = 4     // wrong value, must be fixed
  };

  // attributes
  int currentRow = 0;
  int itensError = 0;
  int itensExpired = 0;
  int itensImported = 0;
  int itensNotChanged = 0;
  int itensUpdated = 0;
  int validade;
  QHash<int, bool> hashAtualizado;
  QHash<QString, int> hash;
  QMap<QString, int> fornecedores;
  QProgressDialog *progressDialog;
  QSqlDatabase db;
  QString file;
  QString fornecedor;
  QStringList idsFornecedor;
  QVariantMap variantMap;
  SqlRelationalTableModel modelProduto;
  SqlRelationalTableModel modelErro;
  // REFAC: a tabela no BD nao usa mais uma unica coluna, nao é mais (1,2,3) e sim 3 colunas separadas
  const Tipo tipo;
  Ui::ImportaProdutos *ui;
  // methods
  auto atualizaCamposProduto() -> bool;
  auto atualizaProduto() -> bool;
  auto buscarCadastrarFornecedor(const QString &fornecedor) -> std::optional<int>;
  auto cadastraFornecedores() -> bool;
  auto cadastraProduto() -> bool;
  auto camposForaDoPadrao() -> bool;
  auto closeEvent(QCloseEvent *event) -> void final;
  auto consistenciaDados() -> void;
  auto contaProdutos() -> void;
  auto expiraPrecosAntigos() -> bool;
  auto importar() -> bool;
  auto insereEmErro() -> bool;
  auto insereEmOk() -> bool;
  auto leituraProduto(const QSqlQuery &query, const QSqlRecord &record) -> void;
  auto marcaProdutoNaoDescontinuado() -> bool;
  auto marcaTodosProdutosDescontinuados() -> bool;
  auto mostraApenasEstesFornecedores() -> void;
  auto on_checkBoxRepresentacao_toggled(const bool checked) -> void;
  auto on_pushButtonSalvar_clicked() -> void;
  auto pintarCamposForaDoPadrao(const int row) -> bool;
  auto readFile() -> bool;
  auto readValidade() -> bool;
  auto salvar() -> bool;
  auto setProgressDialog() -> void;
  auto setVariantMap() -> void;
  auto setupTables() -> void;
  auto verificaSeProdutoJaCadastrado() -> bool;
  auto verificaSeRepresentacao() -> bool;
  auto verificaTabela(const QSqlRecord &record) -> bool;
};

#endif // IMPORTAPRODUTOS_H

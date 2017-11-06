#ifndef IMPORTAPRODUTOS_H
#define IMPORTAPRODUTOS_H

#include <QDialog>
#include <QProgressDialog>

#include "sqlrelationaltablemodel.h"

namespace Ui {
class ImportaProdutos;
}

class ImportaProdutos : public QDialog {
  Q_OBJECT

public:
  explicit ImportaProdutos(QWidget *parent = 0);
  ~ImportaProdutos();
  void importarProduto();
  void importarEstoque();
  void importarPromocao();

signals:
  void errorSignal(const QString &error);
  void transactionEnded();
  void transactionStarted();

private slots:
  void on_checkBoxRepresentacao_toggled(const bool checked);
  void on_pushButtonSalvar_clicked();
  void on_tableErro_entered(const QModelIndex &);
  void on_tableProdutos_entered(const QModelIndex &);
  void on_tabWidget_currentChanged(const int index);

private:
  enum class Tipo { Produto = 0, Estoque = 1, Promocao = 2 };

  enum class FieldColors {
    White = 0,  // no change
    Green = 1,  // new value
    Yellow = 2, // value changed
    Gray = 3,   // wrong value but accepted
    Red = 4     // wrong value, must be fixed
  };

  // attributes
  bool hasError = false;
  int i = 0;
  int itensError = 0;
  int itensExpired = 0;
  int itensImported = 0;
  int itensNotChanged = 0;
  int itensUpdated = 0;
  int row = 0;
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
  SqlRelationalTableModel model;
  SqlRelationalTableModel modelErro;
  // REFAC: a tabela no BD nao usa mais uma unica coluna, nao é mais (1,2,3) e sim 3 colunas separadas
  Tipo tipo;
  Ui::ImportaProdutos *ui;
  // methods
  bool atualizaCamposProduto();
  bool atualizaProduto();
  bool buscarCadastrarFornecedor(const QString &fornecedor, int &id);
  bool cadastraFornecedores();
  bool cadastraProduto();
  bool camposForaDoPadrao();
  bool expiraPrecosAntigos();
  bool guardaNovoPrecoValidade();
  bool importar();
  bool insereEmErro();
  bool insereEmOk();
  bool marcaProdutoNaoDescontinuado();
  bool marcaTodosProdutosDescontinuados();
  bool pintarCamposForaDoPadrao(const int row);
  bool readFile();
  bool readValidade();
  bool verificaSeProdutoJaCadastrado();
  bool verificaSeRepresentacao();
  bool verificaTabela(const QSqlRecord &record);
  virtual void closeEvent(QCloseEvent *event) override;
  void consistenciaDados();
  void contaProdutos();
  void importarTabela();
  void leituraProduto(const QSqlQuery &query, const QSqlRecord &record);
  void mostraApenasEstesFornecedores();
  void salvar();
  void setProgressDialog();
  void setupTables();
  void setVariantMap();
};

#endif // IMPORTAPRODUTOS_H

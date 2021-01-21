#pragma once

#include "sqltablemodel.h"
#include "xlsxdocument.h"

#include <QDialog>
#include <QProgressDialog>

namespace Ui {
class ImportaProdutos;
}

class ImportaProdutos final : public QDialog {
  Q_OBJECT

  struct Produto {
    int idFornecedor;
    QString fornecedor;
    QString descricao;
    QString un;
    QString colecao;
    double m2cx;
    double pccx;
    double kgcx;
    QString formComercial;
    QString codComercial;
    QString codBarras;
    QString ncm;
    QString ncmEx;
    double qtdPallet;
    double custo;
    double precoVenda;
    QString ui;
    QString un2;
    double minimo;
    double mva;
    double st;
    double sticms;
    double quantCaixa;
    double markup;
  };

public:
  enum class Tipo { Normal = 0, Promocao = 1 };

  explicit ImportaProdutos(const Tipo tipo, QWidget *parent);
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
  int itensError = 0;
  int itensExpired = 0;
  int itensImported = 0;
  int itensNotChanged = 0;
  int itensUpdated = 0;
  int validade;
  Produto produto;
  QHash<QString, int> hashModel;
  QMap<QString, int> fornecedores;
  QProgressDialog *progressDialog;
  QString file;
  QString fornecedor;
  QString idsFornecedor;
  QString validadeString;
  QVector<int> vectorProdutosImportados;
  SqlTableModel modelErro;
  SqlTableModel modelEstoque;
  SqlTableModel modelProduto;
  Tipo const tipo;
  Ui::ImportaProdutos *ui;
  // methods
  auto atualizaCamposProduto(const int row) -> void;
  auto atualizaProduto() -> void;
  auto buscarCadastrarFornecedor() -> int;
  auto cadastraFornecedores(QXlsx::Document &xlsx) -> void;
  auto camposForaDoPadrao() -> bool;
  auto closeEvent(QCloseEvent *event) -> void final;
  auto importar() -> void;
  auto insereEmErro() -> void;
  auto insereEmOk() -> void;
  auto leituraProduto(QXlsx::Document &xlsx, const int row) -> void;
  auto marcaProdutoNaoDescontinuado(const int row) -> void;
  auto marcaTodosProdutosDescontinuados() -> void;
  auto mostraApenasEstesFornecedores() -> void;
  auto on_checkBoxRepresentacao_toggled(const bool checked) -> void;
  auto on_pushButtonSalvar_clicked() -> void;
  auto pintarCamposForaDoPadrao(const int row) -> void;
  auto readFile() -> bool;
  auto readValidade() -> bool;
  auto salvar() -> void;
  auto setConnections() -> void;
  auto setProgressDialog() -> void;
  auto setupModels() -> void;
  auto setupTables() -> void;
  auto verificaSeRepresentacao() -> void;
  auto verificaTabela(QXlsx::Document &xlsx) -> void;
};

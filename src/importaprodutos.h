#pragma once

#include "sqltablemodel.h"
#include "xlsxdocument.h"

#include <QDate>
#include <QDialog>
#include <QProgressDialog>
#include <QStandardItemModel>

namespace Ui {
class ImportaProdutos;
}

class ImportaProdutos final : public QDialog {
  Q_OBJECT

private:
  // Product data parsed from Excel
  // Note: QVariant fields preserve NULL from database (empty QVariant = NULL)
  struct Produto {
    int idProduto = 0;  // 0 for new products
    int idFornecedor = 0;
    QString fornecedor;
    QString descricao;
    QString un;
    QString colecao;
    double m2cx = 0;
    double pccx = 0;
    double kgcx = 0;
    QString formComercial;
    QString codComercial;
    QString codBarras;
    QString ncm;
    QVariant qtdPallet;   // Nullable
    double custo = 0;
    double precoVenda = 0;
    QString ui;
    QString un2;
    QVariant minimo;      // Nullable
    QVariant mva;         // Nullable
    QVariant st;          // Nullable
    QVariant sticms;      // Nullable
    double quantCaixa = 0;
    double markup = 0;
    QDate validade;
  };

  // Field-level change status for coloring
  enum class FieldStatus { NoChange = 0, New = 1, Changed = 2, OutOfSpec = 3, Invalid = 4 };

  // Change record for a product
  struct ProductChange {
    enum class Type { New, Updated, NotChanged, Discontinued, Error };

    Type type = Type::NotChanged;
    Produto newData;                           // Data from Excel (or DB for discontinued)
    Produto oldData;                           // Original DB data (for comparison display)
    QHash<QString, FieldStatus> fieldStatus;   // Per-field coloring
  };

public:
  enum class Tipo { Normal = 0, Promocao = 1 };
  Q_ENUM(Tipo)

  enum class FieldColors {
    White = 0,  // no change
    Green = 1,  // new value
    Yellow = 2, // value changed
    Gray = 3,   // wrong value but accepted
    Red = 4     // wrong value, must be fixed
  };
  Q_ENUM(FieldColors)

  explicit ImportaProdutos(const Tipo tipo, QWidget *parent);
  ~ImportaProdutos();

  auto importarTabela() -> void;
  auto importarTabelaCLI(const QString &filePath, int validadeDias) -> void;

#ifdef BENCHMARK_BUILD
  // Test accessors for regression testing
  auto getItensImported() const -> int { return itensImported; }
  auto getItensUpdated() const -> int { return itensUpdated; }
  auto getItensNotChanged() const -> int { return itensNotChanged; }
  auto getItensExpired() const -> int { return itensExpired; }
  auto getItensError() const -> int { return itensError; }

  // Legacy model accessors (kept for compatibility, return dummy models)
  auto getModelProduto() -> SqlTableModel & { return modelProduto; }
  auto getModelErro() -> SqlTableModel & { return modelErro; }

  // New optimized accessors for regression testing
  auto getPreviewModel() -> QStandardItemModel & { return m_previewModel; }
  auto getErrorModel() -> QStandardItemModel & { return m_errorModel; }
  auto getProductChanges() const -> const QVector<ProductChange> & { return m_productChanges; }
  auto getErrorChanges() const -> const QVector<ProductChange> & { return m_errorChanges; }
  auto getExistingProducts() const -> const QHash<QString, Produto> & { return m_existingProducts; }
  static auto getPreviewColumns() -> const QStringList & { return PREVIEW_COLUMNS; }
#endif

private:
  // attributes - counters
  int itensError = 0;
  int itensExpired = 0;
  int itensImported = 0;
  int itensNotChanged = 0;
  int itensUpdated = 0;
  int validade = 0;

  // attributes - data
  QMap<QString, int> m_fornecedores;
  QProgressDialog progressDialog;
  QString file;
  QString m_fornecedor;
  QString idsFornecedor;
  QString validadeString;
  Tipo const tipo;
  Ui::ImportaProdutos *ui;

  // New optimized architecture - in-memory processing
  QVector<ProductChange> m_productChanges;      // All changes for save
  QVector<ProductChange> m_errorChanges;        // Error products
  QHash<QString, Produto> m_existingProducts;   // Loaded from DB (unique keys for lookup)
  QVector<Produto> m_allExistingProducts;       // ALL products from DB (including duplicates)
  QStandardItemModel m_previewModel;            // For preview display
  QStandardItemModel m_errorModel;              // For error display

  // Column indices for preview model
  static const QStringList PREVIEW_COLUMNS;

  // methods - core flow
  auto processarArquivo() -> void;
  auto processarArquivoOptimized() -> void;
  auto readFile() -> bool;
  auto readValidade() -> bool;
  auto closeEvent(QCloseEvent *event) -> void final;

  // methods - data loading
  auto loadExistingProducts() -> void;
  auto buscarCadastrarFornecedor() -> int;
  auto cadastraFornecedores(QXlsx::Document &xlsx) -> void;
  auto verificaSeRepresentacao() -> void;
  auto verificaTabela(QXlsx::Document &xlsx) -> void;

  // methods - Excel processing
  auto parseExcelRow(QXlsx::Document &xlsx, int row) -> Produto;
  auto makeProductKey(const Produto &p) const -> QString;
  auto camposForaDoPadrao(const Produto &p) const -> bool;
  auto compareProducts(const Produto &existing, const Produto &imported) -> ProductChange;
  auto makeNewProductChange(const Produto &p) -> ProductChange;
  auto makeDiscontinuedChange(const Produto &p) -> ProductChange;
  auto makeErrorChange(const Produto &p, const QString &reason) -> ProductChange;

  // methods - preview model
  auto buildPreviewModels() -> void;
  auto addRowToModel(QStandardItemModel &model, const ProductChange &change) -> void;
  auto addRowToModelFast(QStandardItemModel &model, int row, const ProductChange &change,
                         const QBrush &textColor, const QBrush &cyanBrush) -> void;
  auto getFieldColor(FieldStatus status) const -> QBrush;
  auto setupTables() -> void;
  auto setConnections() -> void;
  auto setProgressDialog() -> void;

  // methods - save
  auto salvar() -> void;
  auto salvarOptimized() -> void;
  auto atualizaPrecoEstoque() -> void;
  auto marcaTodosProdutosDescontinuados() -> void;

  // methods - UI
  auto on_checkBoxRepresentacao_toggled(const bool checked) -> void;
  auto on_pushButtonSalvar_clicked() -> void;

  // Legacy methods (kept for compatibility, will be removed)
  Produto produto;
  QHash<QString, int> hashModel;
  QHash<QString, int> fieldIdx;
  QVector<int> vectorProdutosImportados;
  SqlTableModel modelErro;
  SqlTableModel modelProduto;
  auto atualizaCamposProduto(const int row) -> void;
  auto cacheFieldIndices() -> void;
  auto atualizaProduto() -> void;
  auto camposForaDoPadrao() -> bool;  // Legacy parameterless version
  auto insereEmErro() -> void;
  auto insereEmOk() -> void;
  auto leituraProduto(QXlsx::Document &xlsx, const int row) -> void;
  auto marcaProdutoNaoDescontinuado(const int row) -> void;
  auto mostraApenasEstesFornecedores() -> void;
  auto pintarCamposForaDoPadrao(const int row) -> void;
  auto setupModels() -> void;
};

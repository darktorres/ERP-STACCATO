#pragma once

#include <QAbstractTableModel>
#include <QHash>
#include <QVariant>
#include <QVector>
#include <functional>

// Model de janela deslizante (sliding window): carrega paginas de ~1000 linhas por vez, via
// paginacao por keyset (cursor = valor da coluna de ordenacao + idNFe como desempate, NULL-safe -
// ver src/sqlpaginatedmodel.cpp), descartando o pedaco mais antigo quando ultrapassa ~3000 linhas
// carregadas. Ao contrario de SqlQueryModel/SqlTableModel, nao envolve QSqlQueryModel (que nao da
// suporte a inserir/remover linhas arbitrariamente) - por isso TableView cai no fallback generico
// de columnIndex() baseado em headerData() para esse model (nao em record()).
//
// Quem monta o SQL de cada pagina e quem chama (nao a classe): ela só pede, via
// QueryBuilderFactory, "me dê o construtor de página para esta coluna/ordem", e usa o construtor
// devolvido para pedir cada página (1a, seguinte, anterior).
class SqlPaginatedModel final : public QAbstractTableModel {
  Q_OBJECT

public:
  enum class Direction { First, Next, Previous };

  // Pedido de uma pagina: cursor (invalido = 1a pagina) + direcao.
  struct PageRequest {
    QVariant cursorValue;
    QVariant cursorId;
    Direction direction = Direction::First;
  };

  // Monta o SQL completo de uma pagina dado o pedido. Implementado pelo widget (reaproveita a
  // logica de filtros/joins que ele ja tem).
  using PageQueryBuilder = std::function<QString(const PageRequest &request)>;

  // Dado o nome da coluna de ordenacao (um de fieldNames) e a ordem, devolve o construtor de
  // pagina pra essa combinacao especifica (permite ao widget mudar quais tabelas o JOIN de corte
  // precisa conforme a coluna escolhida - ex.: Saida so junta cliente/venda quando ordena por
  // Cliente/CPF-CNPJ).
  using QueryBuilderFactory = std::function<PageQueryBuilder(const QString &sortColumn, Qt::SortOrder order)>;

  explicit SqlPaginatedModel(QObject *parent = nullptr);

  // Monta o fragmento WHERE (NULL-safe) pra continuar a paginacao por keyset depois do cursor,
  // na direcao pedida. valueExpr/idExpr: expressoes SQL da coluna de ordenacao e do desempate
  // (ex.: "n.dataHoraEmissao", "n.idNFe"). cursorValue/cursorId invalidos = sem cursor (1a
  // pagina) - devolve "1" (sempre verdadeiro, sem filtro de continuacao). Ordena sempre com nulos
  // por ultimo, independente de order ser ASC ou DESC. Uso: o widget monta o SQL da pagina
  // encaixando isso no WHERE, junto dos outros filtros (status/busca/etc.).
  static auto buildKeysetWhere(const QString &valueExpr, const QString &idExpr, const QVariant &cursorValue, const QVariant &cursorId, Qt::SortOrder order, bool forward) -> QString;

  // Monta a expressao ORDER BY correspondente (mesma convencao de nulos por ultimo).
  static auto buildOrderBy(const QString &valueExpr, const QString &idExpr, Qt::SortOrder order, bool forward) -> QString;

  // Converte um QVariant num literal SQL seguro (strings escapadas via qApp->sanitizeSQL, NULL,
  // datas/números formatados). Usado internamente e pelo widget ao montar os fragmentos acima.
  static auto toSqlLiteral(const QVariant &value) -> QString;

  // Reinicia a paginacao do zero (mudanca de filtro e/ou ordenacao). fieldNames: nomes das
  // colunas na ordem do SELECT de cada pagina (usado por data(row,QString) e headerData()).
  auto reset(const QStringList &fieldNames, const QString &sortColumn, Qt::SortOrder order, const QueryBuilderFactory &factory) -> void;

  auto setHeaderLabel(const QString &fieldName, const QString &label) -> void;

  auto sortColumn() const -> QString;
  auto sortOrder() const -> Qt::SortOrder;
  auto hasMoreAfter() const -> bool;

  // Chamado pelo widget quando a rolagem chega perto do fim/inicio da janela carregada.
  auto tryLoadNext() -> void;
  auto tryLoadPrevious() -> void;

  // Acessores no estilo dos outros models do projeto (SqlQueryModel/SqlTableModel).
  auto data(const int row, const QString &column) const -> QVariant;
  auto data(const int row, const int column) const -> QVariant;
  auto fieldIndex(const QString &fieldName, const bool silent = false) const -> int;

  // QAbstractTableModel
  auto rowCount(const QModelIndex &parent = QModelIndex()) const -> int override;
  auto columnCount(const QModelIndex &parent = QModelIndex()) const -> int override;
  auto data(const QModelIndex &index, int role) const -> QVariant override;
  auto headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const -> QVariant override;
  auto sort(int column, Qt::SortOrder order = Qt::AscendingOrder) -> void override;

signals:
  void moreAvailableChanged(bool moreAvailable);

private:
  static constexpr int PAGE_SIZE = 1000;
  static constexpr int MAX_ROWS = 3 * PAGE_SIZE;

  auto loadFirstPage() -> void;
  auto runQuery(const QString &sql) -> QVector<QVector<QVariant>>;
  auto updateEdgeKeys() -> void;
  auto emitMoreAvailableIfChanged() -> void;

  QStringList fieldNames_;
  QHash<QString, QString> headerLabels_;
  QString sortColumn_;
  Qt::SortOrder sortOrder_ = Qt::DescendingOrder;
  QueryBuilderFactory factory_;
  PageQueryBuilder builder_;

  QVector<QVector<QVariant>> rows_;
  QVariant firstKeyValue_;
  QVariant firstKeyId_;
  QVariant lastKeyValue_;
  QVariant lastKeyId_;
  bool hasMoreAfter_ = false;
  bool hasMoreBefore_ = false;
  bool lastMoreAvailable_ = false;
};

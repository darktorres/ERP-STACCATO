#pragma once

#include <QAbstractTableModel>
#include <QHash>
#include <QVariant>
#include <QVector>
#include <functional>

// Model de janela deslizante (sliding window): carrega paginas de ~1000 linhas por vez, via
// paginacao por keyset (cursor = valores das colunas de ordenacao/desempate, NULL-safe - ver
// src/sqlpaginatedmodel.cpp), descartando o pedaco mais antigo quando ultrapassa ~3000 linhas
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

  // Uma chave de ordenacao/keyset: expressao SQL + direcao explicita. Permite misturar ASC/DESC
  // entre colunas de um desempate composto (ex.: Financeiro Receber ordena parcela DESC dentro de
  // colunas ASC).
  struct KeyExpr {
    QString expr;
    Qt::SortOrder order = Qt::AscendingOrder;
  };

  // Pedido de uma pagina: cursor (vazio = 1a pagina) + direcao. cursorValues tem uma entrada por
  // chave de keyset, na mesma ordem/tamanho sempre usada pelo model: {coluna de ordenacao} +
  // extraKeyFieldNames (de reset()) + {idFieldName}.
  struct PageRequest {
    QVector<QVariant> cursorValues;
    Direction direction = Direction::First;
  };

  // SQL de uma pagina: displaySql e a query completa de exibicao (joins + busca aplicada, como
  // antes). rawPeekSql e a subquery de corte "crua" sozinha (sem busca, sem os joins de exibicao),
  // com o SELECT estendido para trazer as colunas de keyset (coluna de ordenacao + extraKeys +
  // idField, nessa ordem - mesma ordem de keyFieldNames()) em vez de so o id. Normalmente e o
  // "capSql" que o widget ja monta internamente, devolvido tal e qual. Usado pelo model pra
  // continuar avançando o cursor quando a busca filtra uma pagina inteira (ver tryLoadNext()).
  struct PageSql {
    QString displaySql;
    QString rawPeekSql;
  };

  // Monta o SQL completo de uma pagina dado o pedido. Implementado pelo widget (reaproveita a
  // logica de filtros/joins que ele ja tem).
  using PageQueryBuilder = std::function<PageSql(const PageRequest &request)>;

  // Dado o nome da coluna de ordenacao (um de fieldNames) e a ordem, devolve o construtor de
  // pagina pra essa combinacao especifica (permite ao widget mudar quais tabelas o JOIN de corte
  // precisa conforme a coluna escolhida - ex.: Saida so junta cliente/venda quando ordena por
  // Cliente/CPF-CNPJ).
  using QueryBuilderFactory = std::function<PageQueryBuilder(const QString &sortColumn, Qt::SortOrder order)>;

  explicit SqlPaginatedModel(QObject *parent = nullptr);

  // Monta o fragmento WHERE (NULL-safe) pra continuar a paginacao por keyset depois do cursor, na
  // direcao pedida, para um desempate composto de N chaves (ex.: {dataPagamento, idVenda, tipo,
  // parcela, idPagamento}). keys: chaves em ordem de prioridade, cada uma com sua propria direcao
  // (ex.: {"n.dataHoraEmissao", Asc}). A ULTIMA chave e tratada como NOT NULL (sem branch de nulo)
  // - deve ser sempre o id/tiebreaker unico da tabela (ex.: idNFe/idPagamento), nunca uma coluna
  // anulavel. Todas as chaves anteriores sao tratadas como anulaveis (nulos sempre por ultimo,
  // independente de order ser ASC ou DESC), mesmo que na pratica nunca sejam NULL - inofensivo.
  // cursorValues vazio ou de tamanho diferente de keys = sem cursor (1a pagina) - devolve "1"
  // (sempre verdadeiro, sem filtro de continuacao). Uso: o widget monta o SQL da pagina encaixando
  // isso no WHERE, junto dos outros filtros (status/busca/etc.).
  static auto buildKeysetWhere(const QVector<KeyExpr> &keys, const QVector<QVariant> &cursorValues, bool forward) -> QString;

  // Monta a expressao ORDER BY correspondente (mesma convencao de nulos por ultimo, exceto a
  // ultima chave, assumida NOT NULL).
  static auto buildOrderBy(const QVector<KeyExpr> &keys, bool forward) -> QString;

  // Converte um QVariant num literal SQL seguro (strings escapadas via qApp->sanitizeSQL, NULL,
  // datas/números formatados). Usado internamente e pelo widget ao montar os fragmentos acima.
  static auto toSqlLiteral(const QVariant &value) -> QString;

  // Reinicia a paginacao do zero (mudanca de filtro e/ou ordenacao). fieldNames: nomes das
  // colunas na ordem do SELECT de cada pagina (usado por data(row,QString) e headerData()).
  // idFieldName: campo (dentre fieldNames) usado como desempate final/unico da paginacao por
  // keyset - deve ser NOT NULL (ex.: "idNFe", "idPagamento"). extraKeyFieldNames: desempates fixos
  // adicionais aplicados entre a coluna de ordenacao (que pode mudar por clique de cabecalho, via
  // sort()) e idFieldName - vazio na maioria dos casos; usado quando a ordem padrão da tela
  // precisa preservar um desempate composto (ex.: Financeiro Receber: idVenda, tipo, parcela).
  auto reset(const QStringList &fieldNames, const QString &idFieldName, const QString &sortColumn, Qt::SortOrder order, const QStringList &extraKeyFieldNames, const QueryBuilderFactory &factory) -> void;

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
  // Quando a busca filtra uma janela inteira, o model "espia" (rawPeekSql, sem busca/joins) e
  // avanca o cursor pra tentar a proxima janela, repetindo ate achar linhas ou esgotar o cru. Limite
  // por chamada de tryLoadNext/tryLoadPrevious (rolagem) - se estourar, o cursor fica avancado e a
  // proxima rolagem continua dali (nao reinicia). Bem maior em loadFirstPage() pois lá, se parar sem
  // achar nada, a tabela fica sem linha nenhuma (sem scrollbar) e o usuario nao tem como pedir mais.
  static constexpr int MAX_PEEK_LOOPS_SCROLL = 50;
  static constexpr int MAX_PEEK_LOOPS_FIRST = 10000;

  auto loadFirstPage() -> void;
  auto runQuery(const QString &sql) -> QVector<QVector<QVariant>>;
  auto keyFieldNames() const -> QStringList; // {sortColumn_} + extraKeyFieldNames_ + {idFieldName_}
  auto edgeKeyValues(const QVector<QVariant> &row) const -> QVector<QVariant>;
  auto updateEdgeKeys() -> void;
  auto emitMoreAvailableIfChanged() -> void;

  QStringList fieldNames_;
  QHash<QString, QString> headerLabels_;
  QString idFieldName_;
  QStringList extraKeyFieldNames_;
  QString sortColumn_;
  Qt::SortOrder sortOrder_ = Qt::DescendingOrder;
  QueryBuilderFactory factory_;
  PageQueryBuilder builder_;

  QVector<QVector<QVariant>> rows_;
  QVector<QVariant> firstKeyValues_;
  QVector<QVariant> lastKeyValues_;
  bool hasMoreAfter_ = false;
  bool hasMoreBefore_ = false;
  bool lastMoreAvailable_ = false;
};

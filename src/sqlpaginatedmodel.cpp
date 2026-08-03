#include "sqlpaginatedmodel.h"

#include "application.h"
#include "sqlquery.h"

#include <QFont>
#include <QSqlError>

#include <algorithm>

namespace {

// "Antes do cursor ou depois dele" (NULL-safe, nulos sempre por ultimo) para uma UNICA chave
// anulavel - branches 1+2 do esquema original de 3 branches (a 3a branch, "empatou, olha a
// proxima chave", vem da composicao em buildKeysetWhere, nao daqui).
QString lessGreaterNullable(const QString &expr, const QVariant &cursorValue, const Qt::SortOrder order, const bool forward) {
  const bool descending = (order == Qt::DescendingOrder);
  const bool wantLess = (descending == forward);
  const QString valueOp = wantLess ? "<" : ">";
  const QString isNullOp = forward ? ">" : "<"; // nulos sempre por ultimo: "continuar pra frente" = isNull crescente

  const QString cursorValueLiteral = SqlPaginatedModel::toSqlLiteral(cursorValue);
  const QString cursorIsNullLiteral = (cursorValue.isValid() and not cursorValue.isNull()) ? "0" : "1";

  return "((" + expr + " IS NULL) " + isNullOp + " " + cursorIsNullLiteral + ")" + " OR ((" + expr + " IS NULL) <=> " + cursorIsNullLiteral + " AND NOT (" + expr + " <=> " + cursorValueLiteral +
         ") AND (" + expr + " " + valueOp + " " + cursorValueLiteral + " OR " + cursorValueLiteral + " IS NULL))";
}

// Mesma comparacao, mas para a chave final (assumida NOT NULL, ex.: idNFe/idPagamento) - sem
// necessidade de tratamento de nulo.
QString lessGreaterNotNull(const QString &expr, const QVariant &cursorValue, const Qt::SortOrder order, const bool forward) {
  const bool descending = (order == Qt::DescendingOrder);
  const bool wantLess = (descending == forward);
  const QString valueOp = wantLess ? "<" : ">";

  return expr + " " + valueOp + " " + SqlPaginatedModel::toSqlLiteral(cursorValue);
}

// "Empatou com o cursor nesta chave" - NULL-safe via <=> (funciona tanto pra valor real quanto NULL).
QString equalClause(const QString &expr, const QVariant &cursorValue) { return expr + " <=> " + SqlPaginatedModel::toSqlLiteral(cursorValue); }

} // namespace

SqlPaginatedModel::SqlPaginatedModel(QObject *parent) : QAbstractTableModel(parent) {}

void SqlPaginatedModel::reset(const QStringList &fieldNames, const QString &idFieldName, const QString &sortColumn, const Qt::SortOrder order, const QStringList &extraKeyFieldNames,
                               const QueryBuilderFactory &factory) {
  fieldNames_ = fieldNames;
  idFieldName_ = idFieldName;
  extraKeyFieldNames_ = extraKeyFieldNames;
  sortColumn_ = sortColumn;
  sortOrder_ = order;
  factory_ = factory;
  builder_ = factory_(sortColumn_, sortOrder_);

  loadFirstPage();
}

void SqlPaginatedModel::setHeaderLabel(const QString &fieldName, const QString &label) { headerLabels_.insert(fieldName, label); }

QString SqlPaginatedModel::sortColumn() const { return sortColumn_; }

Qt::SortOrder SqlPaginatedModel::sortOrder() const { return sortOrder_; }

bool SqlPaginatedModel::hasMoreAfter() const { return hasMoreAfter_; }

void SqlPaginatedModel::loadFirstPage() {
  QVector<QVariant> cursor; // vazio = sem cursor (1a tentativa)
  Direction direction = Direction::First;
  QVector<QVector<QVariant>> displayRows;
  int peeks = 0;

  // Se a busca filtrar a 1a janela inteira, espia (sem busca/joins) e avanca o cursor pra tentar a
  // proxima - repete ate achar linhas ou esgotar o corte cru. Sem isso a tabela ficaria vazia (e sem
  // scrollbar, ou seja sem chance do usuario "pedir mais") sempre que o termo buscado nao estiver na
  // 1a janela de ~1000 linhas (ex.: filtro "Todos"/sem data, termo real mas antigo/recente demais).
  while (true) {
    PageRequest request;
    request.cursorValues = cursor;
    request.direction = direction;

    const PageSql sql = builder_(request);

    displayRows = runQuery(sql.displaySql);

    if (not displayRows.isEmpty()) { break; }
    if (++peeks >= MAX_PEEK_LOOPS_FIRST) { break; } // salvaguarda - nao deveria disparar na pratica

    const auto peekRows = runQuery(sql.rawPeekSql);

    if (peekRows.size() < PAGE_SIZE) { break; } // corte cru esgotou - fim genuino do historico

    cursor = peekRows.last();
    direction = Direction::Next;
  }

  beginResetModel();

  rows_ = displayRows;
  hasMoreBefore_ = false;

  endResetModel();

  updateEdgeKeys();

  hasMoreAfter_ = rows_.size() >= PAGE_SIZE;

  emitMoreAvailableIfChanged();
}

void SqlPaginatedModel::tryLoadNext() {
  if (not hasMoreAfter_ or lastKeyValues_.isEmpty()) { return; }

  QVector<QVariant> cursor = lastKeyValues_;
  QVector<QVector<QVariant>> newRows;
  int peeks = 0;
  bool exhausted = false;

  while (true) {
    PageRequest request;
    request.cursorValues = cursor;
    request.direction = Direction::Next;

    const PageSql sql = builder_(request);

    newRows = runQuery(sql.displaySql);

    if (not newRows.isEmpty()) { break; }

    ++peeks;

    const auto peekRows = runQuery(sql.rawPeekSql);

    if (peekRows.size() < PAGE_SIZE) {
      exhausted = true;
      if (not peekRows.isEmpty()) { cursor = peekRows.last(); }
      break;
    }

    cursor = peekRows.last();

    if (peeks >= MAX_PEEK_LOOPS_SCROLL) { break; } // limite desta rolagem - a proxima continua do cursor avancado
  }

  if (newRows.isEmpty()) {
    lastKeyValues_ = cursor; // avanca mesmo sem linhas novas, p/ a proxima chamada continuar dali
    hasMoreAfter_ = not exhausted;
    emitMoreAvailableIfChanged();
    return;
  }

  const int insertFirst = rows_.size();

  beginInsertRows(QModelIndex(), insertFirst, insertFirst + newRows.size() - 1);
  rows_ += newRows;
  endInsertRows();

  hasMoreAfter_ = newRows.size() >= PAGE_SIZE;
  hasMoreBefore_ = true; // ja existe pelo menos uma pagina carregada antes do inicio atual

  updateEdgeKeys();

  // só descarta quando ha folga alem do limite, pra nao remover linhas perto da area visivel logo apos inserir
  if (rows_.size() > MAX_ROWS) {
    const int removeCount = rows_.size() - MAX_ROWS;

    beginRemoveRows(QModelIndex(), 0, removeCount - 1);
    rows_.remove(0, removeCount);
    endRemoveRows();

    updateEdgeKeys();
  }

  emitMoreAvailableIfChanged();
}

void SqlPaginatedModel::tryLoadPrevious() {
  if (not hasMoreBefore_ or firstKeyValues_.isEmpty()) { return; }

  QVector<QVariant> cursor = firstKeyValues_;
  QVector<QVector<QVariant>> newRows;
  int peeks = 0;
  bool exhausted = false;

  while (true) {
    PageRequest request;
    request.cursorValues = cursor;
    request.direction = Direction::Previous;

    const PageSql sql = builder_(request);

    newRows = runQuery(sql.displaySql); // ja vem na ordem de exibicao (exibicaoOrderBy e sempre forward=true no widget)

    if (not newRows.isEmpty()) { break; }

    ++peeks;

    const auto peekRows = runQuery(sql.rawPeekSql);

    if (peekRows.size() < PAGE_SIZE) {
      exhausted = true;
      if (not peekRows.isEmpty()) { cursor = peekRows.last(); }
      break;
    }

    // capSql (rawPeekSql) sempre ordena do mais perto do cursor pro mais longe, tanto pra Next
    // quanto pra Previous (buildOrderBy com forward=false so inverte a direcao efetiva, nao troca
    // "perto/longe" de lado) - por isso .last() continua correto aqui tambem.
    cursor = peekRows.last();

    if (peeks >= MAX_PEEK_LOOPS_SCROLL) { break; } // limite desta rolagem - a proxima continua do cursor avancado
  }

  if (newRows.isEmpty()) {
    firstKeyValues_ = cursor; // avanca mesmo sem linhas novas, p/ a proxima chamada continuar dali
    hasMoreBefore_ = not exhausted;
    return;
  }

  beginInsertRows(QModelIndex(), 0, newRows.size() - 1);
  rows_ = newRows + rows_;
  endInsertRows();

  hasMoreBefore_ = newRows.size() >= PAGE_SIZE;

  updateEdgeKeys();

  if (rows_.size() > MAX_ROWS) {
    const int removeCount = rows_.size() - MAX_ROWS;
    const int start = rows_.size() - removeCount;

    beginRemoveRows(QModelIndex(), start, rows_.size() - 1);
    rows_.remove(start, removeCount);
    endRemoveRows();

    hasMoreAfter_ = true;
    updateEdgeKeys();
    emitMoreAvailableIfChanged();
  }
}

QStringList SqlPaginatedModel::keyFieldNames() const {
  QStringList keys;
  keys << sortColumn_;
  keys += extraKeyFieldNames_;
  keys << idFieldName_;
  return keys;
}

QVector<QVariant> SqlPaginatedModel::edgeKeyValues(const QVector<QVariant> &row) const {
  QVector<QVariant> values;

  for (const auto &fieldName : keyFieldNames()) {
    const int idx = fieldIndex(fieldName, true);

    if (idx == -1) { return {}; } // chave nao encontrada nas colunas da pagina: cursor invalido

    values << row.value(idx);
  }

  return values;
}

void SqlPaginatedModel::updateEdgeKeys() {
  if (rows_.isEmpty()) {
    firstKeyValues_.clear();
    lastKeyValues_.clear();
    return;
  }

  firstKeyValues_ = edgeKeyValues(rows_.first());
  lastKeyValues_ = edgeKeyValues(rows_.last());
}

void SqlPaginatedModel::emitMoreAvailableIfChanged() {
  if (hasMoreAfter_ != lastMoreAvailable_) {
    lastMoreAvailable_ = hasMoreAfter_;
    emit moreAvailableChanged(hasMoreAfter_);
  }
}

QVector<QVector<QVariant>> SqlPaginatedModel::runQuery(const QString &sql) {
  SqlQuery query;

  if (not query.exec(sql)) { throw RuntimeException("Erro lendo dados paginados: " + query.lastError().text()); }

  const int columnCount = query.record().count();

  QVector<QVector<QVariant>> result;

  while (query.next()) {
    QVector<QVariant> row;
    row.reserve(columnCount);

    for (int i = 0; i < columnCount; ++i) { row << query.value(i); }

    result << row;
  }

  return result;
}

int SqlPaginatedModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid()) { return 0; }

  return rows_.size();
}

int SqlPaginatedModel::columnCount(const QModelIndex &parent) const {
  if (parent.isValid()) { return 0; }

  return fieldNames_.size();
}

QVariant SqlPaginatedModel::data(const QModelIndex &index, const int role) const {
  if (not index.isValid()) { return {}; }

  if (role == Qt::FontRole) {
    const int statusIdx = fieldIndex("status", true);

    if (statusIdx != -1) {
      const QString status = data(index.row(), statusIdx).toString();

      if (status == "CANCELADA" or status == "CANCELADO" or status == "SUBSTITUIDO") {
        QFont font;
        font.setStrikeOut(true);
        return font;
      }
    }

    return {};
  }

  if (role != Qt::DisplayRole and role != Qt::EditRole) { return {}; }

  return data(index.row(), index.column());
}

QVariant SqlPaginatedModel::headerData(const int section, const Qt::Orientation orientation, const int role) const {
  if (orientation != Qt::Horizontal or role != Qt::DisplayRole) { return QAbstractTableModel::headerData(section, orientation, role); }
  if (section < 0 or section >= fieldNames_.size()) { return {}; }

  const QString &field = fieldNames_.at(section);

  return headerLabels_.value(field, field);
}

void SqlPaginatedModel::sort(const int column, const Qt::SortOrder order) {
  if (column < 0 or column >= fieldNames_.size() or not factory_) { return; }

  sortColumn_ = fieldNames_.at(column);
  sortOrder_ = order;
  builder_ = factory_(sortColumn_, sortOrder_);

  loadFirstPage();
}

QVariant SqlPaginatedModel::data(const int row, const QString &column) const { return data(row, fieldIndex(column)); }

QVariant SqlPaginatedModel::data(const int row, const int column) const {
  if (row == -1 or column == -1) { throw RuntimeException("Erro: linha/coluna -1 SqlPaginatedModel"); }
  if (row < 0 or row >= rows_.size() or column < 0 or column >= fieldNames_.size()) { return {}; }

  return rows_.at(row).at(column);
}

int SqlPaginatedModel::fieldIndex(const QString &fieldName, const bool silent) const {
  for (int i = 0; i < fieldNames_.size(); ++i) {
    if (fieldNames_.at(i).compare(fieldName, Qt::CaseInsensitive) == 0) { return i; }
  }

  if (not silent) { throw RuntimeException("\"" + fieldName + "\" não encontrado no model paginado!"); }

  return -1;
}

QString SqlPaginatedModel::toSqlLiteral(const QVariant &value) {
  if (not value.isValid() or value.isNull()) { return "NULL"; }

  if (value.userType() == QMetaType::QDateTime) { return "'" + value.toDateTime().toString("yyyy-MM-dd HH:mm:ss") + "'"; }
  if (value.userType() == QMetaType::QDate) { return "'" + value.toDate().toString("yyyy-MM-dd") + "'"; }

  if (value.userType() == QMetaType::Int or value.userType() == QMetaType::UInt or value.userType() == QMetaType::LongLong or value.userType() == QMetaType::ULongLong or
      value.userType() == QMetaType::Double or value.userType() == QMetaType::Bool) {
    return value.toString();
  }

  return "'" + qApp->sanitizeSQL(value.toString()) + "'";
}

QString SqlPaginatedModel::buildKeysetWhere(const QVector<KeyExpr> &keys, const QVector<QVariant> &cursorValues, const bool forward) {
  if (keys.isEmpty() or cursorValues.size() != keys.size()) { return "1"; } // sem cursor: 1a pagina, sem filtro de continuacao

  QStringList clauses;

  for (int i = 0; i < keys.size(); ++i) {
    QStringList prefix;

    for (int j = 0; j < i; ++j) { prefix << equalClause(keys.at(j).expr, cursorValues.at(j)); }

    const bool isLast = (i == keys.size() - 1);
    const QString cmp = isLast ? lessGreaterNotNull(keys.at(i).expr, cursorValues.at(i), keys.at(i).order, forward) : lessGreaterNullable(keys.at(i).expr, cursorValues.at(i), keys.at(i).order, forward);

    prefix << "(" + cmp + ")";

    clauses << "(" + prefix.join(" AND ") + ")";
  }

  // Envolvido num parenteses externo: quem chama concatena isto com " AND " junto de outros filtros
  // (ex.: "n.tipo = 'ENTRADA' AND " + buildKeysetWhere(...)) -- sem esse parenteses, o OR de N partes
  // "vaza" por precedencia (AND liga mais forte que OR em SQL), perdendo o filtro externo nos ramos.
  return "(" + clauses.join(" OR ") + ")";
}

QString SqlPaginatedModel::buildOrderBy(const QVector<KeyExpr> &keys, const bool forward) {
  const QString isNullDir = forward ? "ASC" : "DESC"; // nulos sempre por ultimo: "continuar pra frente" = isNull crescente

  QStringList parts;

  for (int i = 0; i < keys.size(); ++i) {
    const auto &key = keys.at(i);
    const bool descending = (key.order == Qt::DescendingOrder);
    const bool effectiveDescending = (descending == forward);
    const QString dir = effectiveDescending ? "DESC" : "ASC";

    const bool isLast = (i == keys.size() - 1);

    if (not isLast) { parts << "(" + key.expr + " IS NULL) " + isNullDir; }

    parts << key.expr + " " + dir;
  }

  return parts.join(", ");
}

#ifndef IMPRESSAO_H
#define IMPRESSAO_H

#include <QSqlQuery>

#include "lrreportengine.h"
#include "sqlrelationaltablemodel.h"

class Impressao final {

public:
  explicit Impressao(const QString &id);
  ~Impressao() = default;
  Impressao(const Impressao &) = delete;
  auto print() -> void;

private:
  // attributes
  enum class Tipo { Orcamento, Venda } tipo;
  const QString id;
  QSqlQuery queryCliente;
  QSqlQuery queryEndEnt;
  QSqlQuery queryEndFat;
  QSqlQuery queryLoja;
  QSqlQuery queryLojaEnd;
  QSqlQuery query;
  QSqlQuery queryProfissional;
  QSqlQuery queryVendedor;
  SqlRelationalTableModel modelItem;
  // methods
  auto setQuerys() -> bool;
  auto verificaTipo() -> void;
};

#endif // IMPRESSAO_H

#pragma once

#include "sqltablemodel.h"

#include <QSqlQuery>

class PDF final {

public:
  enum class Tipo { Orcamento, Venda };
  explicit PDF(const QString &id, const Tipo tipo, QObject *parent);
  ~PDF() = default;
  PDF(const PDF &) = delete;
  auto gerarPdf() -> void;

private:
  // attributes
  const Tipo tipo;
  const QString id;
  QObject *parent;
  QSqlQuery queryCliente;
  QSqlQuery queryEndEnt;
  QSqlQuery queryEndFat;
  QSqlQuery queryLoja;
  QSqlQuery queryLojaEnd;
  QSqlQuery query;
  QSqlQuery queryProfissional;
  QSqlQuery queryVendedor;
  SqlTableModel modelItem;
  // methods
  auto setQuerys() -> bool;
};

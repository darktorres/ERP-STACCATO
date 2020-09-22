#pragma once

#include "sqlquery.h"
#include "sqltablemodel.h"

#include <QWidget>

class PDF final {

public:
  enum class Tipo { Orcamento, Venda };
  explicit PDF(const QString &id, const Tipo tipo, QWidget *parent);
  ~PDF() = default;
  PDF(const PDF &) = delete;
  auto gerarPdf() -> void;

private:
  // attributes
  const Tipo tipo;
  const QString id;
  SqlQuery queryCliente;
  SqlQuery queryEndEnt;
  SqlQuery queryEndFat;
  SqlQuery queryLoja;
  SqlQuery queryLojaEnd;
  SqlQuery query;
  SqlQuery queryProfissional;
  SqlQuery queryVendedor;
  SqlTableModel modelItem;
  QWidget *parent;
  // methods
  auto setQuerys() -> void;
};

#pragma once

#include "sqlquery.h"
#include "sqltablemodel.h"

#include <QWidget>

class PDF final {

public:
  enum class Tipo { Orcamento, Venda };

  explicit PDF(const QString &id, const Tipo tipo, QWidget *parent);
  ~PDF() = default;

  auto gerarPdf() -> void;

  // TODO: make private?
  bool mostrarRT = false;

private:
  // attributes
  QString const id;
  QWidget *parent = nullptr;
  SqlQuery query;
  SqlQuery queryCliente;
  SqlQuery queryEndEnt;
  SqlQuery queryEndFat;
  SqlQuery queryLoja;
  SqlQuery queryLojaEnd;
  SqlQuery queryProfissional;
  SqlQuery queryVendedor;
  SqlTableModel modelItem;
  Tipo const tipo;
  // methods
  auto setQuerys() -> void;
};

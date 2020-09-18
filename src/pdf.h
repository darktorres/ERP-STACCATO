#pragma once

#include "sqltablemodel.h"

#include <QSqlQuery>
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
  QSqlQuery queryCliente;
  QSqlQuery queryEndEnt;
  QSqlQuery queryEndFat;
  QSqlQuery queryLoja;
  QSqlQuery queryLojaEnd;
  QSqlQuery query;
  QSqlQuery queryProfissional;
  QSqlQuery queryVendedor;
  SqlTableModel modelItem;
  QWidget *parent;
  // methods
  auto setQuerys() -> void;
};

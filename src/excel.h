#pragma once

#include "sqlquery.h"
#include "xlsxdocument.h"

class Excel final {

public:
  enum class Tipo { Orcamento, Venda };

  Excel(const QString &id, const Tipo tipo, QWidget *parent);

  // TODO: make private?
  bool mostrarRT = false;
  bool anexoCompra = false;
  int ordemCompra = 0;
  QString customFileName;

  auto gerarExcel() -> void;

private:
  // attributes
  QString const id;
  QString fileName;
  QWidget *parent = nullptr;
  SqlQuery query;
  SqlQuery queryCliente;
  SqlQuery queryEndEnt;
  SqlQuery queryEndFat;
  SqlQuery queryLoja;
  SqlQuery queryLojaEnd;
  SqlQuery queryProduto;
  SqlQuery queryProfissional;
  SqlQuery queryVendedor;
  Tipo const tipo;
  // methods
  auto hideUnusedRows(QXlsx::Document &xlsx) -> void;
  auto setQuerys() -> void;
};

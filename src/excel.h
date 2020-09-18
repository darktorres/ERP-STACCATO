#pragma once

#include "xlsxdocument.h"

#include <QSqlQuery>

class Excel final {

public:
  enum class Tipo { Orcamento, Venda };
  Excel(const QString &id, const Tipo tipo, QWidget *parent);
  auto gerarExcel(const int oc = 0, const bool isRepresentacao = false, const QString &representacao = QString()) -> void;
  auto getFileName() const -> QString;

private:
  // attributes
  const Tipo tipo;
  const QString id;
  QSqlQuery query;
  QSqlQuery queryCliente;
  QSqlQuery queryEndEnt;
  QSqlQuery queryEndFat;
  QSqlQuery queryLoja;
  QSqlQuery queryLojaEnd;
  QSqlQuery queryProduto;
  QSqlQuery queryProfissional;
  QSqlQuery queryVendedor;
  QString fileName;
  QWidget *parent;
  // methods
  auto hideUnusedRows(QXlsx::Document &xlsx) -> void;
  auto setQuerys() -> void;
};

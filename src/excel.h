#ifndef EXCEL_H
#define EXCEL_H

#include <QSqlQuery>
#include <QWidget>

#include <xlsxdocument.h>

class Excel final {

public:
  enum class Tipo { Orcamento, Venda };
  Excel(const QString &id, const Tipo tipo);
  auto gerarExcel(const int oc = 0, const bool isRepresentacao = false, const QString &representacao = QString()) -> bool;
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
  // methods
  auto hideUnusedRows(QXlsx::Document &xlsx) -> void;
  auto setQuerys() -> bool;
};

#endif // EXCEL_H

#ifndef IMPRESSAO_H
#define IMPRESSAO_H

#include <QSqlQuery>

#include "lrreportengine.h"
#include "sqlrelationaltablemodel.h"

class Impressao final : public QObject {
  Q_OBJECT

public:
  explicit Impressao(const QString &id);
  Impressao(const Impressao &) = delete;
  auto print() -> void;

signals:
  void errorSignal(const QString &error);
  void warningSignal(const QString &warning);
  void informationSignal(const QString &information);

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
  LimeReport::ReportEngine *report;
  // methods
  auto setQuerys() -> bool;
  auto verificaTipo() -> void;
};

#endif // IMPRESSAO_H

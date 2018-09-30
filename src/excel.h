#ifndef EXCEL_H
#define EXCEL_H

#include <QSqlQuery>
#include <QWidget>

#include <xlsxdocument.h>

class Excel final : public QObject {
  Q_OBJECT

public:
  Excel(QString id, QWidget *parent = nullptr);
  auto gerarExcel(const int oc = 0, const bool isRepresentacao = false, const QString &representacao = QString()) -> bool;
  auto getFileName() const -> QString;

signals:
  void errorSignal(const QString &error);
  void warningSignal(const QString &warning);
  void informationSignal(const QString &information);

private:
  // attributes
  enum class Tipo { Orcamento, Venda } tipo;
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
  auto setQuerys() -> bool;
  auto verificaTipo() -> void;
};

#endif // EXCEL_H

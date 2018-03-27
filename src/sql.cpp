#include <QDebug>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>

#include "sql.h"

bool Sql::updateVendaStatus(const QString &idVenda) {
  QStringList list = idVenda.split(", ");

  for (auto &item : list) {
    item.prepend("'");
    item.append("'");
  }

  list.join(",");

  const QString idVenda2 = list.join(",");

  QSqlQuery query;

  if (not query.exec("UPDATE venda v, venda_has_produto vp SET v.status = vp.status "
                     "WHERE v.idVenda = vp.idVenda AND vp.status = 'DEVOLVIDO' AND v.status != 'CANCELADO' AND v.devolucao = false AND v.idVenda IN (" +
                     idVenda2 + ")")) {
    QMessageBox::critical(nullptr, "Erro!", "Erro atualizando status 'DEVOLVIDO': " + query.lastError().text());
    return false;
  }

  if (not query.exec("UPDATE venda v, venda_has_produto vp SET v.status = vp.status "
                     "WHERE v.idVenda = vp.idVenda AND vp.status = 'ENTREGUE' AND v.status != 'CANCELADO' AND v.devolucao = false AND v.idVenda IN (" +
                     idVenda2 + ")")) {
    QMessageBox::critical(nullptr, "Erro!", "Erro atualizando status 'ENTREGUE': " + query.lastError().text());
    return false;
  }

  if (not query.exec("UPDATE venda v, venda_has_produto vp SET v.status = vp.status "
                     "WHERE v.idVenda = vp.idVenda AND vp.status = 'REPO. ENTREGA' AND v.status != 'CANCELADO' AND v.devolucao = false AND v.idVenda IN (" +
                     idVenda2 + ")")) {
    QMessageBox::critical(nullptr, "Erro!", "Erro atualizando status 'REPO. ENTREGA': " + query.lastError().text());
    return false;
  }

  if (not query.exec("UPDATE venda v, venda_has_produto vp SET v.status = vp.status "
                     "WHERE v.idVenda = vp.idVenda AND vp.status = 'REPO. RECEB.' AND v.status != 'CANCELADO' AND v.devolucao = false AND v.idVenda IN (" +
                     idVenda2 + ")")) {
    QMessageBox::critical(nullptr, "Erro!", "Erro atualizando status 'REPO. RECEB.': " + query.lastError().text());
    return false;
  }

  if (not query.exec("UPDATE venda v, venda_has_produto vp SET v.status = vp.status "
                     "WHERE v.idVenda = vp.idVenda AND vp.status = 'EM ENTREGA' AND v.status != 'CANCELADO' AND v.devolucao = false AND v.idVenda IN (" +
                     idVenda2 + ")")) {
    QMessageBox::critical(nullptr, "Erro!", "Erro atualizando status 'EM ENTREGA': " + query.lastError().text());
    return false;
  }

  if (not query.exec("UPDATE venda v, venda_has_produto vp SET v.status = vp.status "
                     "WHERE v.idVenda = vp.idVenda AND vp.status = 'ENTREGA AGEND.' AND v.status != 'CANCELADO' AND v.devolucao = false AND v.idVenda IN (" +
                     idVenda2 + ")")) {
    QMessageBox::critical(nullptr, "Erro!", "Erro atualizando status 'ENTREGA AGEND.': " + query.lastError().text());
    return false;
  }

  if (not query.exec("UPDATE venda v, venda_has_produto vp SET v.status = vp.status "
                     "WHERE v.idVenda = vp.idVenda AND vp.status = 'ESTOQUE' AND v.status != 'CANCELADO' AND v.devolucao = false AND v.idVenda IN (" +
                     idVenda2 + ")")) {
    QMessageBox::critical(nullptr, "Erro!", "Erro atualizando status 'ESTOQUE': " + query.lastError().text());
    return false;
  }

  if (not query.exec("UPDATE venda v, venda_has_produto vp SET v.status = vp.status "
                     "WHERE v.idVenda = vp.idVenda AND vp.status = 'EM RECEBIMENTO' AND v.status != 'CANCELADO' AND v.devolucao = false AND v.idVenda IN (" +
                     idVenda2 + ")")) {
    QMessageBox::critical(nullptr, "Erro!", "Erro atualizando status 'EM RECEBIMENTO': " + query.lastError().text());
    return false;
  }

  if (not query.exec("UPDATE venda v, venda_has_produto vp SET v.status = vp.status "
                     "WHERE v.idVenda = vp.idVenda AND vp.status = 'EM COLETA' AND v.status != 'CANCELADO' AND v.devolucao = false AND v.idVenda IN (" +
                     idVenda2 + ")")) {
    QMessageBox::critical(nullptr, "Erro!", "Erro atualizando status 'EM COLETA': " + query.lastError().text());
    return false;
  }

  if (not query.exec("UPDATE venda v, venda_has_produto vp SET v.status = vp.status "
                     "WHERE v.idVenda = vp.idVenda AND vp.status = 'EM FATURAMENTO' AND v.status != 'CANCELADO' AND v.devolucao = false AND v.idVenda IN (" +
                     idVenda2 + ")")) {
    QMessageBox::critical(nullptr, "Erro!", "Erro atualizando status 'EM FATURAMENTO': " + query.lastError().text());
    return false;
  }

  if (not query.exec("UPDATE venda v, venda_has_produto vp SET v.status = vp.status "
                     "WHERE v.idVenda = vp.idVenda AND vp.status = 'EM COMPRA' AND v.status != 'CANCELADO' AND v.devolucao = false AND v.idVenda IN (" +
                     idVenda2 + ")")) {
    QMessageBox::critical(nullptr, "Erro!", "Erro atualizando status 'EM COMPRA': " + query.lastError().text());
    return false;
  }

  if (not query.exec("UPDATE venda v, venda_has_produto vp SET v.status = vp.status "
                     "WHERE v.idVenda = vp.idVenda AND vp.status = 'INICIADO' AND v.status != 'CANCELADO' AND v.devolucao = false AND v.idVenda IN (" +
                     idVenda2 + ")")) {
    QMessageBox::critical(nullptr, "Erro!", "Erro atualizando status 'INICIADO': " + query.lastError().text());
    return false;
  }

  if (not query.exec("UPDATE venda v, venda_has_produto vp SET v.status = vp.status "
                     "WHERE v.idVenda = vp.idVenda AND vp.status = 'PENDENTE' AND v.status != 'CANCELADO' AND v.devolucao = false AND v.idVenda IN (" +
                     idVenda2 + ")")) {
    QMessageBox::critical(nullptr, "Erro!", "Erro atualizando status 'PENDENTE': " + query.lastError().text());
    return false;
  }

  return true;
}

#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>

#include "application.h"
#include "sql.h"

bool Sql::updateVendaStatus(const QString &idVenda) {
  QStringList list = idVenda.split(", ");

  for (auto &item : list) {
    item.prepend("'");
    item.append("'");
  }

  list.join(",");

  const QString idVenda2 = list.join(",");

  // TODO: como a devolucao vai entrar no fluxo de logistica o status dos produtos não vão mais ser fixos e devem ser alterados nessas querys tambem

  if (QSqlQuery query; not query.exec("UPDATE venda v, venda_has_produto vp SET v.status = vp.status "
                                      "WHERE v.idVenda = vp.idVenda AND vp.status = 'DEVOLVIDO' AND v.status != 'CANCELADO' AND v.devolucao = false AND v.idVenda IN (" +
                                      idVenda2 + ")")) {
    return qApp->enqueueError(false, "Erro atualizando status 'DEVOLVIDO': " + query.lastError().text());
  }

  if (QSqlQuery query; not query.exec("UPDATE venda v, venda_has_produto vp SET v.status = vp.status "
                                      "WHERE v.idVenda = vp.idVenda AND vp.status = 'ENTREGUE' AND v.status != 'CANCELADO' AND v.devolucao = false AND v.idVenda IN (" +
                                      idVenda2 + ")")) {
    return qApp->enqueueError(false, "Erro atualizando status 'ENTREGUE': " + query.lastError().text());
  }

  if (QSqlQuery query; not query.exec("UPDATE venda v, venda_has_produto vp SET v.status = vp.status "
                                      "WHERE v.idVenda = vp.idVenda AND vp.status = 'REPO. ENTREGA' AND v.status != 'CANCELADO' AND v.devolucao = false AND v.idVenda IN (" +
                                      idVenda2 + ")")) {
    return qApp->enqueueError(false, "Erro atualizando status 'REPO. ENTREGA': " + query.lastError().text());
  }

  if (QSqlQuery query; not query.exec("UPDATE venda v, venda_has_produto vp SET v.status = vp.status "
                                      "WHERE v.idVenda = vp.idVenda AND vp.status = 'REPO. RECEB.' AND v.status != 'CANCELADO' AND v.devolucao = false AND v.idVenda IN (" +
                                      idVenda2 + ")")) {
    return qApp->enqueueError(false, "Erro atualizando status 'REPO. RECEB.': " + query.lastError().text());
  }

  if (QSqlQuery query; not query.exec("UPDATE venda v, venda_has_produto vp SET v.status = vp.status "
                                      "WHERE v.idVenda = vp.idVenda AND vp.status = 'EM ENTREGA' AND v.status != 'CANCELADO' AND v.devolucao = false AND v.idVenda IN (" +
                                      idVenda2 + ")")) {
    return qApp->enqueueError(false, "Erro atualizando status 'EM ENTREGA': " + query.lastError().text());
  }

  if (QSqlQuery query; not query.exec("UPDATE venda v, venda_has_produto vp SET v.status = vp.status "
                                      "WHERE v.idVenda = vp.idVenda AND vp.status = 'ENTREGA AGEND.' AND v.status != 'CANCELADO' AND v.devolucao = false AND v.idVenda IN (" +
                                      idVenda2 + ")")) {
    return qApp->enqueueError(false, "Erro atualizando status 'ENTREGA AGEND.': " + query.lastError().text());
  }

  if (QSqlQuery query; not query.exec("UPDATE venda v, venda_has_produto vp SET v.status = vp.status "
                                      "WHERE v.idVenda = vp.idVenda AND vp.status = 'ESTOQUE' AND v.status != 'CANCELADO' AND v.devolucao = false AND v.idVenda IN (" +
                                      idVenda2 + ")")) {
    return qApp->enqueueError(false, "Erro atualizando status 'ESTOQUE': " + query.lastError().text());
  }

  if (QSqlQuery query; not query.exec("UPDATE venda v, venda_has_produto vp SET v.status = vp.status "
                                      "WHERE v.idVenda = vp.idVenda AND vp.status = 'EM RECEBIMENTO' AND v.status != 'CANCELADO' AND v.devolucao = false AND v.idVenda IN (" +
                                      idVenda2 + ")")) {
    return qApp->enqueueError(false, "Erro atualizando status 'EM RECEBIMENTO': " + query.lastError().text());
  }

  if (QSqlQuery query; not query.exec("UPDATE venda v, venda_has_produto vp SET v.status = vp.status "
                                      "WHERE v.idVenda = vp.idVenda AND vp.status = 'EM COLETA' AND v.status != 'CANCELADO' AND v.devolucao = false AND v.idVenda IN (" +
                                      idVenda2 + ")")) {
    return qApp->enqueueError(false, "Erro atualizando status 'EM COLETA': " + query.lastError().text());
  }

  if (QSqlQuery query; not query.exec("UPDATE venda v, venda_has_produto vp SET v.status = vp.status "
                                      "WHERE v.idVenda = vp.idVenda AND vp.status = 'EM FATURAMENTO' AND v.status != 'CANCELADO' AND v.devolucao = false AND v.idVenda IN (" +
                                      idVenda2 + ")")) {
    return qApp->enqueueError(false, "Erro atualizando status 'EM FATURAMENTO': " + query.lastError().text());
  }

  if (QSqlQuery query; not query.exec("UPDATE venda v, venda_has_produto vp SET v.status = vp.status "
                                      "WHERE v.idVenda = vp.idVenda AND vp.status = 'EM COMPRA' AND v.status != 'CANCELADO' AND v.devolucao = false AND v.idVenda IN (" +
                                      idVenda2 + ")")) {
    return qApp->enqueueError(false, "Erro atualizando status 'EM COMPRA': " + query.lastError().text());
  }

  if (QSqlQuery query; not query.exec("UPDATE venda v, venda_has_produto vp SET v.status = vp.status "
                                      "WHERE v.idVenda = vp.idVenda AND vp.status = 'INICIADO' AND v.status != 'CANCELADO' AND v.devolucao = false AND v.idVenda IN (" +
                                      idVenda2 + ")")) {
    return qApp->enqueueError(false, "Erro atualizando status 'INICIADO': " + query.lastError().text());
  }

  if (QSqlQuery query; not query.exec("UPDATE venda v, venda_has_produto vp SET v.status = vp.status "
                                      "WHERE v.idVenda = vp.idVenda AND vp.status = 'PENDENTE' AND v.status != 'CANCELADO' AND v.devolucao = false AND v.idVenda IN (" +
                                      idVenda2 + ")")) {
    return qApp->enqueueError(false, "Erro atualizando status 'PENDENTE': " + query.lastError().text());
  }

  return true;
}

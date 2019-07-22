#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>

#include "application.h"
#include "sql.h"

bool Sql::updateVendaStatus(const QStringList &idVendas) { return updateVendaStatus(idVendas.join(", ")); }

bool Sql::updateVendaStatus(const QString &idVendas) {
  QStringList list = idVendas.split(", ");

  for (auto &item : list) { item.prepend("'").append("'"); }

  return runQuerys(list.join(", "));
}

bool Sql::runQuerys(const QString &idVenda) {
  // TODO: como a devolucao vai entrar no fluxo de logistica o status dos produtos não vão mais ser fixos e devem ser alterados nessas querys tambem

  if (idVenda.isEmpty()) { return true; }

  const QString queryString =
      "UPDATE venda v, venda_has_produto vp SET v.status = vp.status WHERE v.idVenda = vp.idVenda AND vp.status = '%1' AND v.status != 'CANCELADO' AND v.devolucao = FALSE AND v.idVenda IN (%2)";

  if (QSqlQuery query; not query.exec(queryString.arg("DEVOLVIDO").arg(idVenda))) { return qApp->enqueueError(false, "Erro atualizando status 'DEVOLVIDO': " + query.lastError().text()); }
  if (QSqlQuery query; not query.exec(queryString.arg("ENTREGUE").arg(idVenda))) { return qApp->enqueueError(false, "Erro atualizando status 'ENTREGUE': " + query.lastError().text()); }
  if (QSqlQuery query; not query.exec(queryString.arg("REPO. ENTREGA").arg(idVenda))) { return qApp->enqueueError(false, "Erro atualizando status 'REPO. ENTREGA': " + query.lastError().text()); }
  if (QSqlQuery query; not query.exec(queryString.arg("REPO. RECEB.").arg(idVenda))) { return qApp->enqueueError(false, "Erro atualizando status 'REPO. RECEB.': " + query.lastError().text()); }
  if (QSqlQuery query; not query.exec(queryString.arg("EM ENTREGA").arg(idVenda))) { return qApp->enqueueError(false, "Erro atualizando status 'EM ENTREGA': " + query.lastError().text()); }
  if (QSqlQuery query; not query.exec(queryString.arg("ENTREGA AGEND.").arg(idVenda))) { return qApp->enqueueError(false, "Erro atualizando status 'ENTREGA AGEND.': " + query.lastError().text()); }
  if (QSqlQuery query; not query.exec(queryString.arg("ESTOQUE").arg(idVenda))) { return qApp->enqueueError(false, "Erro atualizando status 'ESTOQUE': " + query.lastError().text()); }
  if (QSqlQuery query; not query.exec(queryString.arg("EM RECEBIMENTO").arg(idVenda))) { return qApp->enqueueError(false, "Erro atualizando status 'EM RECEBIMENTO': " + query.lastError().text()); }
  if (QSqlQuery query; not query.exec(queryString.arg("EM COLETA").arg(idVenda))) { return qApp->enqueueError(false, "Erro atualizando status 'EM COLETA': " + query.lastError().text()); }
  if (QSqlQuery query; not query.exec(queryString.arg("EM FATURAMENTO").arg(idVenda))) { return qApp->enqueueError(false, "Erro atualizando status 'EM FATURAMENTO': " + query.lastError().text()); }
  if (QSqlQuery query; not query.exec(queryString.arg("EM COMPRA").arg(idVenda))) { return qApp->enqueueError(false, "Erro atualizando status 'EM COMPRA': " + query.lastError().text()); }
  if (QSqlQuery query; not query.exec(queryString.arg("INICIADO").arg(idVenda))) { return qApp->enqueueError(false, "Erro atualizando status 'INICIADO': " + query.lastError().text()); }
  if (QSqlQuery query; not query.exec(queryString.arg("PENDENTE").arg(idVenda))) { return qApp->enqueueError(false, "Erro atualizando status 'PENDENTE': " + query.lastError().text()); }

  return true;
}

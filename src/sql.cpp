#include "sql.h"

#include "application.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>

bool Sql::updateVendaStatus(const QStringList &idVendas) { return updateVendaStatus(idVendas.join(", ")); }

bool Sql::updateVendaStatus(const QString &idVendas) {
  QStringList list = idVendas.split(", ", Qt::SkipEmptyParts);
  list.removeDuplicates();

  for (auto const &idVenda : list) {
    if (QSqlQuery query; not query.exec("CALL update_venda_status('" + idVenda + "')")) { return qApp->enqueueException(false, "Erro atualizando status: " + query.lastError().text()); }
  }

  return true;
}

// TODO: como a devolucao vai entrar no fluxo de logistica o status dos produtos não vão mais ser fixos e devem ser alterados nessas querys tambem
// FIXME: recebimento de estoque altera os consumos que por sua vez altera venda_has_produto mas depende do pedido_fornecedor_has_produto ter vp.idVenda preenchido para esta função funcionar

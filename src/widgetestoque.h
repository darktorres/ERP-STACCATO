#ifndef WIDGETESTOQUE_H
#define WIDGETESTOQUE_H

#include "sqlquerymodel.h"
#include "sqlrelationaltablemodel.h"
#include "widget.h"

namespace Ui {
class WidgetEstoque;
}

class WidgetEstoque final : public Widget {
  Q_OBJECT

public:
  explicit WidgetEstoque(QWidget *parent = 0);
  ~WidgetEstoque();
  bool updateTables();

private slots:
  void on_table_activated(const QModelIndex &index);
  void on_table_entered(const QModelIndex &);
  void on_pushButtonRelatorio_clicked();

private:
  // attributes
  const QString view_estoque2 = "SELECT group_concat(DISTINCT `n`.`cnpjDest` SEPARATOR ',') AS `cnpjDest`, e.status, e.idEstoque, pf.fornecedor, e.descricao, e.consumo AS restante, e.un AS `unEst`, "
                                "if((`p`.`un` = `p`.`un2`), `p`.`un`, concat(`p`.`un`, '/', `p`.`un2`)) AS `unProd`, if(((`p`.`un` = 'M²') OR (`p`.`un` = 'M2') OR (`p`.`un` = 'ML')), "
                                "(consumo / `p`.`m2cx`), (consumo / `p`.`pccx`)) AS `Caixas`, e.lote, e.local, e.bloco, e.codComercial, group_concat(DISTINCT `n`.`numeroNFe` "
                                "SEPARATOR ', ') AS `nfe`, pf.idCompra, pf.dataPrevColeta, pf.dataRealColeta, pf.dataPrevReceb, pf.dataRealReceb FROM (SELECT e.idProduto, e.status, e.idEstoque, "
                                "e.descricao, e.codComercial, e.un, e.lote, e.local, e.bloco, e.quant, e.quant + coalesce(sum(consumo.quant), 0) AS consumo FROM estoque e LEFT JOIN "
                                "estoque_has_consumo consumo ON e.idEstoque = consumo.idEstoque WHERE e.status != 'CANCELADO' AND e.status != 'QUEBRADO' GROUP BY e.idEstoque) e LEFT JOIN "
                                "estoque_has_compra ehc ON e.idEstoque = ehc.idEstoque LEFT JOIN pedido_fornecedor_has_produto pf ON pf.idCompra = ehc.idCompra AND e.codComercial = pf.codComercial "
                                "LEFT JOIN estoque_has_nfe ehn ON e.idEstoque = ehn.idEstoque LEFT JOIN nfe n ON ehn.idNFe = n.idNFe LEFT JOIN produto p ON e.idProduto = p.idProduto";
  SqlQueryModel model;
  Ui::WidgetEstoque *ui;
  // methods
  bool setupTables();
  void montaFiltro();
};

#endif // WIDGETESTOQUE_H

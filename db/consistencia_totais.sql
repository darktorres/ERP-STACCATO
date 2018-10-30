SELECT 
    vp.idVenda,
    SUM(vp.parcial),
    v.subTotalBru,
    SUM(vp.parcialDesc),
    v.subTotalLiq,
    SUM(vp.total),
    v.total - v.frete,
    v.created
FROM
    venda_has_produto vp
        LEFT JOIN
    venda v ON vp.idVenda = v.idVenda
GROUP BY vp.idVenda
   HAVING abs(SUM(vp.parcial) - v.subTotalBru) > 1
   AND abs(SUM(vp.parcialDesc) - v.subTotalLiq) > 1;
   
   
select sum(parcial), sum(parcialDesc), sum(total) from venda_has_produto where idVenda = 'ALPH-181024';
select * from venda where idVenda = 'ALPH-181024';
select * from venda_has_produto where idVenda = 'ALPH-181024';
select produto, prcUnitario, quant, prcUnitario * quant, parcial, descUnitario * quant, parcialDesc from venda_has_produto where idVenda = 'GABR-181084';

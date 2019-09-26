#pragma once

#include <QDomElement>
#include <QStandardItemModel>

class XML final {

public:
  explicit XML(const QByteArray &fileContent, const QString &fileName = QString());
  auto lerValores(const QStandardItem *item) -> void;
  auto limparValores() -> void;

  const QByteArray fileContent;
  const QString fileName;
  QStandardItemModel model;
  QString local;

  QString chaveAcesso;
  QString nNF;
  // emit
  QString xFant;
  QString xNome;
  // dest
  QString cnpj;
  // produto
  QString codProd;
  QString codBarras;
  QString descricao;
  QString ncm;
  QString cfop;
  QString un;
  double quant = 0;
  double valorUnid = 0;
  double valor = 0;
  QString codBarrasTrib;
  QString unTrib;
  double quantTrib = 0;
  double valorTrib = 0;
  double desconto = 0;
  bool compoeTotal = false;
  QString numeroPedido;
  int itemPedido = 0;
  // icms
  QString tipoICMS;
  int orig = 0;
  int cstICMS = 0;
  int modBC = 0;
  double vBC = 0;
  double pICMS = 0;
  double vICMS = 0;
  int modBCST = 0;
  double pMVAST = 0;
  double vBCST = 0;
  double pICMSST = 0;
  double vICMSST = 0;
  // ipi
  int cEnq = 0;
  int cstIPI = 0;
  double vBCIPI = 0;
  double pIPI = 0;
  double vIPI = 0;
  // pis
  int cstPIS = 0;
  double vBCPIS = 0;
  double pPIS = 0;
  double vPIS = 0;
  // cofins
  int cstCOFINS = 0;
  double vBCCOFINS = 0;
  double pCOFINS = 0;
  double vCOFINS = 0;
  // total
  double vBC_Total = 0;
  double vICMS_Total = 0;
  double vICMSDeson_Total = 0;
  double vBCST_Total = 0;
  double vST_Total = 0;
  double vProd_Total = 0;
  double vFrete_Total = 0;
  double vSeg_Total = 0;
  double vDesc_Total = 0;
  double vII_Total = 0;
  double vPIS_Total = 0;
  double vCOFINS_Total = 0;
  double vOutro_Total = 0;
  double vNF_Total = 0;
  // transportadora
  QString xNomeTransp;

  // xml
  int idNFe = 0;

  // methods
private:
  auto lerCOFINSProduto(const QStandardItem *child) -> void;
  auto lerDadosProduto(const QStandardItem *child) -> void;
  auto lerICMSProduto(const QStandardItem *child) -> void;
  auto lerIPIProduto(const QStandardItem *child) -> void;
  auto lerPISProduto(const QStandardItem *child) -> void;
  auto lerTotais(const QStandardItem *child) -> void;
  auto montarArvore() -> void;
  auto readChild(const QDomElement &element, QStandardItem *elementItem) -> void;
};

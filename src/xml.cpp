#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>

#include "application.h"
#include "xml.h"

XML::XML(const QByteArray &fileContent, const QString &fileName) : fileContent(fileContent), fileName(fileName) { montarArvore(); }

void XML::readChild(const QDomElement &element, QStandardItem *elementItem) {
  QDomElement child = element.firstChildElement();

  for (; not child.isNull(); child = child.nextSiblingElement()) {
    if (child.firstChild().isText()) {
      QStandardItem *childItem = new QStandardItem(child.nodeName() + " - " + child.text());
      elementItem->appendRow(childItem);
      continue;
    }

    QDomNamedNodeMap map = child.attributes();
    QString attributes = child.nodeName();

    if (map.size() > 0) {
      for (int i = 0; i < map.size(); ++i) { attributes += " " + map.item(i).nodeName() + "=\"" + map.item(i).nodeValue() + "\""; }
    }

    auto *childItem = new QStandardItem(attributes);
    elementItem->appendRow(childItem);
    readChild(child, childItem);
  }
}

void XML::limparValores() {
  // produto
  codProd = "";
  codBarras = "";
  descricao = "";
  ncm = "";
  cfop = "";
  un = "";
  quant = 0;
  valorUnid = 0;
  valor = 0;
  codBarrasTrib = "";
  unTrib = "";
  quantTrib = 0;
  valorTrib = 0;
  desconto = 0;
  compoeTotal = false;
  numeroPedido = "";
  itemPedido = 0;
  // icms
  tipoICMS = "";
  orig = 0;
  cstICMS = 0;
  modBC = 0;
  vBC = 0;
  pICMS = 0;
  vICMS = 0;
  modBCST = 0;
  pMVAST = 0;
  vBCST = 0;
  pICMSST = 0;
  vICMSST = 0;
  // ipi
  cEnq = 0;
  cstIPI = 0;
  vBCIPI = 0;
  pIPI = 0;
  vIPI = 0;
  // pis
  cstPIS = 0;
  vBCPIS = 0;
  pPIS = 0;
  vPIS = 0;
  // cofins
  cstCOFINS = 0;
  vBCCOFINS = 0;
  pCOFINS = 0;
  vCOFINS = 0;
}

void XML::lerValores(const QStandardItem *item) {
  for (int row = 0; row < item->rowCount(); ++row) {
    for (int col = 0; col < item->columnCount(); ++col) {
      const QStandardItem *child = item->child(row, col);
      const QString parentText = child->parent()->text();
      QString text = child->text();

      if (text.left(6) == "infNFe") { chaveAcesso = text.mid(text.indexOf("Id=") + 7, 44); }
      if (text.left(3) == "nNF") { nNF = text.remove(0, 6); }

      if (parentText == "emit" and text.left(7) == "xFant -") { xFant = text.remove(0, 8); }
      if (parentText == "emit" and text.left(7) == "xNome -") { xNome = text.remove(0, 8); }
      if (parentText == "emit" and text.left(6) == "CNPJ -") { cnpjOrig = text.remove(0, 7); }
      if (parentText == "dest" and text.left(6) == "CNPJ -") { cnpjDest = text.remove(0, 7); }
      if (parentText == "transporta" and text.left(7) == "xNome -") { xNomeTransp = text.remove(0, 8); }

      if (parentText == "prod") { lerDadosProduto(child); }
      if (text == "ICMS") { tipoICMS = child->child(0, 0)->text(); }
      if (parentText == tipoICMS) { lerICMSProduto(child); }
      if (parentText == "IPI" or parentText == "IPITrib" or parentText == "IPINT") { lerIPIProduto(child); }
      if (parentText == "PISAliq") { lerPISProduto(child); }
      if (parentText == "COFINSAliq") { lerCOFINSProduto(child); }

      if (parentText == "ICMSTot") { lerTotais(child); }

      if (child->hasChildren()) { lerValores(child); }
    }
  }
}

void XML::lerDadosProduto(const QStandardItem *child) {
  QString text = child->text();

  if (text.left(7) == "cProd -") { codProd = text.remove(0, 8); }
  if (text.left(6) == "cEAN -") { codBarras = text.remove(0, 7); }
  if (text.left(7) == "xProd -") { descricao = text.remove(0, 8); }
  if (text.left(5) == "NCM -") { ncm = text.remove(0, 6); }
  if (text.left(6) == "CFOP -") { cfop = text.remove(0, 7); }
  if (text.left(6) == "uCom -") { un = text.remove(0, 7).toUpper(); }
  if (text.left(6) == "qCom -") { quant = text.remove(0, 7).toDouble(); }
  if (text.left(8) == "vUnCom -") { valorUnid = text.remove(0, 9).toDouble(); }
  if (text.left(7) == "vProd -") { valor = text.remove(0, 8).toDouble(); }
  if (text.left(10) == "cEANTrib -") { codBarrasTrib = text.remove(0, 11); }
  if (text.left(7) == "uTrib -") { unTrib = text.remove(0, 8); }
  if (text.left(7) == "qTrib -") { quantTrib = text.remove(0, 8).toDouble(); }
  if (text.left(9) == "vUnTrib -") { valorTrib = text.remove(0, 10).toDouble(); }
  if (text.left(7) == "vDesc -") { desconto = text.remove(0, 8).toDouble(); }
  if (text.left(8) == "indTot -") { compoeTotal = static_cast<bool>(text.remove(0, 9).toInt()); }
  if (text.left(6) == "xPed -") { numeroPedido = text.remove(0, 7); }
  if (text.left(10) == "nItemPed -") { itemPedido = text.remove(0, 11).toInt(); }

    // remove 'A' from end of product code
    if (xNome == "CECRISA REVEST. CERAMICOS S.A." and codProd.endsWith("A")) {
      codProd = codProd.left(codProd.size() - 1);
      desconto = 0;
    }
  }
}

void XML::lerICMSProduto(const QStandardItem *child) {
  QString text = child->text();

  if (text.left(6) == "orig -") { orig = text.remove(0, 7).toInt(); }
  if (text.left(5) == "CST -") { cstICMS = text.remove(0, 6).toInt(); }
  if (text.left(7) == "modBC -") { modBC = text.remove(0, 8).toInt(); }
  if (text.left(5) == "vBC -") { vBC = text.remove(0, 6).toDouble(); }
  if (text.left(7) == "pICMS -") { pICMS = text.remove(0, 8).toDouble(); }
  if (text.left(7) == "vICMS -") { vICMS = text.remove(0, 8).toDouble(); }
  if (text.left(9) == "modBCST -") { modBCST = text.remove(0, 10).toInt(); }
  if (text.left(8) == "pMVAST -") { pMVAST = text.remove(0, 9).toDouble(); }
  if (text.left(7) == "vBCST -") { vBCST = text.remove(0, 8).toDouble(); }
  if (text.left(9) == "pICMSST -") { pICMSST = text.remove(0, 10).toDouble(); }
  if (text.left(9) == "vICMSST -") { vICMSST = text.remove(0, 10).toDouble(); }
}

void XML::lerIPIProduto(const QStandardItem *child) {
  QString text = child->text();

  if (text.left(6) == "cEnq -") { cEnq = text.remove(0, 7).toInt(); }

  if (text.left(5) == "CST -") { cstIPI = text.remove(0, 6).toInt(); }
  if (text.left(5) == "vBC -") { vBCIPI = text.remove(0, 6).toDouble(); }
  if (text.left(6) == "pIPI -") { pIPI = text.remove(0, 7).toDouble(); }
  if (text.left(6) == "vIPI -") { vIPI = text.remove(0, 7).toDouble(); }

  if (text.left(5) == "CST -") { cstIPI = text.remove(0, 6).toInt(); }
}

void XML::lerPISProduto(const QStandardItem *child) {
  QString text = child->text();

  if (text.left(5) == "CST -") { cstPIS = text.remove(0, 6).toInt(); }
  if (text.left(5) == "vBC -") { vBCPIS = text.remove(0, 6).toDouble(); }
  if (text.left(6) == "pPIS -") { pPIS = text.remove(0, 7).toDouble(); }
  if (text.left(6) == "vPIS -") { vPIS = text.remove(0, 7).toDouble(); }
}

void XML::lerCOFINSProduto(const QStandardItem *child) {
  QString text = child->text();

  if (text.left(5) == "CST -") { cstCOFINS = text.remove(0, 6).toInt(); }
  if (text.left(5) == "vBC -") { vBCCOFINS = text.remove(0, 6).toDouble(); }
  if (text.left(9) == "pCOFINS -") { pCOFINS = text.remove(0, 10).toDouble(); }
  if (text.left(9) == "vCOFINS -") { vCOFINS = text.remove(0, 10).toDouble(); }
}

void XML::lerTotais(const QStandardItem *child) {
  QString text = child->text();

  if (text.left(5) == "vBC -") { vBC_Total = text.remove(0, 6).toDouble(); }
  if (text.left(7) == "vICMS -") { vICMS_Total = text.remove(0, 8).toDouble(); }
  if (text.left(12) == "vICMSDeson -") { vICMSDeson_Total = text.remove(0, 13).toDouble(); }
  if (text.left(7) == "vBCST -") { vBCST_Total = text.remove(0, 8).toDouble(); }
  if (text.left(5) == "vST -") { vST_Total = text.remove(0, 6).toDouble(); }
  if (text.left(7) == "vProd -") { vProd_Total = text.remove(0, 8).toDouble(); }
  if (text.left(8) == "vFrete -") { vFrete_Total = text.remove(0, 9).toDouble(); }
  if (text.left(6) == "vSeg -") { vSeg_Total = text.remove(0, 7).toDouble(); }
  if (text.left(7) == "vDesc -") { vDesc_Total = text.remove(0, 8).toDouble(); }
  if (text.left(5) == "vII -") { vII_Total = text.remove(0, 6).toDouble(); }
  if (text.left(6) == "vIPI -") { vPIS_Total = text.remove(0, 7).toDouble(); }
  if (text.left(6) == "vPIS -") { vPIS_Total = text.remove(0, 7).toDouble(); }
  if (text.left(9) == "vCOFINS -") { vCOFINS_Total = text.remove(0, 10).toDouble(); }
  if (text.left(8) == "vOutro -") { vOutro_Total = text.remove(0, 9).toDouble(); }
  if (text.left(5) == "vNF -") { vNF_Total = text.remove(0, 6).toDouble(); }
}

void XML::montarArvore() {
  if (fileContent.isEmpty()) { return qApp->enqueueError("XML vazio!"); }

  QDomDocument document;
  QString error;

  if (not document.setContent(fileContent, &error)) { return qApp->enqueueError("Erro lendo arquivo: " + error); }

  QDomElement root = document.firstChildElement();
  QDomNamedNodeMap map = root.attributes();
  QString attributes = root.nodeName();

  if (map.size() > 0) {
    for (int i = 0; i < map.size(); ++i) { attributes += " " + map.item(i).nodeName() + "=\"" + map.item(i).nodeValue() + "\""; }
  }

  auto *rootItem = new QStandardItem(attributes);

  model.appendRow(rootItem);

  readChild(root, rootItem);

  lerValores(model.item(0, 0));
}

bool XML::validar(const Tipo tipo) {
  if (not verificaCNPJ(tipo) or verificaExiste() or not verificaValido()) { return false; }

  return true;
}

bool XML::verificaCNPJ(const Tipo tipo) {
  if (tipo == Tipo::Entrada and cnpjDest.left(11) != "09375013000") { return qApp->enqueueError(false, "CNPJ da nota não é da Staccato!"); }
  if (tipo == Tipo::Saida and cnpjOrig.left(11) != "09375013000") { return qApp->enqueueError(false, "CNPJ da nota não é da Staccato!"); }

  return true;
}

bool XML::verificaExiste() {
  QSqlQuery query;
  query.prepare("SELECT idNFe FROM nfe WHERE chaveAcesso = :chaveAcesso");
  query.bindValue(":chaveAcesso", chaveAcesso);

  if (not query.exec()) { return qApp->enqueueError(false, "Erro verificando se nota já cadastrada: " + query.lastError().text()); }

  if (query.first()) { return qApp->enqueueError(true, "Nota já cadastrada!"); }

  return false;
}

bool XML::verificaValido() {
  if (not fileContent.contains("Autorizado o uso da NF-e")) { return qApp->enqueueError(false, "NFe não está autorizada pela SEFAZ!"); }

  return true;
}

#include "xml.h"

#include "application.h"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>

XML::XML(const QByteArray &fileContent, const Tipo tipo) : fileContent(fileContent), tipo(tipo) { montarArvore(); }

XML::XML(const QByteArray &fileContent) : XML(fileContent, XML::Tipo::Nulo) {}

void XML::montarArvore() {
  if (fileContent.isEmpty()) {
    error = true;
    return qApp->enqueueException("XML vazio!");
  }

  QDomDocument document;
  QString errorText;

  if (not document.setContent(fileContent, &errorText)) {
    error = true;
    return qApp->enqueueException("Erro lendo arquivo: " + errorText);
  }

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

  if (produtos.isEmpty()) { error = true; }
}

void XML::readChild(const QDomElement &element, QStandardItem *elementItem) {
  QDomElement child = element.firstChildElement();

  for (; not child.isNull(); child = child.nextSiblingElement()) {
    if (child.firstChild().isText()) {
      elementItem->appendRow(new QStandardItem(child.nodeName() + " - " + child.text()));
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

void XML::limparValores() { produto = {}; }

void XML::lerValores(const QStandardItem *item) {
  for (int row = 0; row < item->rowCount(); ++row) {
    for (int col = 0; col < item->columnCount(); ++col) {
      const QStandardItem *child = item->child(row, col);
      const QString parentText = child->parent()->text();
      QString text = child->text();

      if (text.contains("infNFe")) { chaveAcesso = text.mid(text.indexOf("Id=") + 7, 44); }
      if (text.contains("nNF -")) { nNF = text.remove("nNF - "); }
      if (text.contains("dhEmi -")) { dataHoraEmissao = text.remove("dhEmi - "); }

      if (parentText == "emit" and text.contains("xFant -")) { xFant = text.remove("xFant - "); }
      if (parentText == "emit" and text.contains("xNome -")) { xNome = text.remove("xNome - "); }
      if (parentText == "emit" and text.contains("CNPJ -")) { cnpjOrig = text.remove("CNPJ - "); }
      if (parentText == "dest" and text.contains("CNPJ -")) { cnpjDest = text.remove("CNPJ - "); }
      if (parentText == "transporta" and text.contains("xNome -")) { xNomeTransp = text.remove("xNome - "); }

      if (parentText == "prod") { lerDadosProduto(child); }
      if (parentText == "ICMS" and text.left(4) == "ICMS") { produto.tipoICMS = text; }
      if (parentText == produto.tipoICMS) { lerICMSProduto(child); }
      if (parentText == "IPITrib" or parentText == "IPINT") { lerIPIProduto(child); }
      if (parentText == "PISAliq" or parentText == "PISQtde" or parentText == "PISNT" or parentText == "PISOutr") { lerPISProduto(child); }
      if (parentText == "COFINSAliq" or parentText == "COFINSQtde" or parentText == "COFINSNT" or parentText == "COFINSOutr") { lerCOFINSProduto(child); }

      if (parentText == "ICMSTot") { lerTotais(child); }

      if (child->hasChildren()) {
        lerValores(child);

        if (parentText == "COFINS") {
          produtos << produto;
          limparValores();
        }
      }
    }
  }
}

void XML::lerDadosProduto(const QStandardItem *child) {
  QString text = child->text();

  if (text.contains("cProd -")) { produto.codProd = text.remove("cProd - "); }
  if (text.contains("cEAN -")) { produto.codBarras = text.remove("cEAN - "); }
  if (text.contains("xProd -")) { produto.descricao = text.remove("xProd - "); }
  if (text.contains("NCM -")) { produto.ncm = text.remove("NCM - "); }
  if (text.contains("NVE -")) { produto.nve = text.remove("NVE - "); }
  if (text.contains("EXTIPI -")) { produto.extipi = text.remove("EXTIPI - "); }
  if (text.contains("CEST -")) { produto.cest = text.remove("CEST - "); }
  if (text.contains("CFOP -")) { produto.cfop = text.remove("CFOP - "); }
  if (text.contains("uCom -")) { produto.un = text.remove("uCom - ").toUpper(); }
  if (text.contains("qCom -")) { produto.quant = text.remove("qCom - ").toDouble(); }
  if (text.contains("vUnCom -")) { produto.valorUnid = text.remove("vUnCom - ").toDouble(); }
  if (text.contains("vProd -")) { produto.valor = text.remove("vProd - ").toDouble(); }
  if (text.contains("cEANTrib -")) { produto.codBarrasTrib = text.remove("cEANTrib - "); }
  if (text.contains("uTrib -")) { produto.unTrib = text.remove("uTrib - "); }
  if (text.contains("qTrib -")) { produto.quantTrib = text.remove("qTrib - ").toDouble(); }
  if (text.contains("vUnTrib -")) { produto.valorUnidTrib = text.remove("vUnTrib - ").toDouble(); }
  if (text.contains("vFrete -")) { produto.frete = text.remove("vFrete - ").toDouble(); }
  if (text.contains("vSeg -")) { produto.seguro = text.remove("vSeg - ").toDouble(); }
  if (text.contains("vDesc -")) { produto.desconto = text.remove("vDesc - ").toDouble(); }
  if (text.contains("vOutro -")) { produto.outros = text.remove("vOutro - ").toDouble(); }
  if (text.contains("indTot -")) { produto.compoeTotal = static_cast<bool>(text.remove("indTot - ").toInt()); }
  if (text.contains("xPed -")) { produto.numeroPedido = text.remove("xPed - "); }
  if (text.contains("nItemPed -")) { produto.itemPedido = text.remove("nItemPed - ").toInt(); }
}

void XML::lerICMSProduto(const QStandardItem *child) {
  QString text = child->text();

  if (text.contains("orig -")) { produto.orig = text.remove("orig - ").toInt(); }
  if (text.contains("CST -")) { produto.cstICMS = text.remove("CST - ").toInt(); }
  if (text.contains("modBC -")) { produto.modBC = text.remove("modBC - ").toInt(); }
  if (text.contains("vBC -")) { produto.vBC = text.remove("vBC - ").toDouble(); }
  if (text.contains("pICMS -")) { produto.pICMS = text.remove("pICMS - ").toDouble(); }
  if (text.contains("vICMS -")) { produto.vICMS = text.remove("vICMS - ").toDouble(); }
  if (text.contains("modBCST -")) { produto.modBCST = text.remove("modBCST - ").toInt(); }
  if (text.contains("pMVAST -")) { produto.pMVAST = text.remove("pMVAST - ").toDouble(); }
  if (text.contains("vBCST -")) { produto.vBCST = text.remove("vBCST - ").toDouble(); }
  if (text.contains("pICMSST -")) { produto.pICMSST = text.remove("pICMSST - ").toDouble(); }
  if (text.contains("vICMSST -")) { produto.vICMSST = text.remove("vICMSST - ").toDouble(); }
}

void XML::lerIPIProduto(const QStandardItem *child) {
  QString text = child->text();

  if (text.contains("cEnq -")) { produto.cEnq = text.remove("cEnq - ").toInt(); }

  if (text.contains("CST -")) { produto.cstIPI = text.remove("CST - ").toInt(); }
  if (text.contains("vBC -")) { produto.vBCIPI = text.remove("vBC - ").toDouble(); }
  if (text.contains("pIPI -")) { produto.pIPI = text.remove("pIPI - ").toDouble(); }
  if (text.contains("vIPI -")) { produto.vIPI = text.remove("vIPI - ").toDouble(); }

  if (text.contains("CST -")) { produto.cstIPI = text.remove("CST - ").toInt(); }
}

void XML::lerPISProduto(const QStandardItem *child) {
  QString text = child->text();

  if (text.contains("CST -")) { produto.cstPIS = text.remove("CST - ").toInt(); }
  if (text.contains("vBC -")) { produto.vBCPIS = text.remove("vBC - ").toDouble(); }
  if (text.contains("pPIS -")) { produto.pPIS = text.remove("pPIS - ").toDouble(); }
  if (text.contains("vPIS -")) { produto.vPIS = text.remove("vPIS - ").toDouble(); }
}

void XML::lerCOFINSProduto(const QStandardItem *child) {
  QString text = child->text();

  if (text.contains("CST -")) { produto.cstCOFINS = text.remove("CST - ").toInt(); }
  if (text.contains("vBC -")) { produto.vBCCOFINS = text.remove("vBC - ").toDouble(); }
  if (text.contains("pCOFINS -")) { produto.pCOFINS = text.remove("pCOFINS - ").toDouble(); }
  if (text.contains("vCOFINS -")) { produto.vCOFINS = text.remove("vCOFINS - ").toDouble(); }
}

void XML::lerTotais(const QStandardItem *child) {
  QString text = child->text();

  if (text.contains("vBC -")) { vBC_Total = text.remove("vBC - ").toDouble(); }
  if (text.contains("vICMS -")) { vICMS_Total = text.remove("vICMS - ").toDouble(); }
  if (text.contains("vICMSDeson -")) { vICMSDeson_Total = text.remove("vICMSDeson - ").toDouble(); }
  if (text.contains("vBCST -")) { vBCST_Total = text.remove("vBCST - ").toDouble(); }
  if (text.contains("vST -")) { vST_Total = text.remove("vST - ").toDouble(); }
  if (text.contains("vProd -")) { vProd_Total = text.remove("vProd - ").toDouble(); }
  if (text.contains("vFrete -")) { vFrete_Total = text.remove("vFrete - ").toDouble(); }
  if (text.contains("vSeg -")) { vSeg_Total = text.remove("vSeg - ").toDouble(); }
  if (text.contains("vDesc -")) { vDesc_Total = text.remove("vDesc - ").toDouble(); }
  if (text.contains("vII -")) { vII_Total = text.remove("vII - ").toDouble(); }
  if (text.contains("vIPI -")) { vPIS_Total = text.remove("vIPI - ").toDouble(); }
  if (text.contains("vPIS -")) { vPIS_Total = text.remove("vPIS - ").toDouble(); }
  if (text.contains("vCOFINS -")) { vCOFINS_Total = text.remove("vCOFINS - ").toDouble(); }
  if (text.contains("vOutro -")) { vOutro_Total = text.remove("vOutro - ").toDouble(); }
  if (text.contains("vNF -")) { vNF_Total = text.remove("vNF - ").toDouble(); }
}

bool XML::validar() {
  if (not verificaCNPJ() or not verificaValido()) { return false; }

  return true;
}

bool XML::verificaCNPJ() {
  if (tipo == Tipo::Entrada and cnpjDest.left(11) != "09375013000") { return qApp->enqueueException(false, "CNPJ da nota não é da Staccato!"); }
  if (tipo == Tipo::Saida and cnpjOrig.left(11) != "09375013000") { return qApp->enqueueException(false, "CNPJ da nota não é da Staccato!"); }

  return true;
}

bool XML::verificaValido() {
  if (not fileContent.contains("Autorizado o uso da NF-e")) { return qApp->enqueueError(false, "NFe não está autorizada pela SEFAZ!"); }

  return true;
}

bool XML::verificaNCMs() {
  QStringList ncms;
  bool erro = false;

  for (const auto &produto : produtos) {
    QSqlQuery query;

    if (not query.exec("SELECT 0 FROM ncm WHERE ncm = '" + produto.ncm + "'")) { return qApp->enqueueException(false, "Erro buscando ncm: " + query.lastError().text()); }

    if (not query.first()) {
      ncms << produto.ncm;
      erro = true;
    }
  }

  ncms.removeDuplicates();

  if (erro) { return qApp->enqueueError(false, "Os seguintes NCMs não foram encontrados na tabela!\nCadastre eles em \"Gerenciar NCMs\"!\n   -" + ncms.join("\n   -")); }

  return true;
}

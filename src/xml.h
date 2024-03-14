#pragma once

#include "xlsxdocument.h"

#include <QDomElement>
#include <QStandardItemModel>

class XML final {

private:
  struct Produto {
    // produto
    QString codProd;
    QString codBarras;
    QString descricao;
    QString ncm;
    QString nve;
    QString extipi;
    QString cest;
    QString cfop;
    QString un;
    double quant = 0;
    double valor = 0;
    QString codBarrasTrib;
    QString unTrib;
    double quantTrib = 0;
    double valorUnidTrib = 0;
    double frete = 0;
    double seguro = 0;
    double desconto = 0;
    double outros = 0;
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
    // gare
    double valorGare = 0;
  };

  struct Duplicata {
    QString nDup;
    QString dVenc;
    double vDup = 0;
  };

public:
  enum class Tipo { Entrada, Saida, Nulo };

  explicit XML(const QString &fileContent_, const Tipo tipo_, QWidget *parent_);
  explicit XML(const QString &fileContent_);

  auto exportarDados(QXlsx::Document &xlsx, int &row) -> void;
  auto validar() -> void;
  auto verificaNCMs() -> void;

  QVector<Produto> produtos;
  QVector<Duplicata> duplicatas;

  QString const fileContent;
  QStandardItemModel model;
  QString local;

  // xml
  int idNFe = 0;

  QString chaveAcesso;
  QString dataHoraEmissao;
  // identificacao
  QString nNF;
  // emitente
  QString xNome;
  QString cnpjOrig;
  // destinatario
  QString cnpjDest;
  // total
  double vNF_Total = 0;
  // transportadora
  QString xNomeTransp;

private:
  // attributes
  QDomDocument document;
  Produto produto;
  Duplicata duplicata;
  Tipo const tipo;
  QWidget *parent = nullptr;
  // methods
  auto lerCOFINSProduto(const QStandardItem *child) -> void;
  auto lerDadosProduto(const QStandardItem *child) -> void;
  auto lerICMSProduto(const QStandardItem *child) -> void;
  auto lerIPIProduto(const QStandardItem *child) -> void;
  auto lerPISProduto(const QStandardItem *child) -> void;
  auto lerTotais(const QStandardItem *child) -> void;
  auto lerValores(const QStandardItem *item) -> void;
  auto limparValores() -> void;
  auto montarArvore() -> void;
  auto readChild(const QDomElement &element, QStandardItem *elementItem) -> void;
  auto verificaCNPJ() -> void;
  auto verificaValido() -> void;
};

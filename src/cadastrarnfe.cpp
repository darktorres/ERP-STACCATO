#include "cadastrarnfe.h"
#include "ui_cadastrarnfe.h"

#include "acbrlib.h"
#include "application.h"
#include "cadastrocliente.h"
#include "checkboxdelegate.h"
#include "file.h"
#include "porcentagemdelegate.h"
#include "reaisdelegate.h"
#include "sql.h"
#include "user.h"

#include <QDate>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlRecord>

CadastrarNFe::CadastrarNFe(const QString &idVenda, const QStringList &items, const Tipo tipo, QWidget *parent) : QDialog(parent), idVenda(idVenda), tipo(tipo), ui(new Ui::CadastrarNFe) {
  ui->setupUi(this);

  setWindowFlags(Qt::Window);

  const int lojaACBr = User::getSetting("User/lojaACBr").toInt();

  if (lojaACBr == 0) { throw RuntimeError("Escolha a loja a ser utilizada em \"Opções->Configurações->ACBr->Loja\"!", this); }

  const QString emailContabilidade = User::getSetting("User/emailContabilidade").toString();

  if (emailContabilidade.isEmpty()) { throw RuntimeError(R"("Email Contabilidade" não está configurado! Ajuste no menu "Opções->Configurações")", this); }

  const QString emailLogistica = User::getSetting("User/emailLogistica").toString();

  if (emailLogistica.isEmpty()) { throw RuntimeError(R"("Email Logistica" não está configurado! Ajuste no menu "Opções->Configurações")", this); }

  ui->itemBoxLoja->setSearchDialog(SearchDialog::loja(this));
  ui->itemBoxLoja->setId(User::getSetting("User/lojaACBr"));

  setupTables();

  mapper.setModel(&modelProduto);
  mapper.setSubmitPolicy(QDataWidgetMapper::AutoSubmit);

  mapper.addMapping(ui->doubleSpinBoxICMSvbc, modelProduto.fieldIndex("vBC"));
  mapper.addMapping(ui->doubleSpinBoxICMSpicms, modelProduto.fieldIndex("pICMS"));
  mapper.addMapping(ui->doubleSpinBoxICMSvicms, modelProduto.fieldIndex("vICMS"));
  mapper.addMapping(ui->doubleSpinBoxICMSpmvast, modelProduto.fieldIndex("pMVAST"));
  mapper.addMapping(ui->doubleSpinBoxICMSvbcst, modelProduto.fieldIndex("vBCST"));
  mapper.addMapping(ui->doubleSpinBoxICMSpicmsst, modelProduto.fieldIndex("pICMSST"));
  mapper.addMapping(ui->doubleSpinBoxICMSvicmsst, modelProduto.fieldIndex("vICMSST"));
  mapper.addMapping(ui->lineEditIPIcEnq, modelProduto.fieldIndex("cEnq"));
  mapper.addMapping(ui->doubleSpinBoxPISvbc, modelProduto.fieldIndex("vBCPIS"));
  mapper.addMapping(ui->doubleSpinBoxPISppis, modelProduto.fieldIndex("pPIS"));
  mapper.addMapping(ui->doubleSpinBoxPISvpis, modelProduto.fieldIndex("vPIS"));
  mapper.addMapping(ui->doubleSpinBoxCOFINSvbc, modelProduto.fieldIndex("vBCCOFINS"));
  mapper.addMapping(ui->doubleSpinBoxCOFINSpcofins, modelProduto.fieldIndex("pCOFINS"));
  mapper.addMapping(ui->doubleSpinBoxCOFINSvcofins, modelProduto.fieldIndex("vCOFINS"));

  ui->lineEditModelo->setInputMask("99;_");
  ui->lineEditSerie->setInputMask("999;_");
  ui->lineEditCodigo->setInputMask("99999999;_");
  ui->lineEditNumero->setInputMask("999999999;_");
  ui->lineEditFormatoPagina->setInputMask("9;_");

  ui->itemBoxCliente->setRegisterDialog("CadastroCliente");

  ui->itemBoxCliente->setSearchDialog(SearchDialog::cliente(this));
  ui->itemBoxEnderecoFaturamento->setSearchDialog(SearchDialog::enderecoCliente(this));
  ui->itemBoxEnderecoEntrega->setSearchDialog(SearchDialog::enderecoCliente(this));
  ui->itemBoxVeiculo->setSearchDialog(SearchDialog::veiculo(this));

  if (idVenda.isEmpty()) { throw RuntimeException("Venda vazio!", this); }

  ui->frameST->hide();

  prepararNFe(items);
}

CadastrarNFe::~CadastrarNFe() { delete ui; }

void CadastrarNFe::setupTables() {
  modelVenda.setTable("venda");

  modelVenda.setFilter("idVenda = '" + idVenda + "'");

  modelVenda.select();

  //----------------------------------------------------------

  modelLoja.setTable("loja");

  modelLoja.setFilter("idLoja = " + ui->itemBoxLoja->getId().toString());

  modelLoja.select();

  //----------------------------------------------------------

  // TODO: verificar porque essa view usa ABS() e IF() para não ter valores negativos, como não é feito NFe de devolução porque esse código para valores negativos?
  modelProduto.setTable("view_produto_estoque");

  modelProduto.setHeaderData("fornecedor", "Fornecedor");
  modelProduto.setHeaderData("produto", "Produto");
  modelProduto.setHeaderData("caixas", "Caixas");
  modelProduto.setHeaderData("descUnitario", "R$ Unit.");
  modelProduto.setHeaderData("quant", "Quant.");
  modelProduto.setHeaderData("un", "Un.");
  modelProduto.setHeaderData("quantCaixa", "Quant./Cx.");
  modelProduto.setHeaderData("codComercial", "Cód. Com.");
  modelProduto.setHeaderData("formComercial", "Form. Com.");
  modelProduto.setHeaderData("total", "Total");
  modelProduto.setHeaderData("reposicaoEntrega", "Reposição?");
  modelProduto.setHeaderData("codBarras", "Cód. Barras");
  modelProduto.setHeaderData("ncm", "NCM");
  modelProduto.setHeaderData("cest", "CEST");
  modelProduto.setHeaderData("cfop", "CFOP");

  ui->tableItens->setModel(&modelProduto);

  ui->tableItens->setPersistentColumns({"reposicaoEntrega"});

  ui->tableItens->setItemDelegateForColumn("descUnitario", new ReaisDelegate(this));
  ui->tableItens->setItemDelegateForColumn("total", new ReaisDelegate(this));
  ui->tableItens->setItemDelegateForColumn("reposicaoEntrega", new CheckBoxDelegate(true, this));
  ui->tableItens->setItemDelegateForColumn("vBC", new ReaisDelegate(this));
  ui->tableItens->setItemDelegateForColumn("pICMS", new PorcentagemDelegate(false, this));
  ui->tableItens->setItemDelegateForColumn("vICMS", new ReaisDelegate(this));
  ui->tableItens->setItemDelegateForColumn("pMVAST", new PorcentagemDelegate(false, this));
  ui->tableItens->setItemDelegateForColumn("vBCST", new ReaisDelegate(this));
  ui->tableItens->setItemDelegateForColumn("pICMSST", new PorcentagemDelegate(false, this));
  ui->tableItens->setItemDelegateForColumn("vICMSST", new ReaisDelegate(this));
  ui->tableItens->setItemDelegateForColumn("vBCPIS", new ReaisDelegate(this));
  ui->tableItens->setItemDelegateForColumn("pPIS", new PorcentagemDelegate(false, this));
  ui->tableItens->setItemDelegateForColumn("vPIS", new ReaisDelegate(this));
  ui->tableItens->setItemDelegateForColumn("vBCCOFINS", new ReaisDelegate(this));
  ui->tableItens->setItemDelegateForColumn("pCOFINS", new PorcentagemDelegate(false, this));
  ui->tableItens->setItemDelegateForColumn("vCOFINS", new ReaisDelegate(this));

  ui->tableItens->hideColumn("idRelacionado");
  ui->tableItens->hideColumn("peso");
  ui->tableItens->hideColumn("idProduto");
  ui->tableItens->hideColumn("idVendaProduto2");
  ui->tableItens->hideColumn("numeroPedido");
  ui->tableItens->hideColumn("itemPedido");
  ui->tableItens->hideColumn("unTrib");
}

QString CadastrarNFe::montarXML() {
  QString nfe;

  QTextStream stream(&nfe);

  stream << "NFE.CriarNFe(\"\n";

  writeIdentificacao(stream);
  writeEmitente(stream);
  writeDestinatario(stream);
  writeProduto(stream);
  writeTotal(stream);
  writeTransportadora(stream);
  writePagamento(stream);
  writeVolume(stream);
  writeComplemento(stream);

  stream << "\",1)\n"; // return xml

  return nfe;
}

QString CadastrarNFe::gerarNota(ACBr &acbrRemoto) {
  QString resposta = acbrRemoto.enviarComando(montarXML());
  //  qDebug() << "resposta: " << resposta;

  //-------------------------------------------

  if (not resposta.contains("OK")) { throw RuntimeException("ACBR " + resposta, this); }

  const QStringList respostaSplit = resposta.remove("OK: ").split("\r\n", Qt::SkipEmptyParts);

  if (not respostaSplit.filter("Alertas:", Qt::CaseInsensitive).isEmpty()) { throw RuntimeException(respostaSplit.at(1), this); }

  //-------------------------------------------

  QString filePath = respostaSplit.at(0);
  xml = respostaSplit.at(1);

  return filePath;
}

int CadastrarNFe::preCadastrarNota() {
  qDebug() << "precadastrar";

  qApp->startTransaction("CadastrarNFe::preCadastrarNota");

  QString tipoString;

  if (tipo == Tipo::Entrada) { tipoString = "ENTRADA"; }
  if (tipo == Tipo::Saida or tipo == Tipo::Futura or tipo == Tipo::SaidaAposFutura) { tipoString = "SAÍDA"; }

  SqlQuery queryNota;
  queryNota.prepare("INSERT INTO nfe (idVenda, numeroNFe, tipo, xml, status, chaveAcesso, cnpjOrig, cnpjDest, valor) "
                    "VALUES (:idVenda, :numeroNFe, :tipo, :xml, 'NOTA PENDENTE', :chaveAcesso, :cnpjOrig, :cnpjDest, :valor)");
  queryNota.bindValue(":idVenda", idVenda);
  queryNota.bindValue(":numeroNFe", ui->lineEditNumero->text());
  queryNota.bindValue(":tipo", tipoString);
  queryNota.bindValue(":xml", xml);
  queryNota.bindValue(":chaveAcesso", chaveAcesso);
  queryNota.bindValue(":cnpjOrig", clearStr(ui->lineEditEmitenteCNPJ->text()));
  queryNota.bindValue(":cnpjDest", clearStr(ui->lineEditDestinatarioCPFCNPJ->text()));
  queryNota.bindValue(":valor", ui->doubleSpinBoxValorNota->value());

  if (not queryNota.exec()) { throw RuntimeException("Erro guardando nota: " + queryNota.lastError().text()); }

  const QVariant idNFe = queryNota.lastInsertId();

  if (queryNota.lastInsertId().isNull()) { throw RuntimeException("Erro lastInsertId"); }

  if (tipo == Tipo::Entrada) {
    SqlQuery query;
    query.prepare("UPDATE venda_has_produto2 SET idNFeEntrada = :idNFeEntrada WHERE idVendaProduto2 = :idVendaProduto2");

    for (int row = 0; row < modelProduto.rowCount(); ++row) {
      query.bindValue(":idNFeEntrada", idNFe);
      query.bindValue(":idVendaProduto2", modelProduto.data(row, "idVendaProduto2"));

      if (not query.exec()) { throw RuntimeException("Erro salvando NFe nos produtos: " + query.lastError().text()); }
    }
  }

  if (tipo == Tipo::Saida or tipo == Tipo::SaidaAposFutura) {
    SqlQuery queryCompra;
    queryCompra.prepare("UPDATE pedido_fornecedor_has_produto2 SET status = 'EM ENTREGA' WHERE status = 'ENTREGA AGEND.' AND idVendaProduto2 = :idVendaProduto2");

    SqlQuery queryVenda;
    queryVenda.prepare("UPDATE venda_has_produto2 SET status = 'EM ENTREGA', idNFeSaida = :idNFeSaida WHERE status = 'ENTREGA AGEND.' AND idVendaProduto2 = :idVendaProduto2");

    SqlQuery queryVeiculo;
    queryVeiculo.prepare("UPDATE veiculo_has_produto SET status = 'EM ENTREGA', idNFeSaida = :idNFeSaida WHERE status = 'ENTREGA AGEND.' AND idVendaProduto2 = :idVendaProduto2");

    for (int row = 0; row < modelProduto.rowCount(); ++row) {
      queryCompra.bindValue(":idVendaProduto2", modelProduto.data(row, "idVendaProduto2"));

      if (not queryCompra.exec()) { throw RuntimeException("Erro atualizando status do pedido_fornecedor: " + queryCompra.lastError().text()); }

      //----------------------------------------

      queryVenda.bindValue(":idNFeSaida", idNFe);
      queryVenda.bindValue(":idVendaProduto2", modelProduto.data(row, "idVendaProduto2"));

      if (not queryVenda.exec()) { throw RuntimeException("Erro salvando NFe nos produtos: " + queryVenda.lastError().text()); }

      //----------------------------------------

      queryVeiculo.bindValue(":idVendaProduto2", modelProduto.data(row, "idVendaProduto2"));
      queryVeiculo.bindValue(":idNFeSaida", idNFe);

      if (not queryVeiculo.exec()) { throw RuntimeException("Erro atualizando carga veiculo: " + queryVeiculo.lastError().text()); }
    }

    Sql::updateVendaStatus(idVenda);
  }

  if (tipo == Tipo::Futura) {
    SqlQuery query;
    query.prepare("UPDATE venda_has_produto2 SET idNFeFutura = :idNFeFutura WHERE `idVendaProduto2` = :idVendaProduto2");

    for (int row = 0; row < modelProduto.rowCount(); ++row) {
      query.bindValue(":idNFeFutura", idNFe);
      query.bindValue(":idVendaProduto2", modelProduto.data(row, "idVendaProduto2"));

      if (not query.exec()) { throw RuntimeException("Erro salvando NFe nos produtos: " + query.lastError().text()); }
    }
  }

  qApp->endTransaction();

  return idNFe.toInt();
}

void CadastrarNFe::removerNota(const int idNFe) {
  qApp->startTransaction("CadastrarNFe::removerNota");

  if (tipo == Tipo::Entrada) {
    // TODO: verificar o status da devolucao a ser usado

    SqlQuery queryVenda;
    queryVenda.prepare("UPDATE venda_has_produto2 SET idNFeEntrada = NULL WHERE idNFeEntrada = :idNFeEntrada");
    queryVenda.bindValue(":idNFeEntrada", idNFe);

    if (not queryVenda.exec()) { throw RuntimeException("Erro removendo nfe da venda: " + queryVenda.lastError().text()); }
  }

  if (tipo == Tipo::Saida or tipo == Tipo::SaidaAposFutura) {
    SqlQuery queryVenda;
    queryVenda.prepare("UPDATE venda_has_produto2 SET status = 'ENTREGA AGEND.', idNFeSaida = NULL WHERE status = 'EM ENTREGA' AND idNFeSaida = :idNFeSaida");
    queryVenda.bindValue(":idNFeSaida", idNFe);

    if (not queryVenda.exec()) { throw RuntimeException("Erro removendo nfe da venda: " + queryVenda.lastError().text()); }

    //----------------------------------------

    SqlQuery queryVeiculo;
    queryVeiculo.prepare("UPDATE veiculo_has_produto SET status = 'ENTREGA AGEND.', idNFeSaida = NULL WHERE status = 'EM ENTREGA' AND idNFeSaida = :idNFeSaida");
    queryVeiculo.bindValue(":idNFeSaida", idNFe);

    if (not queryVeiculo.exec()) { throw RuntimeException("Erro removendo nfe do veiculo: " + queryVeiculo.lastError().text()); }

    //----------------------------------------

    SqlQuery queryCompra;
    queryCompra.prepare("UPDATE pedido_fornecedor_has_produto2 SET status = 'ENTREGA AGEND.' WHERE status = 'EM ENTREGA' AND idVendaProduto2 = :idVendaProduto2");

    for (int row = 0; row < modelProduto.rowCount(); ++row) {
      queryCompra.bindValue(":idVendaProduto2", modelProduto.data(row, "idVendaProduto2"));

      if (not queryCompra.exec()) { throw RuntimeException("Erro removendo nfe da compra: " + queryCompra.lastError().text()); }
    }

    Sql::updateVendaStatus(idVenda);
  }

  if (tipo == Tipo::Futura) { // aqui nunca precisa alterar status
    SqlQuery queryVenda;
    queryVenda.prepare("UPDATE venda_has_produto2 SET idNFeFutura = NULL WHERE idNFeFutura = :idNFeFutura");
    queryVenda.bindValue(":idNFeFutura", idNFe);

    if (not queryVenda.exec()) { throw RuntimeException("Erro removendo nfe da venda: " + queryVenda.lastError().text()); }
  }

  SqlQuery queryNota;
  queryNota.prepare("DELETE FROM nfe WHERE idNFe = :idNFe");
  queryNota.bindValue(":idNFe", idNFe);

  if (not queryNota.exec()) { throw RuntimeException("Erro removendo nota: " + queryNota.lastError().text()); }

  qApp->endTransaction();
}

void CadastrarNFe::processarResposta(const QString &resposta, const QString &filePath, const int idNFe, ACBr &acbrRemoto) {
  if (resposta.contains("xMotivo=Rejeição")) {
    qDebug() << "rejeicao";

    removerNota(idNFe);

    const QString rejeicao = resposta.split("\r\n", Qt::SkipEmptyParts).filter("xMotivo=Rejeição").first().remove("xMotivo=");

    manterAberto = true;
    throw RuntimeException(rejeicao, this);
  }

  if (resposta.contains("Erro Interno")) {
    qDebug() << "erro interno sefaz";

    removerNota(idNFe);

    manterAberto = true;
    throw RuntimeException("Erro interno na SEFAZ, tente enviar novamente!");
  }

  if (resposta.contains("Autorizado o uso da NF-e") or resposta.contains("Uso Denegado")) { return carregarArquivo(acbrRemoto, filePath); }

  throw RuntimeException("Resposta não tratada:\n" + resposta);
}

void CadastrarNFe::on_pushButtonEnviarNFE_clicked() {
  qApp->setSilent(false);

  validarDados();
  criarChaveAcesso();

  ACBr acbrRemoto;

  const QString filePath = gerarNota(acbrRemoto);

  if (not validarRegras(acbrRemoto, filePath)) { return; }

  const int idNFe = preCadastrarNota();

  try {
    enviarNFe(acbrRemoto, filePath, idNFe); // dont close if rejection
    enviarEmail(acbrRemoto, filePath);      // close if error

    ACBrLib::gerarDanfe(xml.toLatin1()); // close if error
  } catch (std::exception &) {
    if (not manterAberto) { close(); }
    manterAberto = false;
    throw;
  }

  close();
}

void CadastrarNFe::cadastrar(const int idNFe) {
  SqlQuery queryNFe;
  queryNFe.prepare("UPDATE nfe SET status = :status, xml = :xml WHERE status = 'NOTA PENDENTE' AND idNFe = :idNFe");
  queryNFe.bindValue(":xml", xml);
  queryNFe.bindValue(":idNFe", idNFe);

  if (xml.contains("Autorizado o uso da NF-e")) { queryNFe.bindValue(":status", "AUTORIZADO"); }
  if (xml.contains("Uso Denegado")) { queryNFe.bindValue(":status", "DENEGADA"); }

  if (not queryNFe.exec()) { throw RuntimeException("Erro atualizando XML da NFe: " + queryNFe.lastError().text()); }
}

void CadastrarNFe::updateTotais() {
  double baseICMS = 0;
  double valorICMS = 0;
  double valorPIS = 0;
  double valorCOFINS = 0;
  double valorProdutos = 0;

  for (int row = 0; row < modelProduto.rowCount(); ++row) {
    baseICMS += modelProduto.data(row, "vBC").toDouble();
    valorICMS += modelProduto.data(row, "vICMS").toDouble();
    valorPIS += QString::number(modelProduto.data(row, "vPIS").toDouble(), 'f', 2).toDouble();
    valorCOFINS += QString::number(modelProduto.data(row, "vCOFINS").toDouble(), 'f', 2).toDouble();
    valorProdutos += modelProduto.data(row, "total").toDouble();
  }

  const double valorFrete = ui->doubleSpinBoxValorFrete->value();
  const double valorNota = valorProdutos + valorFrete;

  ui->doubleSpinBoxBaseICMS->setValue(baseICMS);
  ui->doubleSpinBoxValorICMS->setValue(valorICMS);
  ui->doubleSpinBoxValorPIS->setValue(valorPIS);
  ui->doubleSpinBoxValorCOFINS->setValue(valorCOFINS);
  ui->doubleSpinBoxValorProdutos->setValue(valorProdutos);
  ui->doubleSpinBoxValorNota->setValue(valorNota);

  updateComplemento();
}

void CadastrarNFe::updateComplemento() {
  const double total =
      ui->doubleSpinBoxBaseICMS->value() + ui->doubleSpinBoxValorICMS->value() + ui->doubleSpinBoxValorPIS->value() + ui->doubleSpinBoxValorCOFINS->value(); // + ui->doubleSpinBoxValorIPI->value()

  const QString endereco = (ui->itemBoxEnderecoEntrega->getId() == 1)
                               ? "NÃO HÁ/RETIRA"
                               : ui->lineEditDestinatarioLogradouro_2->text() + ", " + ui->lineEditDestinatarioNumero_2->text() + " " + ui->lineEditDestinatarioComplemento_2->text() + " - " +
                                     ui->lineEditDestinatarioBairro_2->text() + " - " + ui->lineEditDestinatarioCidade_2->text() + " - " + ui->lineEditDestinatarioUF_2->text() + " - " +
                                     ui->lineEditDestinatarioCEP_2->text();

  const QString texto = "Venda de código " + modelVenda.data(0, "idVenda").toString() + ";END. ENTREGA: " + endereco +
                        ";Informações Adicionais de Interesse do Fisco: ICMS RECOLHIDO ANTECIPADAMENTE CONFORME ARTIGO 313Y;Total Aproximado de tributos federais, estaduais e municipais: R$ " +
                        QLocale(QLocale::Portuguese).toString(total);

  ui->infCompSistema->setPlainText(texto);
}

void CadastrarNFe::preencherNumeroNFe() {
  SqlQuery queryCnpj;
  queryCnpj.prepare("SELECT cnpj FROM loja WHERE idLoja = :idLoja");
  queryCnpj.bindValue(":idLoja", ui->itemBoxLoja->getId());

  if (not queryCnpj.exec()) { throw RuntimeException("Erro buscando CNPJ: " + queryCnpj.lastError().text(), this); }

  if (not queryCnpj.first()) { throw RuntimeException("CNPJ não encontrado da loja com id: " + ui->itemBoxLoja->getId().toString()); }

  const QString cnpj = clearStr(queryCnpj.value("cnpj").toString());

  SqlQuery queryNfe;

  if (not queryNfe.exec("SELECT COALESCE(MAX(numeroNFe), 0) + 1 AS numeroNFe FROM nfe WHERE cnpjOrig = '" + cnpj + "'")) {
    throw RuntimeException("Erro buscando idNFe: " + queryNfe.lastError().text(), this);
  }

  if (not queryNfe.first()) { throw RuntimeException("Número da NFe não encontrado!"); }

  const QString numeroNFe = queryNfe.value("numeroNFe").toString().rightJustified(9, '0');

  ui->lineEditNumero->setText(numeroNFe);
  ui->lineEditCodigo->setText("12121212");
}

void CadastrarNFe::prepararNFe(const QStringList &items) {
  modelProduto.setFilter("idVendaProduto2 IN (" + items.join(", ") + ")");

  modelProduto.select();

  preencherDadosNFe();
  preencherEmitente();
  preencherDestinatario();
  preencherTotais();
  preencherImpostos();
  preencherTransporte();
  buscarAliquotas();
  updateTotais();
  setConnections();

  ui->comboBoxRegime->setCurrentText("Tributação Normal");
}

void CadastrarNFe::criarChaveAcesso() {
  const QStringList listChave = {modelLoja.data(0, "codUF").toString(),
                                 qApp->serverDate().toString("yyMM"),
                                 clearStr(ui->lineEditEmitenteCNPJ->text()),
                                 ui->lineEditModelo->text(),
                                 ui->lineEditSerie->text(),
                                 ui->lineEditNumero->text(),
                                 "1",
                                 ui->lineEditCodigo->text()};

  chaveAcesso = listChave.join("");

  calculaDigitoVerificador();
}

QString CadastrarNFe::clearStr(const QString &str) const { return QString(str).remove(".").remove("/").remove("-").remove(" ").remove("(").remove(")"); }

void CadastrarNFe::calculaDigitoVerificador() {
  if (chaveAcesso.size() != 43) { throw RuntimeException("Erro no tamanho da chave: " + chaveAcesso); }

  int soma = 0;
  int mult = 4;

  for (const auto &i : std::as_const(chaveAcesso)) {
    soma += i.digitValue() * mult--;
    mult = (mult == 1) ? 9 : mult;
  }

  const int resto = soma % 11;
  const int numero = (resto < 2) ? 0 : 11 - resto;

  chaveAcesso += QString::number(numero);
}

void CadastrarNFe::writeIdentificacao(QTextStream &stream) {
  stream << "[Identificacao]\n";
  stream << "NaturezaOperacao = " + ui->comboBoxNatureza->currentText() + "\n";
  stream << "Modelo = " + ui->lineEditModelo->text() + "\n";
  stream << "Serie = " + ui->lineEditSerie->text() + "\n";
  stream << "Codigo = " << ui->lineEditCodigo->text() + "\n"; // 1
  stream << "Numero = " << ui->lineEditNumero->text() + "\n"; // 1
  stream << "Emissao = " + qApp->serverDate().toString("dd/MM/yyyy") + "\n";
  stream << "Saida = " + qApp->serverDate().toString("dd/MM/yyyy") + "\n";
  stream << "Tipo = " + ui->comboBoxTipo->currentText().left(1) + "\n";
  stream << "finNFe = " + ui->comboBoxFinalidade->currentText().left(1) + "\n";
  stream << "FormaPag = " + ui->lineEditFormatoPagina->text() + "\n";
  stream << "idDest = " + ui->comboBoxDestinoOperacao->currentText().left(1) + "\n";
  stream << "indPres = 1\n";
  stream << "indFinal = 1\n";

  if (tipo == Tipo::Entrada) {
    SqlQuery query;
    query.prepare("SELECT chaveAcesso FROM nfe WHERE idNFe = (SELECT vp3.idNFeSaida FROM venda_has_produto2 vp2 LEFT JOIN venda_has_produto2 vp3 ON vp2.idRelacionado = vp3.idVendaProduto2 WHERE "
                  "vp2.idVendaProduto2 = :idVendaProduto2)");
    query.bindValue(":idVendaProduto2", modelProduto.data(0, "idVendaProduto2"));

    if (not query.exec()) { throw RuntimeException("Erro buscando NFe referenciada: " + query.lastError().text(), this); }

    if (not query.first()) { throw RuntimeException("NFe referenciada não encontrada!"); }

    stream << "[NFRef001]\n";
    stream << "refNFe = " + query.value("chaveAcesso").toString() + "\n";
  }

  if (tipo == Tipo::SaidaAposFutura) {
    SqlQuery query;
    query.prepare("SELECT chaveAcesso FROM nfe WHERE idNFe = (SELECT idNFeFutura FROM venda_has_produto2 WHERE `idVendaProduto2` = :idVendaProduto2)");
    query.bindValue(":idVendaProduto2", modelProduto.data(0, "idVendaProduto2"));

    if (not query.exec()) { throw RuntimeException("Erro buscando NFe referenciada: " + query.lastError().text(), this); }

    if (not query.first()) { throw RuntimeException("NFe referenciada não encontrada!"); }

    stream << "[NFRef001]\n";
    stream << "refNFe = " + query.value("chaveAcesso").toString() + "\n";
  }
}

void CadastrarNFe::writeEmitente(QTextStream &stream) const {
  stream << "[Emitente]\n";
  stream << "CNPJ = " + clearStr(ui->lineEditEmitenteCNPJ->text()) + "\n";
  //  stream << "CNPJ = 99999090910270\n";
  stream << "IE = " + ui->lineEditEmitenteInscEstadual->text() + "\n";
  stream << "Razao = " + ui->lineEditEmitenteNomeRazao->text().left(59) + "\n";
  stream << "Fantasia = " + ui->lineEditEmitenteFantasia->text() + "\n";
  stream << "Fone = " + ui->lineEditEmitenteTel1->text() + "\n";
  stream << "CEP = " + clearStr(ui->lineEditEmitenteCEP->text()) + "\n";
  stream << "Logradouro = " + ui->lineEditEmitenteLogradouro->text() + "\n";
  stream << "Numero = " + ui->lineEditEmitenteNumero->text() + "\n";
  stream << "Complemento = " + ui->lineEditEmitenteComplemento->text() + "\n";
  stream << "Bairro = " + ui->lineEditEmitenteBairro->text() + "\n";
  stream << "cMun = " + queryIBGEEmit.value("codigo").toString() + "\n";
  stream << "Cidade = " + ui->lineEditEmitenteCidade->text() + "\n";
  stream << "UF = " + ui->lineEditEmitenteUF->text() + "\n";
}

void CadastrarNFe::writeDestinatario(QTextStream &stream) const {
  stream << "[Destinatario]\n";

  stream << "NomeRazao = " + ui->lineEditDestinatarioNomeRazao->text().left(59) + "\n";

  if (ui->lineEditDestinatarioCPFCNPJ->text().length() == 14) {
    stream << "CPF = " + clearStr(ui->lineEditDestinatarioCPFCNPJ->text()) + "\n";
    stream << "indIEDest = 9\n";
  }

  if (ui->lineEditDestinatarioCPFCNPJ->text().length() == 18) {
    stream << "CNPJ = " + clearStr(ui->lineEditDestinatarioCPFCNPJ->text()) + "\n";

    const QString inscEst = ui->lineEditDestinatarioInscEst->text();

    stream << (inscEst == "ISENTO" or inscEst.isEmpty() ? "indIEDest = 9" : "IE = " + clearStr(inscEst)) + "\n";
  }

  // TODO: se tel1 estiver vazio usar tel2
  stream << "Fone = " + ui->lineEditDestinatarioTel1->text() + "\n";
  stream << "CEP = " + clearStr(ui->lineEditDestinatarioCEP->text()) + "\n";
  stream << "Logradouro = " + ui->lineEditDestinatarioLogradouro->text().left(59) + "\n";
  stream << "Numero = " + ui->lineEditDestinatarioNumero->text() + "\n";
  stream << "Complemento = " + ui->lineEditDestinatarioComplemento->text().left(59) + "\n";

  const QString bairro = ui->lineEditDestinatarioBairro->text();

  if (bairro.isEmpty()) {
    stream << "Bairro = SEM BAIRRO\n";
  } else {
    stream << "Bairro = " + bairro + "\n";
  }

  stream << "cMun = " + queryIBGEDest.value("codigo").toString() + "\n";
  stream << "Cidade = " + ui->lineEditDestinatarioCidade->text() + "\n";
  stream << "UF = " + ui->lineEditDestinatarioUF->text() + "\n";
}

void CadastrarNFe::writeProduto(QTextStream &stream) const {
  double sumFrete = 0;

  for (int row = 0; row < modelProduto.rowCount(); ++row) {
    const QString numProd = QString::number(row + 1).rightJustified(3, '0'); // padding with zeros

    stream << "[Produto" + numProd + "]\n";
    stream << "CFOP = " + modelProduto.data(row, "cfop").toString() + "\n";

    const QString cest = modelProduto.data(row, "cest").toString();
    if (not cest.isEmpty()) { stream << "CEST = " + cest + "\n"; }

    stream << "NCM = " + modelProduto.data(row, "ncm").toString() + "\n";
    stream << "Codigo = " + modelProduto.data(row, "codComercial").toString() + "\n";

    const QString codBarras = modelProduto.data(row, "codBarras").toString();
    if (not codBarras.isEmpty()) { stream << "cEAN = " + codBarras + "\n"; }
    if (not codBarras.isEmpty()) { stream << "cEANTrib = " + codBarras + "\n"; }

    const QString produto = modelProduto.data(row, "produto").toString();
    QString formato = modelProduto.data(row, "formComercial").toString();
    formato = (formato.isEmpty() ? "" : " - " + formato);
    const QString descricao = produto + formato;
    const QString caixas = modelProduto.data(row, "caixas").toString();
    stream << "Descricao = " + descricao.left(100).remove("\"") + " (" + caixas + " Cx.)\n";

    stream << "Unidade = " + modelProduto.data(row, "un").toString() + "\n";
    stream << "Quantidade = " + modelProduto.data(row, "quant").toString() + "\n";
    stream << "ValorUnitario = " + QString::number(modelProduto.data(row, "descUnitario").toDouble(), 'f', 10) + "\n";

    const double total = modelProduto.data(row, "total").toDouble();
    stream << "ValorTotal = " + QString::number(total, 'f', 2) + "\n";

    const double proporcao = total / ui->doubleSpinBoxValorProdutos->value();
    const double frete = (ui->checkBoxFrete->isChecked() ? ui->doubleSpinBoxValorFrete->value() * proporcao : 0);
    sumFrete += QString::number(frete, 'f', 2).toDouble();

    if (sumFrete > ui->doubleSpinBoxValorFrete->value()) {
      const double diff = sumFrete - ui->doubleSpinBoxValorFrete->value();
      const double frete2 = frete - diff;
      stream << "vFrete = " + QString::number(frete2, 'f', 2) + "\n";
    } else {
      stream << "vFrete = " + QString::number(frete, 'f', 2) + "\n";
    }

    if (tipo == Tipo::Futura) { continue; }

    stream << "[ICMS" + numProd + "]\n";
    stream << "CST = " + modelProduto.data(row, "cstICMS").toString() + "\n";
    stream << "Modalidade = " + modelProduto.data(row, "modBC").toString() + "\n";
    stream << "ValorBase = " + modelProduto.data(row, "vBC").toString() + "\n";

    stream << "Aliquota = " + QString::number(modelProduto.data(row, "pICMS").toDouble(), 'f', 2) + "\n";

    stream << "Valor = " + modelProduto.data(row, "vICMS").toString() + "\n";

    stream << "[IPI" + numProd + "]\n";
    stream << "ClasseEnquadramento = " + modelProduto.data(row, "cEnq").toString() + "\n";
    stream << "CST = " + modelProduto.data(row, "cstIPI").toString() + "\n";

    stream << "[PIS" + numProd + "]\n";
    stream << "CST = " + modelProduto.data(row, "cstPIS").toString() + "\n";
    stream << "ValorBase = " + modelProduto.data(row, "vBCPIS").toString() + "\n";
    stream << "Aliquota = " + modelProduto.data(row, "pPIS").toString() + "\n";
    stream << "Valor = " + QString::number(modelProduto.data(row, "vPIS").toDouble(), 'f', 2) + "\n";

    stream << "[COFINS" + numProd + "]\n";
    stream << "CST = " + modelProduto.data(row, "cstCOFINS").toString() + "\n";
    stream << "ValorBase = " + modelProduto.data(row, "vBCCOFINS").toString() + "\n";
    stream << "Aliquota = " + modelProduto.data(row, "pCOFINS").toString() + "\n";
    stream << "Valor = " + QString::number(modelProduto.data(row, "vCOFINS").toDouble(), 'f', 2) + "\n";

    // PARTILHA ICMS

    const QString inscEst = ui->lineEditDestinatarioInscEst->text();

    if (ui->comboBoxDestinoOperacao->currentText().startsWith("2") and (inscEst == "ISENTO" or inscEst.isEmpty())) {
      stream << "[ICMSUFDest" + numProd + "]\n";
      stream << "vBCUFDest = " + modelProduto.data(row, "vBCPIS").toString() + "\n"; // TODO: should be valorProduto + frete + ipi + outros - desconto
      stream << "pICMSUFDest = " + queryPartilhaIntra.value("valor").toString() + "\n";
      stream << "pICMSInter = " + queryPartilhaInter.value("valor").toString() + "\n";

      const double diferencaICMS = (queryPartilhaIntra.value("valor").toDouble() - queryPartilhaInter.value("valor").toDouble()) / 100.;
      const double difal = modelProduto.data(row, "vBCPIS").toDouble() * diferencaICMS;

      stream << "pICMSInterPart = 100\n";
      stream << "vICMSUFDest = " + QString::number(difal, 'f', 2) + "\n";
    }

    // http://www.asseinfo.com.br/blog/difal-diferencial-de-aliquota-icms/
  }
}

void CadastrarNFe::writeTotal(QTextStream &stream) const {
  stream << "[Total]\n";
  stream << "BaseICMS = " + QString::number(ui->doubleSpinBoxBaseICMS->value(), 'f', 2) + "\n";
  stream << "ValorICMS = " + QString::number(ui->doubleSpinBoxValorICMS->value(), 'f', 2) + "\n";
  stream << "ValorIPI = " + QString::number(ui->doubleSpinBoxValorIPI->value(), 'f', 2) + "\n";
  stream << "ValorPIS = " + QString::number(ui->doubleSpinBoxValorPIS->value(), 'f', 2) + "\n";
  stream << "ValorCOFINS = " + QString::number(ui->doubleSpinBoxValorCOFINS->value(), 'f', 2) + "\n";
  stream << "ValorProduto = " + QString::number(ui->doubleSpinBoxValorProdutos->value(), 'f', 2) + "\n";
  const double valorFrete = (ui->checkBoxFrete->isChecked()) ? ui->doubleSpinBoxValorFrete->value() : 0;
  stream << "ValorFrete = " + QString::number(valorFrete, 'f', 2) + "\n";
  stream << "ValorNota = " + QString::number(ui->doubleSpinBoxValorNota->value(), 'f', 2) + "\n";

  // PARTILHA ICMS

  const QString inscEst = ui->lineEditDestinatarioInscEst->text();

  if (ui->comboBoxDestinoOperacao->currentText().startsWith("2") and (inscEst == "ISENTO" or inscEst.isEmpty())) {
    double totalIcmsDest = 0;

    const double diferencaICMS = (queryPartilhaIntra.value("valor").toDouble() - queryPartilhaInter.value("valor").toDouble()) / 100.;

    for (int row = 0; row < modelProduto.rowCount(); ++row) {
      const double difal = modelProduto.data(row, "vBCPIS").toDouble() * diferencaICMS;

      totalIcmsDest += QString::number(difal, 'f', 2).toDouble();
    }

    stream << "vICMSUFDest = " + QString::number(totalIcmsDest, 'f', 2) + "\n";
  }
}

void CadastrarNFe::writeTransportadora(QTextStream &stream) const {
  stream << "[Transportador]\n";
  stream << "FretePorConta = " << ui->comboBoxFreteConta->currentText().left(1) + "\n";

  const QString razaoSocial = ui->lineEditTransportadorRazaoSocial->text();

  if (ui->comboBoxDestinoOperacao->currentText().startsWith("2") or razaoSocial == "RETIRA" or razaoSocial == "CARRO EXTRA") { return; }

  stream << "NomeRazao = " << ui->lineEditTransportadorRazaoSocial->text() + "\n";
  stream << "CnpjCpf = " << ui->lineEditTransportadorCpfCnpj->text() + "\n";
  stream << "IE = " << ui->lineEditTransportadorInscEst->text() + "\n";
  stream << "Endereco = " << ui->lineEditTransportadorEndereco->text() + "\n";
  stream << "Cidade = " << ui->lineEditTransportadorMunicipio->text() + "\n";
  stream << "UF = " << ui->lineEditTransportadorUf->text() + "\n";
  stream << "ValorServico = \n";
  stream << "ValorBase = \n";
  stream << "Aliquota = \n";
  stream << "Valor = \n";
  stream << "CFOP = \n";
  stream << "CidadeCod = \n";
  stream << "Placa = " << ui->lineEditTransportadorPlaca->text().remove("-") + "\n";
  stream << "UFPlaca = " << ui->lineEditTransportadorUfPlaca->text() + "\n";
  stream << "RNTC = \n";
}

void CadastrarNFe::writePagamento(QTextStream &stream) {
  if (tipo == Tipo::Entrada) {
    stream << "[Pag001]\n";

    stream << "tPag = 90\n"; // sem pagamento
  }

  if (tipo == Tipo::Saida or tipo == Tipo::Futura or tipo == Tipo::SaidaAposFutura) {
    stream << "[Pag001]\n";

    stream << "tPag = 01\n";
    stream << "vPag = " + QString::number(ui->doubleSpinBoxValorNota->value(), 'f', 2) + "\n";
  }
}

void CadastrarNFe::writeVolume(QTextStream &stream) const {
  stream << "[Volume001]\n";

  stream << "Quantidade = " << ui->spinBoxVolumesQuant->text() + "\n";
  stream << "Especie = " << ui->lineEditVolumesEspecie->text() + "\n";
  stream << "Marca = " << ui->lineEditVolumesMarca->text() + "\n";
  stream << "Numeracao = " << ui->lineEditVolumesNumeracao->text() + "\n";
  stream << "PesoLiquido = " << QString::number(ui->doubleSpinBoxVolumesPesoLiq->value(), 'f', 3) + "\n";
  stream << "PesoBruto = " << QString::number(ui->doubleSpinBoxVolumesPesoBruto->value(), 'f', 3) + "\n";
}

void CadastrarNFe::writeComplemento(QTextStream &stream) const {
  if (tipo == Tipo::Entrada) {}

  if (tipo == Tipo::Saida or tipo == Tipo::Futura or tipo == Tipo::SaidaAposFutura) {
    const QString infUsuario = (ui->infCompUsuario->toPlainText().isEmpty()) ? "" : ui->infCompUsuario->toPlainText();
    const QString infComp = (infUsuario.isEmpty() ? "" : infUsuario + ";") + ui->infCompSistema->toPlainText();

    stream << "[DadosAdicionais]\n";
    stream << "infCpl = " + infComp + "\n";
  }
}

void CadastrarNFe::on_tableItens_dataChanged(const QModelIndex &index) {
  unsetConnections();

  try {
    const QString header = modelProduto.headerData(index.column(), Qt::Horizontal).toString();
    const int row = index.row();

    const double quant = modelProduto.data(row, "quant").toDouble();

    if (header == "R$ Unit.") {
      const double descUnitario = modelProduto.data(row, "descUnitario").toDouble();
      const double preco = descUnitario * quant;

      modelProduto.setData(row, "total", preco);
    }

    if (header == "Total") {
      const double total = modelProduto.data(row, "total").toDouble();
      const double preco = total / quant;

      modelProduto.setData(row, "descUnitario", preco);
    }
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();

  updateTotais();
}

void CadastrarNFe::on_tableItens_clicked(const QModelIndex &index) {
  if (not index.isValid()) {
    ui->frameICMS->setDisabled(true);
    ui->frameIPI->setDisabled(true);
    ui->framePIS->setDisabled(true);
    ui->frameCOFINS->setDisabled(true);
    ui->frameInterEstadual->setDisabled(true);
    return;
  }

  unsetConnections();

  try {
    const int row = index.row();

    ui->frameICMS->setEnabled(true);
    ui->frameIPI->setEnabled(true);
    ui->framePIS->setEnabled(true);
    ui->frameCOFINS->setEnabled(true);
    ui->frameInterEstadual->setEnabled(true);

    listarCfop();

    ui->comboBoxCfop->setCurrentIndex(ui->comboBoxCfop->findText(modelProduto.data(row, "cfop").toString(), Qt::MatchStartsWith));
    ui->comboBoxICMSOrig->setCurrentIndex(ui->comboBoxICMSOrig->findText(modelProduto.data(row, "orig").toString(), Qt::MatchStartsWith));
    ui->comboBoxSituacaoTributaria->setCurrentIndex(ui->comboBoxSituacaoTributaria->findText(modelProduto.data(row, "cstICMS").toString(), Qt::MatchStartsWith));

    on_comboBoxSituacaoTributaria_currentTextChanged(ui->comboBoxSituacaoTributaria->currentText());

    ui->comboBoxICMSModBc->setCurrentIndex(modelProduto.data(row, "modBC").toInt() + 1);
    ui->comboBoxICMSModBcSt->setCurrentIndex(modelProduto.data(row, "modBCST").toInt() + 1);
    ui->comboBoxIPIcst->setCurrentIndex(ui->comboBoxIPIcst->findText(modelProduto.data(row, "cstIPI").toString(), Qt::MatchStartsWith));
    ui->comboBoxPIScst->setCurrentIndex(ui->comboBoxPIScst->findText(modelProduto.data(row, "cstPIS").toString(), Qt::MatchStartsWith));
    ui->comboBoxCOFINScst->setCurrentIndex(ui->comboBoxCOFINScst->findText(modelProduto.data(row, "cstCOFINS").toString(), Qt::MatchStartsWith));

    SqlQuery queryCfop;

    if (ui->comboBoxTipo->currentText() == "0 Entrada") { queryCfop.prepare("SELECT NAT FROM cfop_entr WHERE CFOP_DE = :cfop OR CFOP_FE = :cfop"); }
    if (ui->comboBoxTipo->currentText() == "1 Saída") { queryCfop.prepare("SELECT NAT FROM cfop_sai WHERE CFOP_DE = :cfop OR CFOP_FE = :cfop"); }

    queryCfop.bindValue(":cfop", modelProduto.data(row, "cfop"));

    if (not queryCfop.exec()) { throw RuntimeException("Erro buscando CFOP: " + queryCfop.lastError().text(), this); }

    // -------------------------------------------------------------------------

    // ICMS Inter

    // TODO: arrumar esse codigo, nao é mais 80%
    if (ui->comboBoxDestinoOperacao->currentText().startsWith("2")) {
      ui->frameInterEstadual->setEnabled(true);

      ui->doubleSpinBoxPercentualFcpDestino->setValue(2);
      ui->doubleSpinBoxBaseCalculoDestinatario->setValue(modelProduto.data(row, "vBC").toDouble());
      ui->doubleSpinBoxAliquotaInternaDestinatario->setValue(queryPartilhaIntra.value("valor").toDouble());
      ui->doubleSpinBoxAliquotaInter->setValue(queryPartilhaInter.value("valor").toDouble());
      ui->doubleSpinBoxPercentualPartilha->setValue(80);

      const double diferencaICMS = (queryPartilhaIntra.value("valor").toDouble() - queryPartilhaInter.value("valor").toDouble()) / 100.;
      const double difal = modelProduto.data(row, "vBCPIS").toDouble() * diferencaICMS;

      ui->doubleSpinBoxPartilhaDestinatario->setValue(difal * 0.8);
      ui->doubleSpinBoxPartilhaRemetente->setValue(difal * 0.2);
      ui->doubleSpinBoxFcpDestino->setValue(modelProduto.data(row, "vBCPIS").toDouble() * 0.02);
    }

    // -------------------------------------------------------------------------

    mapper.setCurrentModelIndex(index);
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();
}

void CadastrarNFe::calculaSt() {
  const auto list = ui->tableItens->selectionModel()->selectedRows();

  if (list.isEmpty()) { return; }

  unsetConnections();

  try {
    ui->doubleSpinBoxICMSvicmsst->setValue(ui->doubleSpinBoxICMSvbcst->value() * ui->doubleSpinBoxICMSpicmsst->value() / 100);

    const int row = list.first().row();

    modelProduto.setData(row, "vBCST", ui->doubleSpinBoxICMSvbcst->value());
    modelProduto.setData(row, "pICMSST", ui->doubleSpinBoxICMSpicmsst->value());
    modelProduto.setData(row, "vICMSST", ui->doubleSpinBoxICMSvicmsst->value());

    updateTotais();
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();
}

void CadastrarNFe::on_doubleSpinBoxICMSvbcst_valueChanged() { calculaSt(); }

void CadastrarNFe::on_doubleSpinBoxICMSpicmsst_valueChanged() { calculaSt(); }

void CadastrarNFe::on_doubleSpinBoxICMSvicmsst_valueChanged() { calculaSt(); }

void CadastrarNFe::calculaIcms() {
  const auto list = ui->tableItens->selectionModel()->selectedRows();

  if (list.isEmpty()) { return; }

  unsetConnections();

  try {
    ui->doubleSpinBoxICMSvicms->setValue(ui->doubleSpinBoxICMSvbc->value() * ui->doubleSpinBoxICMSpicms->value() / 100);

    const int row = list.first().row();

    modelProduto.setData(row, "vBC", ui->doubleSpinBoxICMSvbc->value());
    modelProduto.setData(row, "pICMS", ui->doubleSpinBoxICMSpicms->value());
    modelProduto.setData(row, "vICMS", ui->doubleSpinBoxICMSvicms->value());

    updateTotais();
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();
}

void CadastrarNFe::on_doubleSpinBoxICMSvbc_valueChanged() { calculaIcms(); }

void CadastrarNFe::on_doubleSpinBoxICMSpicms_valueChanged() { calculaIcms(); }

void CadastrarNFe::on_doubleSpinBoxICMSvicms_valueChanged() { calculaIcms(); }

void CadastrarNFe::calculaPis() {
  const auto list = ui->tableItens->selectionModel()->selectedRows();

  if (list.isEmpty()) { return; }

  unsetConnections();

  try {
    ui->doubleSpinBoxPISvpis->setValue(ui->doubleSpinBoxPISvbc->value() * ui->doubleSpinBoxPISppis->value() / 100);

    const int row = list.first().row();

    modelProduto.setData(row, "vBCPIS", ui->doubleSpinBoxPISvbc->value());
    modelProduto.setData(row, "pPIS", ui->doubleSpinBoxPISppis->value());
    modelProduto.setData(row, "vPIS", ui->doubleSpinBoxPISvpis->value());

    updateTotais();
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();
}

void CadastrarNFe::on_doubleSpinBoxPISvbc_valueChanged() { calculaPis(); }

void CadastrarNFe::on_doubleSpinBoxPISppis_valueChanged() { calculaPis(); }

void CadastrarNFe::on_doubleSpinBoxPISvpis_valueChanged() { calculaPis(); }

void CadastrarNFe::calculaCofins() {
  const auto list = ui->tableItens->selectionModel()->selectedRows();

  if (list.isEmpty()) { return; }

  unsetConnections();

  try {
    ui->doubleSpinBoxCOFINSvbc->setValue(ui->doubleSpinBoxCOFINSvcofins->value() * 100 / ui->doubleSpinBoxCOFINSpcofins->value());

    const int row = list.first().row();

    modelProduto.setData(row, "vBCCOFINS", ui->doubleSpinBoxCOFINSvbc->value());
    modelProduto.setData(row, "pCOFINS", ui->doubleSpinBoxCOFINSpcofins->value());
    modelProduto.setData(row, "vCOFINS", ui->doubleSpinBoxCOFINSvcofins->value());

    updateTotais();
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();
}

void CadastrarNFe::on_doubleSpinBoxCOFINSvbc_valueChanged() { calculaCofins(); }

void CadastrarNFe::on_doubleSpinBoxCOFINSpcofins_valueChanged() { calculaCofins(); }

void CadastrarNFe::on_doubleSpinBoxCOFINSvcofins_valueChanged() { calculaCofins(); }

void CadastrarNFe::on_itemBoxEnderecoFaturamento_textChanged() {
  SqlQuery queryEndereco;
  queryEndereco.prepare("SELECT logradouro, numero, complemento, bairro, cidade, uf, cep FROM cliente_has_endereco WHERE idEndereco = :idEndereco");
  queryEndereco.bindValue(":idEndereco", ui->itemBoxEnderecoFaturamento->getId());

  if (not queryEndereco.exec()) { throw RuntimeException("Erro lendo endereço do cliente: " + queryEndereco.lastError().text(), this); }

  if (not queryEndereco.first()) { throw RuntimeException("Endereço do cliente não encontrado!"); }

  ui->lineEditDestinatarioLogradouro->setText(queryEndereco.value("logradouro").toString());
  ui->lineEditDestinatarioNumero->setText(queryEndereco.value("numero").toString());
  ui->lineEditDestinatarioComplemento->setText(queryEndereco.value("complemento").toString());
  ui->lineEditDestinatarioBairro->setText(queryEndereco.value("bairro").toString());
  ui->lineEditDestinatarioCidade->setText(queryEndereco.value("cidade").toString());
  ui->lineEditDestinatarioUF->setText(queryEndereco.value("uf").toString());
  ui->lineEditDestinatarioCEP->setText(queryEndereco.value("cep").toString());

  // ------------------------------------------------------------

  const bool mesmaUf = (ui->lineEditEmitenteUF->text() == ui->lineEditDestinatarioUF->text());

  ui->comboBoxDestinoOperacao->setCurrentIndex(mesmaUf ? 0 : 1);

  // ------------------------------------------------------------

  buscarAliquotas();
}

void CadastrarNFe::on_itemBoxEnderecoEntrega_textChanged() {
  SqlQuery queryEndereco;
  queryEndereco.prepare("SELECT logradouro, numero, complemento, bairro, cidade, uf, cep FROM cliente_has_endereco WHERE idEndereco = :idEndereco");
  queryEndereco.bindValue(":idEndereco", ui->itemBoxEnderecoEntrega->getId());

  if (not queryEndereco.exec()) { throw RuntimeException("Erro lendo endereço do cliente: " + queryEndereco.lastError().text(), this); }

  if (not queryEndereco.first()) { throw RuntimeException("Dados não encontrados do endereço com id: " + ui->itemBoxEnderecoEntrega->getId().toString()); }

  ui->lineEditDestinatarioLogradouro_2->setText(queryEndereco.value("logradouro").toString());
  ui->lineEditDestinatarioNumero_2->setText(queryEndereco.value("numero").toString());
  ui->lineEditDestinatarioComplemento_2->setText(queryEndereco.value("complemento").toString());
  ui->lineEditDestinatarioBairro_2->setText(queryEndereco.value("bairro").toString());
  ui->lineEditDestinatarioCidade_2->setText(queryEndereco.value("cidade").toString());
  ui->lineEditDestinatarioUF_2->setText(queryEndereco.value("uf").toString());
  ui->lineEditDestinatarioCEP_2->setText(queryEndereco.value("cep").toString());

  updateComplemento();
}

void CadastrarNFe::on_comboBoxRegime_currentTextChanged(const QString &text) {
  if (text == "Tributação Normal") {
    const QStringList list = {"00 - Tributada integralmente",
                              "10 - Tributada e com cobrança do ICMS por substituição tributária",
                              "20 - Com redução de base de cálculo",
                              "30 - Isenta ou não tributada e com cobrança do ICMS por substituição tributária",
                              "40 - Isenta",
                              "41 - Não tributada",
                              "50 - Suspensão",
                              "51 - Diferimento",
                              "60 - ICMS cobrado anteriormente por substituição tributária",
                              "70 - Com redução de base de cálculo e cobrança do ICMS por substituição tributária",
                              "90 - Outras"};

    ui->comboBoxSituacaoTributaria->clear();
    ui->comboBoxSituacaoTributaria->addItem("");
    ui->comboBoxSituacaoTributaria->addItems(list);
  }

  if (text == "Simples Nacional") {
    const QStringList list = {"101 - Tributada pelo Simples Nacional com permissão de crédito",
                              "102 - Tributada pelo Simples Nacional sem permissão de crédito",
                              "103 - Isenção do ICMS no Simples Nacional para faixa de receita bruta",
                              "201 - Tributada pelo Simples Nacional com permissão de crédito e com cobrança do ICMS por substituição tributária",
                              "202 - Tributada pelo Simples Nacional sem permissão de crédito e com cobrança do ICMS por substituição tributária",
                              "203 - Isenção do ICMS no Simples Nacional para faixa de receita bruta e com cobrança do ICMS por substituição tributária",
                              "300 - Imune",
                              "400 - Não tributada pelo Simples Nacional",
                              "500 - ICMS cobrado anteriormente por substituição tributária (substituído) ou por antecipação",
                              "900 - Outros"};

    ui->comboBoxSituacaoTributaria->clear();
    ui->comboBoxSituacaoTributaria->addItems(list);
  }
}

void CadastrarNFe::on_comboBoxSituacaoTributaria_currentTextChanged(const QString &text) {
  const auto list = ui->tableItens->selectionModel()->selectedRows();

  if (list.isEmpty()) { return; }

  const int row = list.first().row();

  const bool tribNormal = (ui->comboBoxRegime->currentText() == "Tributação Normal");

  QString tipoICMS = (tribNormal ? "ICMS" : "ICMSSN");
  QString cstICMS;

  if (tipoICMS == "ICMS") { cstICMS = text.left(2); }
  if (tipoICMS == "ICMSSN") { cstICMS = text.left(3); }

  tipoICMS += cstICMS;

  modelProduto.setData(row, "tipoICMS", tipoICMS);
  modelProduto.setData(row, "cstICMS", cstICMS);

  // -------------------------------------------------------------------------

  if (text == "00 - Tributada integralmente") {
    ui->frameICMSNormal->show();
    ui->frameST->hide();
  }

  if (text == "10 - Tributada e com cobrança do ICMS por substituição tributária") {}

  if (text == "20 - Com redução de base de cálculo") {}

  if (text == "30 - Isenta ou não tributada e com cobrança do ICMS por substituição tributária") {}

  if (text == "40 - Isenta") {}

  if (text == "41 - Não tributada") {}

  if (text == "50 - Suspensão") {}

  if (text == "51 - Diferimento") {}

  if (text == "60 - ICMS cobrado anteriormente por substituição tributária") {
    ui->frameICMSNormal->hide();
    ui->frameST->show();

    ui->labelPorcMargemIcmsSt->hide();
    ui->doubleSpinBoxICMSpmvast->hide();
    // TODO: icms retido anteriormente, é outro campo?
  }

  if (text == "70 - Com redução de base de cálculo e cobrança do ICMS por substituição tributária") {}

  if (text == "90 - Outras") {}

  // simples nacional

  if (text == "101 - Tributada pelo Simples Nacional com permissão de crédito") {}

  if (text == "102 - Tributada pelo Simples Nacional sem permissão de crédito") {}

  if (text == "103 - Isenção do ICMS no Simples Nacional para faixa de receita bruta") {}

  if (text == "201 - Tributada pelo Simples Nacional com permissão de crédito e com cobrança do ICMS por substituição tributária") {}

  if (text == "202 - Tributada pelo Simples Nacional sem permissão de crédito e com cobrança do ICMS por substituição tributária") {}

  if (text == "203 - Isenção do ICMS no Simples Nacional para faixa de receita bruta e com cobrança do ICMS por substituição tributária") {}

  if (text == "400 - Não tributada pelo Simples Nacional") {}

  if (text == "300 - Imune") {}

  if (text == "500 - ICMS cobrado anteriormente por substituição tributária (substituído) ou por antecipação") {}

  if (text == "900 - Outros") {}
}

void CadastrarNFe::on_comboBoxICMSOrig_currentIndexChanged(const int index) {
  if (index == 0) { return; }

  const auto list = ui->tableItens->selectionModel()->selectedRows();

  if (list.isEmpty()) { return; }

  modelProduto.setData(list.first().row(), "orig", index - 1);
}

void CadastrarNFe::on_comboBoxICMSModBc_currentIndexChanged(const int index) {
  // modBC

  if (index == 0) { return; }

  const auto list = ui->tableItens->selectionModel()->selectedRows();

  if (list.isEmpty()) { return; }

  const int row = list.first().row();

  unsetConnections();

  try {
    modelProduto.setData(row, "modBC", index - 1);

    if (ui->comboBoxICMSModBc->currentText() == "Valor da Operação") {
      ui->doubleSpinBoxICMSvbc->setValue(modelProduto.data(row, "vBCPIS").toDouble());
      modelProduto.setData(row, "vBC", modelProduto.data(row, "vBCPIS").toDouble());
      modelProduto.setData(row, "pICMS", 7);
      calculaIcms();
      // TODO: verificar a aliquota entre estados e setar a porcentagem (caso seja interestadual)
    }
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();
}

void CadastrarNFe::on_comboBoxICMSModBcSt_currentIndexChanged(const int index) {
  // modBCST

  if (index == 0) { return; }

  const auto list = ui->tableItens->selectionModel()->selectedRows();

  if (list.isEmpty()) { return; }

  modelProduto.setData(list.first().row(), "modBCST", index - 1);
}

void CadastrarNFe::on_comboBoxIPIcst_currentTextChanged(const QString &text) {
  const auto list = ui->tableItens->selectionModel()->selectedRows();

  if (list.isEmpty()) { return; }

  modelProduto.setData(list.first().row(), "cstIPI", text.left(2));
}

void CadastrarNFe::on_comboBoxPIScst_currentTextChanged(const QString &text) {
  const auto list = ui->tableItens->selectionModel()->selectedRows();

  if (list.isEmpty()) { return; }

  modelProduto.setData(list.first().row(), "cstPIS", text.left(2));
}

void CadastrarNFe::on_comboBoxCOFINScst_currentTextChanged(const QString &text) {
  const auto list = ui->tableItens->selectionModel()->selectedRows();

  if (list.isEmpty()) { return; }

  modelProduto.setData(list.first().row(), "cstCOFINS", text.left(2));
}

void CadastrarNFe::on_itemBoxVeiculo_textChanged() {
  SqlQuery queryTransp;
  queryTransp.prepare("SELECT t.cnpj, t.razaoSocial, t.inscEstadual, the.logradouro, the.numero, the.complemento, the.bairro, the.cidade, the.uf, thv.placa, thv.ufPlaca, t.antt FROM "
                      "transportadora_has_veiculo thv LEFT JOIN transportadora t ON thv.idTransportadora = t.idTransportadora LEFT JOIN transportadora_has_endereco the ON the.idTransportadora = "
                      "t.idTransportadora WHERE thv.idVeiculo = :idVeiculo");
  queryTransp.bindValue(":idVeiculo", ui->itemBoxVeiculo->getId());

  if (not queryTransp.exec()) { throw RuntimeException("Erro buscando dados da transportadora: " + queryTransp.lastError().text(), this); }

  if (not queryTransp.first()) { throw RuntimeException("Dados não encontrados do veículo com id: " + ui->itemBoxVeiculo->getId().toString()); }

  const QString endereco = queryTransp.value("logradouro").toString() + " - " + queryTransp.value("numero").toString() + " - " + queryTransp.value("complemento").toString() + " - " +
                           queryTransp.value("bairro").toString();

  ui->lineEditTransportadorCpfCnpj->setText(queryTransp.value("cnpj").toString());
  ui->lineEditTransportadorRazaoSocial->setText(queryTransp.value("razaoSocial").toString());
  ui->lineEditTransportadorInscEst->setText(queryTransp.value("inscEstadual").toString());
  ui->lineEditTransportadorEndereco->setText(endereco);
  ui->lineEditTransportadorUf->setText(queryTransp.value("uf").toString());
  ui->lineEditTransportadorMunicipio->setText(queryTransp.value("cidade").toString());

  ui->lineEditTransportadorPlaca->setText(queryTransp.value("placa").toString());
  ui->lineEditTransportadorRntc->setText(queryTransp.value("antt").toString());
  ui->lineEditTransportadorUfPlaca->setText(queryTransp.value("ufPlaca").toString());
}

void CadastrarNFe::on_itemBoxCliente_textChanged() {
  SqlQuery query;
  query.prepare("SELECT nome_razao, pfpj, cpf, cnpj, inscEstadual, tel, telCel FROM cliente WHERE idCliente = :idCliente");
  query.bindValue(":idCliente", ui->itemBoxCliente->getId());

  if (not query.exec()) { throw RuntimeException("Erro buscando dados do cliente: " + query.lastError().text(), this); }

  if (not query.first()) { throw RuntimeException("Dados não encontrados do cliente com id: " + ui->itemBoxCliente->getId().toString()); }

  ui->lineEditDestinatarioNomeRazao->setText(query.value("nome_razao").toString());
  const QString cpfCnpj = (query.value("pfpj").toString() == "PF") ? "cpf" : "cnpj";
  ui->lineEditDestinatarioCPFCNPJ->setText(query.value(cpfCnpj).toString());
  ui->lineEditDestinatarioInscEst->setText(query.value("inscEstadual").toString());
  ui->lineEditDestinatarioTel1->setText(query.value("tel").toString());
  ui->lineEditDestinatarioTel2->setText(query.value("telCel").toString());

  ui->itemBoxEnderecoFaturamento->setFilter("idCliente = " + ui->itemBoxCliente->getId().toString() + " AND desativado = FALSE OR idEndereco = 1");
  ui->itemBoxEnderecoFaturamento->setId(1);

  ui->itemBoxEnderecoEntrega->setFilter("idCliente = " + ui->itemBoxCliente->getId().toString() + " AND desativado = FALSE OR idEndereco = 1");

  ui->itemBoxEnderecoEntrega->setId(1);
}

void CadastrarNFe::validarDados() {
  // TODO: 5recalcular todos os valores dos itens para verificar se os dados batem (usar o 174058 de referencia)

  // -------------------------------------------------------------------------

  // [Emitente]

  if (clearStr(ui->lineEditEmitenteCNPJ->text()).isEmpty()) { throw RuntimeError("CNPJ do emitente vazio!", this); }

  if (ui->lineEditEmitenteNomeRazao->text().isEmpty()) { throw RuntimeError("Razão Social do emitente vazio!", this); }

  if (ui->lineEditEmitenteLogradouro->text().isEmpty()) { throw RuntimeError("Logradouro do emitente vazio!", this); }

  if (ui->lineEditEmitenteBairro->text().isEmpty()) { throw RuntimeError("Bairro do emitente vazio!", this); }

  if (queryIBGEEmit.value("codigo").toString().isEmpty()) { throw RuntimeError("Código do município do emitente vazio!", this); }

  if (ui->lineEditEmitenteUF->text().isEmpty()) { throw RuntimeError("UF do emitente vazio!", this); }

  // [Destinatario]

  if (clearStr(ui->lineEditDestinatarioCPFCNPJ->text()).isEmpty()) { throw RuntimeError("CPF/CNPJ do destinatário vazio!", this); }

  if (ui->lineEditDestinatarioLogradouro->text().isEmpty()) { throw RuntimeError("Logradouro do destinatário vazio!", this); }

  if (ui->lineEditDestinatarioNumero->text().isEmpty()) { throw RuntimeError("Número endereço do destinatário vazio!", this); }

  if (queryIBGEDest.value("codigo").toString().isEmpty()) { throw RuntimeError("Código do município do destinatário vazio!", this); }

  if (ui->lineEditDestinatarioUF->text().isEmpty()) { throw RuntimeError("UF do destinatário vazio!", this); }

  // -------------------------------------------------------------------------

  // [Produto]

  // TODO: se preço for zero mostrar mensagem avisando que se produto for reposição colocar 1 centavo

  for (int row = 0; row < modelProduto.rowCount(); ++row) {
    if (modelProduto.data(row, "cfop").toString().isEmpty()) { throw RuntimeError("Linha " + QString::number(row + 1) + ": CFOP vazio!", this); }
    if (modelProduto.data(row, "ncm").toString().isEmpty()) { throw RuntimeError("Linha " + QString::number(row + 1) + ": NCM vazio!", this); }
    if (modelProduto.data(row, "codComercial").toString().isEmpty()) { throw RuntimeError("Linha " + QString::number(row + 1) + ": Código vazio!", this); }
    if (modelProduto.data(row, "produto").toString().isEmpty()) { throw RuntimeError("Linha " + QString::number(row + 1) + ": Descrição vazio!", this); }
    if (modelProduto.data(row, "un").toString().isEmpty()) { throw RuntimeError("Linha " + QString::number(row + 1) + ": Unidade vazio!", this); }

    if (qFuzzyIsNull(modelProduto.data(row, "quant").toDouble())) { throw RuntimeError("Linha " + QString::number(row + 1) + ": Quantidade = 0!", this); }
    if (qFuzzyIsNull(modelProduto.data(row, "descUnitario").toDouble())) { throw RuntimeError("Linha " + QString::number(row + 1) + ": Preço unitário = R$ 0!", this); }
    if (qFuzzyIsNull(modelProduto.data(row, "total").toDouble())) { throw RuntimeError("Linha " + QString::number(row + 1) + ": Total produto = R$ 0!", this); }
  }
}

void CadastrarNFe::buscarAliquotas() {
  queryPartilhaInter.prepare("SELECT valor FROM icms WHERE origem = :origem AND destino = :destino");
  queryPartilhaInter.bindValue(":origem", ui->lineEditEmitenteUF->text());
  queryPartilhaInter.bindValue(":destino", ui->lineEditDestinatarioUF->text());

  if (not queryPartilhaInter.exec()) { throw RuntimeException("Erro buscando partilha ICMS inter: " + queryPartilhaInter.lastError().text()); }

  if (not queryPartilhaInter.first()) { throw RuntimeException("Partilha ICMS inter não encontrada!"); }

  // -------------------------------------------------------------------------

  queryPartilhaIntra.prepare("SELECT valor FROM icms WHERE origem = :origem AND destino = :destino");
  queryPartilhaIntra.bindValue(":origem", ui->lineEditDestinatarioUF->text());
  queryPartilhaIntra.bindValue(":destino", ui->lineEditDestinatarioUF->text());

  if (not queryPartilhaIntra.exec()) { throw RuntimeException("Erro buscando partilha ICMS intra: " + queryPartilhaIntra.lastError().text()); }

  if (not queryPartilhaIntra.first()) { throw RuntimeException("Partilha ICMS intra não encontrada!"); }
}

void CadastrarNFe::on_comboBoxCfop_currentTextChanged(const QString &text) {
  const auto list = ui->tableItens->selectionModel()->selectedRows();

  if (list.isEmpty()) { return; }

  modelProduto.setData(list.first().row(), "cfop", text.left(4));
}

void CadastrarNFe::on_pushButtonConsultarCadastro_clicked() {
  if (ui->lineEditDestinatarioCPFCNPJ->text().length() == 14) { return qApp->enqueueInformation("SP não faz consulta de CPF!", this); }

  ACBr acbrRemoto;

  const QString resposta = acbrRemoto.enviarComando("NFE.ConsultaCadastro(" + ui->lineEditDestinatarioUF->text() + ", " + ui->lineEditDestinatarioCPFCNPJ->text() + ")");

  if (resposta.contains("CStat=111")) { // Consulta cadastro com uma ocorrência
    QStringList respostaSplit = resposta.split("\r\n", Qt::SkipEmptyParts).filter("IE=");

    if (respostaSplit.empty()) { throw RuntimeException("Não foi encontrado a inscrição estadual na consulta!"); }

    const QString inscricao = respostaSplit.first().remove("IE=");

    if (not inscricao.isEmpty()) {
      ui->lineEditDestinatarioInscEst->setText(inscricao);

      SqlQuery query;
      query.prepare("UPDATE cliente SET inscEstadual = :inscEstadual WHERE idCliente = :idCliente");
      query.bindValue(":inscEstadual", inscricao);
      query.bindValue(":idCliente", modelVenda.data(0, "idCliente"));

      if (not query.exec()) { throw RuntimeException("Erro atualizando Insc. Est.: " + query.lastError().text(), this); }
    }

    return qApp->enqueueInformation("Consulta com sucesso, atualizando dados...", this);
  }

  if (resposta.contains("CStat=259")) { // CNPJ da consulta não cadastrado como contribuinte na UF
    ui->lineEditDestinatarioInscEst->clear();

    SqlQuery query;
    query.prepare("UPDATE cliente SET inscEstadual = NULL WHERE idCliente = :idCliente");
    query.bindValue(":idCliente", modelVenda.data(0, "idCliente"));

    if (not query.exec()) { throw RuntimeException("Erro atualizando Insc. Est.: " + query.lastError().text(), this); }

    return qApp->enqueueInformation("CNPJ da consulta não cadastrado como contribuinte na UF", this);
  }

  throw RuntimeException("Resposta não tratada!", this);
}

void CadastrarNFe::on_doubleSpinBoxValorFrete_valueChanged(const double value) {
  Q_UNUSED(value)
  // TODO: 1refazer rateamento do frete
}

void CadastrarNFe::on_itemBoxLoja_textChanged() {
  // NOTE: o certificado funciona para todas as filiais, não precisa alterar no ACBr

  //  SqlQuery query;
  //  query.prepare("SELECT certificadoSerie, certificadoSenha FROM loja WHERE idLoja = :idLoja AND certificadoSerie IS NOT NULL");
  //  query.bindValue(":idLoja", ui->itemBoxLoja->getId());

  //  if (not query.exec()) { throw RuntimeException("Erro buscando certificado: " + query.lastError().text(), this); }

  //  if (not query.first()) { throw RuntimeError("A loja selecionada não possui certificado cadastrado no sistema!", this); }

  //  ACBr acbrRemoto;

  //  const QString resposta = acbrRemoto.enviarComando("NFE.SetCertificado(" + query.value("certificadoSerie").toString() + "," + query.value("certificadoSenha").toString() + ")");

  //  if (not resposta.contains("OK")) {
  //    ui->itemBoxLoja->clear();
  //    throw RuntimeException("ACBR " + resposta);
  //  }

  preencherNumeroNFe();
  preencherEmitente();
}

void CadastrarNFe::setConnections() {
  if (not blockingSignals.isEmpty()) { blockingSignals.pop(); } // avoid crashing on first setConnections

  if (not blockingSignals.isEmpty()) { return; } // delay setting connections until last unset/set block

  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->checkBoxFrete, &QCheckBox::toggled, this, &CadastrarNFe::on_checkBoxFrete_toggled, connectionType);
  connect(ui->comboBoxCOFINScst, &QComboBox::currentTextChanged, this, &CadastrarNFe::on_comboBoxCOFINScst_currentTextChanged, connectionType);
  connect(ui->comboBoxCfop, &QComboBox::currentTextChanged, this, &CadastrarNFe::on_comboBoxCfop_currentTextChanged, connectionType);
  connect(ui->comboBoxDestinoOperacao, &QComboBox::currentTextChanged, this, &CadastrarNFe::on_comboBoxDestinoOperacao_currentTextChanged, connectionType);
  connect(ui->comboBoxICMSModBc, qOverload<int>(&QComboBox::currentIndexChanged), this, &CadastrarNFe::on_comboBoxICMSModBc_currentIndexChanged, connectionType);
  connect(ui->comboBoxICMSModBcSt, qOverload<int>(&QComboBox::currentIndexChanged), this, &CadastrarNFe::on_comboBoxICMSModBcSt_currentIndexChanged, connectionType);
  connect(ui->comboBoxICMSOrig, qOverload<int>(&QComboBox::currentIndexChanged), this, &CadastrarNFe::on_comboBoxICMSOrig_currentIndexChanged, connectionType);
  connect(ui->comboBoxIPIcst, &QComboBox::currentTextChanged, this, &CadastrarNFe::on_comboBoxIPIcst_currentTextChanged, connectionType);
  connect(ui->comboBoxPIScst, &QComboBox::currentTextChanged, this, &CadastrarNFe::on_comboBoxPIScst_currentTextChanged, connectionType);
  connect(ui->comboBoxRegime, &QComboBox::currentTextChanged, this, &CadastrarNFe::on_comboBoxRegime_currentTextChanged, connectionType);
  connect(ui->comboBoxSituacaoTributaria, &QComboBox::currentTextChanged, this, &CadastrarNFe::on_comboBoxSituacaoTributaria_currentTextChanged, connectionType);
  connect(ui->doubleSpinBoxCOFINSpcofins, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &CadastrarNFe::on_doubleSpinBoxCOFINSpcofins_valueChanged, connectionType);
  connect(ui->doubleSpinBoxCOFINSvbc, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &CadastrarNFe::on_doubleSpinBoxCOFINSvbc_valueChanged, connectionType);
  connect(ui->doubleSpinBoxCOFINSvcofins, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &CadastrarNFe::on_doubleSpinBoxCOFINSvcofins_valueChanged, connectionType);
  connect(ui->doubleSpinBoxICMSpicms, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &CadastrarNFe::on_doubleSpinBoxICMSpicms_valueChanged, connectionType);
  connect(ui->doubleSpinBoxICMSpicmsst, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &CadastrarNFe::on_doubleSpinBoxICMSpicmsst_valueChanged, connectionType);
  connect(ui->doubleSpinBoxICMSvbc, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &CadastrarNFe::on_doubleSpinBoxICMSvbc_valueChanged, connectionType);
  connect(ui->doubleSpinBoxICMSvbcst, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &CadastrarNFe::on_doubleSpinBoxICMSvbcst_valueChanged, connectionType);
  connect(ui->doubleSpinBoxICMSvicms, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &CadastrarNFe::on_doubleSpinBoxICMSvicms_valueChanged, connectionType);
  connect(ui->doubleSpinBoxICMSvicmsst, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &CadastrarNFe::on_doubleSpinBoxICMSvicmsst_valueChanged, connectionType);
  connect(ui->doubleSpinBoxPISppis, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &CadastrarNFe::on_doubleSpinBoxPISppis_valueChanged, connectionType);
  connect(ui->doubleSpinBoxPISvbc, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &CadastrarNFe::on_doubleSpinBoxPISvbc_valueChanged, connectionType);
  connect(ui->doubleSpinBoxPISvpis, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &CadastrarNFe::on_doubleSpinBoxPISvpis_valueChanged, connectionType);
  connect(ui->doubleSpinBoxValorFrete, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &CadastrarNFe::on_doubleSpinBoxValorFrete_valueChanged, connectionType);
  connect(ui->itemBoxCliente, &ItemBox::textChanged, this, &CadastrarNFe::on_itemBoxCliente_textChanged, connectionType);
  connect(ui->itemBoxEnderecoEntrega, &ItemBox::textChanged, this, &CadastrarNFe::on_itemBoxEnderecoEntrega_textChanged, connectionType);
  connect(ui->itemBoxEnderecoFaturamento, &ItemBox::textChanged, this, &CadastrarNFe::on_itemBoxEnderecoFaturamento_textChanged, connectionType);
  connect(ui->itemBoxLoja, &ItemBox::textChanged, this, &CadastrarNFe::on_itemBoxLoja_textChanged, connectionType);
  connect(ui->itemBoxVeiculo, &ItemBox::textChanged, this, &CadastrarNFe::on_itemBoxVeiculo_textChanged, connectionType);
  connect(ui->pushButtonConsultarCadastro, &QPushButton::clicked, this, &CadastrarNFe::on_pushButtonConsultarCadastro_clicked, connectionType);
  connect(ui->pushButtonEnviarNFE, &QPushButton::clicked, this, &CadastrarNFe::on_pushButtonEnviarNFE_clicked, connectionType);
  connect(ui->tableItens, &TableView::clicked, this, &CadastrarNFe::on_tableItens_clicked, connectionType);
  connect(ui->tableItens->model(), &QAbstractItemModel::dataChanged, this, &CadastrarNFe::on_tableItens_dataChanged, connectionType);
}

void CadastrarNFe::unsetConnections() {
  blockingSignals.push(0);

  disconnect(ui->checkBoxFrete, &QCheckBox::toggled, this, &CadastrarNFe::on_checkBoxFrete_toggled);
  disconnect(ui->comboBoxCOFINScst, &QComboBox::currentTextChanged, this, &CadastrarNFe::on_comboBoxCOFINScst_currentTextChanged);
  disconnect(ui->comboBoxCfop, &QComboBox::currentTextChanged, this, &CadastrarNFe::on_comboBoxCfop_currentTextChanged);
  disconnect(ui->comboBoxDestinoOperacao, &QComboBox::currentTextChanged, this, &CadastrarNFe::on_comboBoxDestinoOperacao_currentTextChanged);
  disconnect(ui->comboBoxICMSModBc, qOverload<int>(&QComboBox::currentIndexChanged), this, &CadastrarNFe::on_comboBoxICMSModBc_currentIndexChanged);
  disconnect(ui->comboBoxICMSModBcSt, qOverload<int>(&QComboBox::currentIndexChanged), this, &CadastrarNFe::on_comboBoxICMSModBcSt_currentIndexChanged);
  disconnect(ui->comboBoxICMSOrig, qOverload<int>(&QComboBox::currentIndexChanged), this, &CadastrarNFe::on_comboBoxICMSOrig_currentIndexChanged);
  disconnect(ui->comboBoxIPIcst, &QComboBox::currentTextChanged, this, &CadastrarNFe::on_comboBoxIPIcst_currentTextChanged);
  disconnect(ui->comboBoxPIScst, &QComboBox::currentTextChanged, this, &CadastrarNFe::on_comboBoxPIScst_currentTextChanged);
  disconnect(ui->comboBoxRegime, &QComboBox::currentTextChanged, this, &CadastrarNFe::on_comboBoxRegime_currentTextChanged);
  disconnect(ui->comboBoxSituacaoTributaria, &QComboBox::currentTextChanged, this, &CadastrarNFe::on_comboBoxSituacaoTributaria_currentTextChanged);
  disconnect(ui->doubleSpinBoxCOFINSpcofins, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &CadastrarNFe::on_doubleSpinBoxCOFINSpcofins_valueChanged);
  disconnect(ui->doubleSpinBoxCOFINSvbc, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &CadastrarNFe::on_doubleSpinBoxCOFINSvbc_valueChanged);
  disconnect(ui->doubleSpinBoxCOFINSvcofins, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &CadastrarNFe::on_doubleSpinBoxCOFINSvcofins_valueChanged);
  disconnect(ui->doubleSpinBoxICMSpicms, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &CadastrarNFe::on_doubleSpinBoxICMSpicms_valueChanged);
  disconnect(ui->doubleSpinBoxICMSpicmsst, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &CadastrarNFe::on_doubleSpinBoxICMSpicmsst_valueChanged);
  disconnect(ui->doubleSpinBoxICMSvbc, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &CadastrarNFe::on_doubleSpinBoxICMSvbc_valueChanged);
  disconnect(ui->doubleSpinBoxICMSvbcst, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &CadastrarNFe::on_doubleSpinBoxICMSvbcst_valueChanged);
  disconnect(ui->doubleSpinBoxICMSvicms, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &CadastrarNFe::on_doubleSpinBoxICMSvicms_valueChanged);
  disconnect(ui->doubleSpinBoxICMSvicmsst, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &CadastrarNFe::on_doubleSpinBoxICMSvicmsst_valueChanged);
  disconnect(ui->doubleSpinBoxPISppis, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &CadastrarNFe::on_doubleSpinBoxPISppis_valueChanged);
  disconnect(ui->doubleSpinBoxPISvbc, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &CadastrarNFe::on_doubleSpinBoxPISvbc_valueChanged);
  disconnect(ui->doubleSpinBoxPISvpis, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &CadastrarNFe::on_doubleSpinBoxPISvpis_valueChanged);
  disconnect(ui->doubleSpinBoxValorFrete, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &CadastrarNFe::on_doubleSpinBoxValorFrete_valueChanged);
  disconnect(ui->itemBoxCliente, &ItemBox::textChanged, this, &CadastrarNFe::on_itemBoxCliente_textChanged);
  disconnect(ui->itemBoxEnderecoEntrega, &ItemBox::textChanged, this, &CadastrarNFe::on_itemBoxEnderecoEntrega_textChanged);
  disconnect(ui->itemBoxEnderecoFaturamento, &ItemBox::textChanged, this, &CadastrarNFe::on_itemBoxEnderecoFaturamento_textChanged);
  disconnect(ui->itemBoxLoja, &ItemBox::textChanged, this, &CadastrarNFe::on_itemBoxLoja_textChanged);
  disconnect(ui->itemBoxVeiculo, &ItemBox::textChanged, this, &CadastrarNFe::on_itemBoxVeiculo_textChanged);
  disconnect(ui->pushButtonConsultarCadastro, &QPushButton::clicked, this, &CadastrarNFe::on_pushButtonConsultarCadastro_clicked);
  disconnect(ui->pushButtonEnviarNFE, &QPushButton::clicked, this, &CadastrarNFe::on_pushButtonEnviarNFE_clicked);
  disconnect(ui->tableItens, &TableView::clicked, this, &CadastrarNFe::on_tableItens_clicked);
  disconnect(ui->tableItens->model(), &QAbstractItemModel::dataChanged, this, &CadastrarNFe::on_tableItens_dataChanged);
}

void CadastrarNFe::listarCfop() {
  const bool mesmaUF = (ui->lineEditEmitenteUF->text() == ui->lineEditDestinatarioUF->text());
  const bool entrada = (ui->comboBoxTipo->currentText() == "0 Entrada");

  const QString stringUF = (mesmaUF ? "CFOP_DE" : "CFOP_FE");
  const QString stringEntrada = (entrada ? "cfop_entr" : "cfop_sai");
  const QString query = "SELECT " + stringUF + ", NAT FROM " + stringEntrada + " WHERE " + stringUF + " != ''";

  SqlQuery queryCfop;

  if (not queryCfop.exec(query)) { throw RuntimeException("Erro buscando CFOP: " + queryCfop.lastError().text(), this); }

  ui->comboBoxCfop->clear();

  ui->comboBoxCfop->addItem("");

  while (queryCfop.next()) { ui->comboBoxCfop->addItem(queryCfop.value(stringUF).toString() + " - " + queryCfop.value("NAT").toString()); }
}

void CadastrarNFe::on_comboBoxDestinoOperacao_currentTextChanged(const QString &text) { ui->tabWidget_4->setTabEnabled(4, text == "2 Operação interestadual"); }

void CadastrarNFe::on_checkBoxFrete_toggled(const bool checked) {
  ui->doubleSpinBoxValorFrete->setEnabled(checked);

  const double frete = ui->doubleSpinBoxValorFrete->value();
  const double total = ui->doubleSpinBoxValorNota->value();

  if (checked) { ui->doubleSpinBoxValorNota->setValue(total + frete); }
  if (not checked) { ui->doubleSpinBoxValorNota->setValue(total - frete); }
}

void CadastrarNFe::enviarNFe(ACBr &acbrRemoto, const QString &filePath, const int idNFe) {
  const QString resposta = acbrRemoto.enviarComando("NFE.EnviarNFe(" + filePath + ", 1, 1, 0, 1)"); // lote, assina, imprime, sincrono
  qDebug() << "enviar nfe: " << resposta;

  processarResposta(resposta, filePath, idNFe, acbrRemoto);

  qApp->startTransaction("CadastrarNFe::enviarNFe");

  cadastrar(idNFe);

  qApp->endTransaction();

  if (xml.contains("Uso Denegado")) {
    // TODO: desvincular do pedido para que possa ser feito outra nfe
    throw RuntimeError(resposta.mid(resposta.indexOf("\r\nMsg=") + 6).split("\r\n").first());
  }

  qApp->enqueueInformation("Autorizado o uso da NF-e!", this);
}

void CadastrarNFe::enviarEmail(ACBr &acbrRemoto, const QString &filePath) {
  const QString assunto = "NFe - " + ui->lineEditNumero->text() + " - STACCATO REVESTIMENTOS COMERCIO E REPRESENTACAO LTDA";
  const QString emailContabilidade = User::getSetting("User/emailContabilidade").toString();
  const QString emailLogistica = User::getSetting("User/emailLogistica").toString();

  acbrRemoto.enviarEmail(emailContabilidade, emailLogistica, assunto, filePath);

  // TODO: enviar email separado para cliente
}

void CadastrarNFe::carregarArquivo(ACBr &acbrRemoto, const QString &filePath) {
  // TODO: testar se comando deu ok
  QString resposta = acbrRemoto.enviarComando("NFe.LoadFromFile(" + filePath + ")");

  xml = resposta.remove("OK: ");
}

void CadastrarNFe::preencherDadosNFe() {
  preencherNumeroNFe();

  ui->lineEditModelo->setText("55");
  ui->lineEditSerie->setText("001");
  ui->lineEditEmissao->setText(qApp->serverDate().toString("dd/MM/yy"));
  ui->lineEditSaida->setText(qApp->serverDate().toString("dd/MM/yy"));
  ui->lineEditFormatoPagina->setText("0");

  if (tipo == Tipo::Entrada) {
    ui->comboBoxNatureza->setCurrentIndex(3);
    ui->comboBoxTipo->setCurrentIndex(0);
    ui->comboBoxFinalidade->setCurrentIndex(3);
  }

  if (tipo == Tipo::Saida or tipo == Tipo::Futura or tipo == Tipo::SaidaAposFutura) { ui->comboBoxTipo->setCurrentIndex(1); }
}

void CadastrarNFe::preencherEmitente() {
  // dados do emitente

  SqlQuery queryEmitente;
  queryEmitente.prepare("SELECT razaoSocial, nomeFantasia, cnpj, inscEstadual, tel FROM loja WHERE idLoja = :idLoja");
  queryEmitente.bindValue(":idLoja", ui->itemBoxLoja->getId());

  if (not queryEmitente.exec()) { throw RuntimeException("Erro lendo dados do emitente: " + queryEmitente.lastError().text(), this); }

  if (not queryEmitente.first()) { throw RuntimeException("Dados não encontrados da loja com id: " + ui->itemBoxLoja->getId().toString()); }

  ui->lineEditEmitenteNomeRazao->setText(queryEmitente.value("razaoSocial").toString());
  ui->lineEditEmitenteFantasia->setText(queryEmitente.value("nomeFantasia").toString());
  ui->lineEditEmitenteCNPJ->setText(queryEmitente.value("cnpj").toString());
  ui->lineEditEmitenteInscEstadual->setText(queryEmitente.value("inscEstadual").toString());
  ui->lineEditEmitenteTel1->setText(queryEmitente.value("tel").toString());

  // endereço do emitente

  SqlQuery queryEndereco;
  queryEndereco.prepare("SELECT logradouro, numero, complemento, bairro, cidade, uf, cep FROM loja_has_endereco WHERE idLoja = :idLoja");
  queryEndereco.bindValue(":idLoja", ui->itemBoxLoja->getId());

  if (not queryEndereco.exec()) { throw RuntimeException("Erro lendo endereço do emitente: " + queryEndereco.lastError().text(), this); }

  if (not queryEndereco.first()) { throw RuntimeException("Dados não encontrados da loja com id: " + ui->itemBoxLoja->getId().toString()); }

  ui->lineEditEmitenteLogradouro->setText(queryEndereco.value("logradouro").toString());
  ui->lineEditEmitenteNumero->setText(queryEndereco.value("numero").toString());
  ui->lineEditEmitenteComplemento->setText(queryEndereco.value("complemento").toString());
  ui->lineEditEmitenteBairro->setText(queryEndereco.value("bairro").toString());
  ui->lineEditEmitenteCidade->setText(queryEndereco.value("cidade").toString());
  ui->lineEditEmitenteUF->setText(queryEndereco.value("uf").toString());
  ui->lineEditEmitenteCEP->setText(queryEndereco.value("cep").toString());
}

void CadastrarNFe::preencherDestinatario() {
  //  if (tipo == Tipo::Entrada) { // usar dados da loja
  //    ui->itemBoxCliente->setDisabled(true);

  //    // dados do destinatario

  //    SqlQuery queryDestinatario;
  //    queryDestinatario.prepare("SELECT razaoSocial, cnpj, inscEstadual, tel, tel2 FROM loja WHERE idLoja = :idLoja");
  //    queryDestinatario.bindValue(":idLoja", ui->itemBoxLoja->getId());

  //    if (not queryDestinatario.exec() or not queryDestinatario.first()) { throw RuntimeException("Erro lendo dados do destinatário: " + queryDestinatario.lastError().text(), this); }

  //    ui->lineEditDestinatarioNomeRazao->setText(queryDestinatario.value("razaoSocial").toString());
  //    ui->lineEditDestinatarioCPFCNPJ->setText(queryDestinatario.value("cnpj").toString());
  //    ui->lineEditDestinatarioInscEst->setText(queryDestinatario.value("inscEstadual").toString());
  //    ui->lineEditDestinatarioTel1->setText(queryDestinatario.value("tel").toString());
  //    ui->lineEditDestinatarioTel2->setText(queryDestinatario.value("tel2").toString());

  //    // endereco faturamento

  //    ui->itemBoxEnderecoFaturamento->setDisabled(true);

  //    ui->itemBoxEnderecoFaturamento->setFilter("(idCliente = " + modelVenda.data(0, "idCliente").toString() + " AND desativado = FALSE) OR idEndereco = 1");
  //    ui->itemBoxEnderecoFaturamento->setId(modelVenda.data(0, "idEnderecoFaturamento"));

  //    SqlQuery queryDestinatarioEndereco;
  //    queryDestinatarioEndereco.prepare("SELECT logradouro, numero, complemento, bairro, cidade, uf, cep FROM loja_has_endereco WHERE idLoja = :idLoja");
  //    queryDestinatarioEndereco.bindValue(":idLoja", ui->itemBoxLoja->getId());

  //    if (not queryDestinatarioEndereco.exec() or not queryDestinatarioEndereco.first()) {
  //      throw RuntimeException("Erro lendo endereço do destinatário: " + queryDestinatarioEndereco.lastError().text(), this);
  //    }

  //    ui->lineEditDestinatarioLogradouro->setText(queryDestinatarioEndereco.value("logradouro").toString());
  //    ui->lineEditDestinatarioNumero->setText(queryDestinatarioEndereco.value("numero").toString());
  //    ui->lineEditDestinatarioComplemento->setText(queryDestinatarioEndereco.value("complemento").toString());
  //    ui->lineEditDestinatarioBairro->setText(queryDestinatarioEndereco.value("bairro").toString());
  //    ui->lineEditDestinatarioCidade->setText(queryDestinatarioEndereco.value("cidade").toString());
  //    ui->lineEditDestinatarioUF->setText(queryDestinatarioEndereco.value("uf").toString());
  //    ui->lineEditDestinatarioCEP->setText(queryDestinatarioEndereco.value("cep").toString());

  //    // endereco entrega

  //    ui->itemBoxEnderecoEntrega->setDisabled(true);
  //  }

  if (tipo == Tipo::Entrada or tipo == Tipo::Saida or tipo == Tipo::Futura or tipo == Tipo::SaidaAposFutura) {
    // dados do destinatario

    SqlQuery queryDestinatario;
    queryDestinatario.prepare("SELECT nome_razao, pfpj, cpf, cnpj, inscEstadual, tel, telCel FROM cliente WHERE idCliente = :idCliente");
    queryDestinatario.bindValue(":idCliente", modelVenda.data(0, "idCliente"));

    if (not queryDestinatario.exec()) { throw RuntimeException("Erro lendo dados do destinatário: " + queryDestinatario.lastError().text(), this); }

    if (not queryDestinatario.first()) { throw RuntimeException("Dados não encontrados do cliente com id: " + modelVenda.data(0, "idCliente").toString()); }

    ui->lineEditDestinatarioNomeRazao->setText(queryDestinatario.value("nome_razao").toString());
    ui->lineEditDestinatarioCPFCNPJ->setText(queryDestinatario.value(queryDestinatario.value("pfpj").toString() == "PF" ? "cpf" : "cnpj").toString());
    ui->lineEditDestinatarioInscEst->setText(queryDestinatario.value("inscEstadual").toString());
    ui->lineEditDestinatarioTel1->setText(queryDestinatario.value("tel").toString());
    ui->lineEditDestinatarioTel2->setText(queryDestinatario.value("telCel").toString());

    ui->itemBoxCliente->setId(modelVenda.data(0, "idCliente"));

    // endereco faturamento

    ui->itemBoxEnderecoFaturamento->setFilter("(idCliente = " + modelVenda.data(0, "idCliente").toString() + " AND desativado = FALSE) OR idEndereco = 1");
    ui->itemBoxEnderecoFaturamento->setId(modelVenda.data(0, "idEnderecoFaturamento"));

    SqlQuery queryEndereco;
    queryEndereco.prepare("SELECT cep, logradouro, numero, complemento, bairro, cidade, uf FROM cliente_has_endereco WHERE idEndereco = :idEndereco");
    queryEndereco.bindValue(":idEndereco", modelVenda.data(0, "idEnderecoFaturamento"));

    if (not queryEndereco.exec()) { throw RuntimeException("Erro lendo endereço do destinatário: " + queryEndereco.lastError().text(), this); }

    if (not queryEndereco.first()) { throw RuntimeException("Dados não encontrados do endereço com id: " + modelVenda.data(0, "idEnderecoFaturamento").toString()); }

    ui->lineEditDestinatarioLogradouro->setText(queryEndereco.value("logradouro").toString());
    ui->lineEditDestinatarioNumero->setText(queryEndereco.value("numero").toString());
    ui->lineEditDestinatarioComplemento->setText(queryEndereco.value("complemento").toString());
    ui->lineEditDestinatarioBairro->setText(queryEndereco.value("bairro").toString());
    ui->lineEditDestinatarioCidade->setText(queryEndereco.value("cidade").toString());
    ui->lineEditDestinatarioUF->setText(queryEndereco.value("uf").toString());
    ui->lineEditDestinatarioCEP->setText(queryEndereco.value("cep").toString());

    // endereco entrega

    ui->itemBoxEnderecoEntrega->setFilter("(idCliente = " + modelVenda.data(0, "idCliente").toString() + " AND desativado = FALSE) OR idEndereco = 1");
    ui->itemBoxEnderecoEntrega->setId(modelVenda.data(0, "idEnderecoEntrega"));

    queryEndereco.prepare("SELECT cep, logradouro, numero, complemento, bairro, cidade, uf FROM cliente_has_endereco WHERE idEndereco = :idEndereco");
    queryEndereco.bindValue(":idEndereco", modelVenda.data(0, "idEnderecoEntrega"));

    if (not queryEndereco.exec()) { throw RuntimeException("Erro lendo endereço do destinatário: " + queryEndereco.lastError().text(), this); }

    if (not queryEndereco.first()) { throw RuntimeException("Dados não encontrados do endereço com id: " + modelVenda.data(0, "idEnderecoEntrega").toString()); }

    ui->lineEditDestinatarioLogradouro_2->setText(queryEndereco.value("logradouro").toString());
    ui->lineEditDestinatarioNumero_2->setText(queryEndereco.value("numero").toString());
    ui->lineEditDestinatarioComplemento_2->setText(queryEndereco.value("complemento").toString());
    ui->lineEditDestinatarioBairro_2->setText(queryEndereco.value("bairro").toString());
    ui->lineEditDestinatarioCidade_2->setText(queryEndereco.value("cidade").toString());
    ui->lineEditDestinatarioUF_2->setText(queryEndereco.value("uf").toString());
    ui->lineEditDestinatarioCEP_2->setText(queryEndereco.value("cep").toString());
  }

  // -------------------------------------------------------------------------

  queryIBGEEmit.prepare("SELECT codigo FROM cidade WHERE nome = :cidade AND uf = :uf");
  queryIBGEEmit.bindValue(":cidade", ui->lineEditEmitenteCidade->text());
  queryIBGEEmit.bindValue(":uf", ui->lineEditEmitenteUF->text());

  if (not queryIBGEEmit.exec()) { throw RuntimeException("Erro buscando código do munícipio!", this); }

  if (not queryIBGEEmit.first()) {
    auto *cliente = new CadastroCliente(nullptr); // TODO: why are these nullptr?
    cliente->viewRegisterById(ui->itemBoxCliente->getId());
    cliente->show();
    throw RuntimeError("Não foi encontrado o código do munícipio, verifique se cidade/estado estão cadastrados corretamente!");
  }

  // -------------------------------------------------------------------------

  queryIBGEDest.prepare("SELECT codigo FROM cidade WHERE nome = :cidade AND uf = :uf");
  queryIBGEDest.bindValue(":cidade", ui->lineEditDestinatarioCidade->text());
  queryIBGEDest.bindValue(":uf", ui->lineEditDestinatarioUF->text());

  if (not queryIBGEDest.exec()) { throw RuntimeException("Erro buscando código do munícipio!", this); }

  if (not queryIBGEDest.first()) {
    auto *cliente = new CadastroCliente(nullptr);
    cliente->viewRegisterById(ui->itemBoxCliente->getId());
    cliente->show();
    throw RuntimeError("Não foi encontrado o código do munícipio, verifique se cidade/estado estão cadastrados corretamente!");
  }
}

void CadastrarNFe::preencherTotais() {
  double valorProdutos = 0;

  for (int row = 0; row < modelProduto.rowCount(); ++row) { valorProdutos += modelProduto.data(row, "total").toDouble(); }

  const double frete = modelVenda.data(0, "frete").toDouble();
  const double totalVenda = modelVenda.data(0, "total").toDouble() - frete;
  const double freteProporcional = valorProdutos / totalVenda * frete;

  ui->doubleSpinBoxValorProdutos->setValue(valorProdutos);
  ui->doubleSpinBoxValorFrete->setValue(freteProporcional);
  ui->doubleSpinBoxValorNota->setValue(valorProdutos + freteProporcional);
}

void CadastrarNFe::preencherImpostos() {
  // buscar aliquotas

  const double porcentagemPIS = User::fromLoja("porcentagemPIS").toDouble();

  if (qFuzzyIsNull(porcentagemPIS)) { throw RuntimeException("Erro buscando % PIS!", this); }

  const double porcentagemCOFINS = User::fromLoja("porcentagemCOFINS").toDouble();

  if (qFuzzyIsNull(porcentagemCOFINS)) { throw RuntimeException("Erro buscando % COFINS!", this); }

  // determinar operacao inter/intra

  const bool mesmaUf = (ui->lineEditEmitenteUF->text() == ui->lineEditDestinatarioUF->text());

  ui->comboBoxDestinoOperacao->setCurrentIndex(mesmaUf ? 0 : 1);

  if (tipo == Tipo::Entrada) {
    // usar mesmos dados da NFe de saida

    for (int row = 0; row < modelProduto.rowCount(); ++row) {
      for (int col = 0; col < modelProduto.columnCount(); ++col) {
        if (modelProduto.data(row, col).isNull()) {
          modelProduto.setData(row, col, 0); // limpar campos dos imposto
        }
      }

      // para CFOP de saída 5101/6101 usar CFOP 1201/2201
      // para CFOP de saída 5102/6102 usar CFOP 1202/2202
      // para CFOP de saída 5401/6401 usar CFOP 1410/2410
      // para CFOP de saída 5403/6403 usar CFOP 1411/2411
      // para CFOP de saída 5405/6404 usar CFOP 1411/2411

      modelProduto.setData(row, "cfop", mesmaUf ? "1411" : "2411");

      modelProduto.setData(row, "tipoICMS", "ICMS60");
      modelProduto.setData(row, "cstICMS", "60");

      const double total = modelProduto.data(row, "total").toDouble();
      const double freteProduto = qFuzzyIsNull(total) ? 0 : total / ui->doubleSpinBoxValorProdutos->value() * ui->doubleSpinBoxValorFrete->value();

      modelProduto.setData(row, "cstPIS", "01");
      modelProduto.setData(row, "vBCPIS", total + freteProduto);
      modelProduto.setData(row, "pPIS", porcentagemPIS);
      modelProduto.setData(row, "vPIS", (total + freteProduto) * porcentagemPIS / 100);
      modelProduto.setData(row, "cstCOFINS", "01");
      modelProduto.setData(row, "vBCCOFINS", total + freteProduto);
      modelProduto.setData(row, "pCOFINS", porcentagemCOFINS);
      modelProduto.setData(row, "vCOFINS", (total + freteProduto) * porcentagemCOFINS / 100);
    }
  }

  if (tipo == Tipo::Saida) {
    for (int row = 0; row < modelProduto.rowCount(); ++row) {
      for (int col = 0; col < modelProduto.columnCount(); ++col) {
        if (modelProduto.data(row, col).isNull()) {
          modelProduto.setData(row, col, 0); // limpar campos dos imposto
        }
      }

      modelProduto.setData(row, "cfop", mesmaUf ? "5403" : "6403");

      modelProduto.setData(row, "tipoICMS", "ICMS60");
      modelProduto.setData(row, "cstICMS", "60");

      const double total = modelProduto.data(row, "total").toDouble();
      const double freteProduto = qFuzzyIsNull(total) ? 0 : total / ui->doubleSpinBoxValorProdutos->value() * ui->doubleSpinBoxValorFrete->value();

      modelProduto.setData(row, "cstPIS", "01");
      modelProduto.setData(row, "vBCPIS", total + freteProduto);
      modelProduto.setData(row, "pPIS", porcentagemPIS);
      modelProduto.setData(row, "vPIS", (total + freteProduto) * porcentagemPIS / 100);
      modelProduto.setData(row, "cstCOFINS", "01");
      modelProduto.setData(row, "vBCCOFINS", total + freteProduto);
      modelProduto.setData(row, "pCOFINS", porcentagemCOFINS);
      modelProduto.setData(row, "vCOFINS", (total + freteProduto) * porcentagemCOFINS / 100);
    }
  }

  if (tipo == Tipo::Futura) {
    // https://www.econeteditora.com.br/boletim_icms/bo-icms-pa/pa-12/boletim-11/icms_pa_venda_futura.php
    // não destacar impostos aqui, somente na nota pós-futura

    for (int row = 0; row < modelProduto.rowCount(); ++row) {
      for (int col = modelProduto.fieldIndex("numeroPedido"); col < modelProduto.columnCount(); ++col) {
        modelProduto.setData(row, col, 0); // limpar campos dos imposto
      }

      modelProduto.setData(row, "cfop", mesmaUf ? "5117" : "6117");
    }

    ui->comboBoxNatureza->setCurrentText("VENDA COM PROMESSA DE ENTREGA FUTURA");
  }

  if (tipo == Tipo::SaidaAposFutura) {
    for (int row = 0; row < modelProduto.rowCount(); ++row) {
      for (int col = modelProduto.fieldIndex("numeroPedido"); col < modelProduto.columnCount(); ++col) {
        modelProduto.setData(row, col, 0); // limpar campos dos imposto
      }

      modelProduto.setData(row, "cfop", mesmaUf ? "5922" : "6922");

      modelProduto.setData(row, "tipoICMS", "ICMS60");
      modelProduto.setData(row, "cstICMS", "60");

      const double total = modelProduto.data(row, "total").toDouble();
      const double freteProduto = qFuzzyIsNull(total) ? 0 : total / ui->doubleSpinBoxValorProdutos->value() * ui->doubleSpinBoxValorFrete->value();

      modelProduto.setData(row, "cstPIS", "01");
      modelProduto.setData(row, "vBCPIS", total + freteProduto);
      modelProduto.setData(row, "pPIS", porcentagemPIS);
      modelProduto.setData(row, "vPIS", (total + freteProduto) * porcentagemPIS / 100);
      modelProduto.setData(row, "cstCOFINS", "01");
      modelProduto.setData(row, "vBCCOFINS", total + freteProduto);
      modelProduto.setData(row, "pCOFINS", porcentagemCOFINS);
      modelProduto.setData(row, "vCOFINS", (total + freteProduto) * porcentagemCOFINS / 100);
    }

    ui->comboBoxNatureza->setCurrentText("VENDA ORIGINADA DE ENCOMENDA COM PROMESSA DE ENTREGA FUTURA");
  }
}

void CadastrarNFe::preencherTransportadora() {
  // dados da transportadora

  SqlQuery queryTransp;
  queryTransp.prepare("SELECT t.cnpj, t.razaoSocial, t.inscEstadual, the.logradouro, the.numero, the.complemento, the.bairro, the.cidade, the.uf, thv.placa, thv.ufPlaca, t.antt FROM "
                      "veiculo_has_produto vhp LEFT JOIN transportadora_has_veiculo thv ON vhp.idVeiculo = thv.idVeiculo LEFT JOIN transportadora t ON thv.idTransportadora = t.idTransportadora LEFT "
                      "JOIN transportadora_has_endereco the ON t.idTransportadora = the.idTransportadora WHERE `idVendaProduto2` = :idVendaProduto2");

  const QString coluna = (tipo == Tipo::Entrada) ? "idRelacionado" : "idVendaProduto2";
  queryTransp.bindValue(":idVendaProduto2", modelProduto.data(0, coluna));

  if (not queryTransp.exec()) { throw RuntimeException("Erro buscando dados da transportadora: " + queryTransp.lastError().text(), this); }

  if (not queryTransp.first()) { throw RuntimeException("Dados não encontrados do id: " + modelProduto.data(0, coluna).toString()); }

  if (queryTransp.value("razaoSocial").toString() == "RETIRA") { return; }

  const QString endereco = queryTransp.value("logradouro").toString().isEmpty() ? ""
                                                                                : queryTransp.value("logradouro").toString() + " - " + queryTransp.value("numero").toString() + " - " +
                                                                                      queryTransp.value("complemento").toString() + " - " + queryTransp.value("bairro").toString();

  ui->lineEditTransportadorCpfCnpj->setText(queryTransp.value("cnpj").toString());
  ui->lineEditTransportadorRazaoSocial->setText(queryTransp.value("razaoSocial").toString());
  ui->lineEditTransportadorInscEst->setText(queryTransp.value("inscEstadual").toString());
  ui->lineEditTransportadorEndereco->setText(endereco);
  ui->lineEditTransportadorUf->setText(queryTransp.value("uf").toString());
  ui->lineEditTransportadorMunicipio->setText(queryTransp.value("cidade").toString());

  ui->lineEditTransportadorPlaca->setText(queryTransp.value("placa").toString());
  ui->lineEditTransportadorRntc->setText(queryTransp.value("antt").toString());
  ui->lineEditTransportadorUfPlaca->setText(queryTransp.value("ufPlaca").toString());
}

void CadastrarNFe::preencherVolumes() {
  // somar pesos para os campos do transporte

  double caixas = 0;
  double peso = 0;

  for (int row = 0; row < modelProduto.rowCount(); ++row) {
    caixas += modelProduto.data(row, "caixas").toDouble();
    peso += modelProduto.data(row, "peso").toDouble();
  }

  ui->spinBoxVolumesQuant->setValue(static_cast<int>(caixas));
  ui->lineEditVolumesEspecie->setText("Caixas");
  ui->doubleSpinBoxVolumesPesoBruto->setValue(peso);
  ui->doubleSpinBoxVolumesPesoLiq->setValue(peso);
}

void CadastrarNFe::preencherTransporte() {
  // TODO: verificar na nota futura qual transportadora preencher

  if (tipo != Tipo::Entrada and tipo != Tipo::Saida and tipo != Tipo::SaidaAposFutura) { return; }

  preencherTransportadora();

  preencherVolumes();
}

bool CadastrarNFe::validarRegras(ACBr &acbrRemoto, const QString &filePath) {
  const QString resposta = acbrRemoto.enviarComando("NFE.ValidarNFeRegraNegocios(" + filePath + ")");

  if (not resposta.startsWith("OK:")) {
    QStringList lines = resposta.split("\r\n", Qt::SkipEmptyParts);
    lines.removeFirst();

    QMessageBox msgBox(QMessageBox::Question, "Atenção!", "Validação apresentou os seguintes problemas:\n" + lines.join("\n") + "\n\n\nDeseja continuar mesmo assim?",
                       QMessageBox::Yes | QMessageBox::No, this);
    msgBox.setButtonText(QMessageBox::Yes, "Continuar");
    msgBox.setButtonText(QMessageBox::No, "Voltar");

    if (msgBox.exec() == QMessageBox::No) { return false; }
  }

  return true;
}

// TODO: 5colocar NCM para poder ser alterado na caixinha em baixo
// TODO: 3criar logo para nota
// TODO: 5bloquear edicao direto na tabela
// TODO: os produtos de reposicao devem sair na nota com o valor que foram vendidos originalmente
// TODO: quando mudar a finalidade operacao para devolucao mudar as tabelas de cfop
// TODO: testar a função de pré-gravar nota e consultar
// TODO: [Informações Adicionais de Interesse do Fisco: ICMS RECOLHIDO ANTECIPADAMENTE CONFORME ARTIGO 313Y;] não vai em operações inter e
// precisa detalhar a partilha no complemento bem como origem e destino
// TODO: colocar um botao para imprimir uma previa da DANFE? utilizar o xml antes do envio

// NFe Devolucao

// TODO: falta colocar a observacao da recusa/devolucao do cliente nas informacoes complementares
// http://www.spednews.com.br/icms-como-dar-entrada-de-mercadoria-recusada-pelo-destinatario/
// https://sigaofisco.com.br/icms-como-dar-entrada-de-mercadoria-recusada-pelo-destinatario/
// https://cr.inf.br/blog/manual-como-fazer-nota-de-devolucao/
// https://legislacao.fazenda.sp.gov.br/Paginas/RC20724_2019.aspx

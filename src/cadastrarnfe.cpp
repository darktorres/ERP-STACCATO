#include <QDate>
#include <QDebug>
#include <QFile>
#include <QMessageBox>
#include <QSqlError>

#include "acbr.h"
#include "cadastrarnfe.h"
#include "porcentagemdelegate.h"
#include "reaisdelegate.h"
#include "ui_cadastrarnfe.h"
#include "usersession.h"

CadastrarNFe::CadastrarNFe(const QString &idVenda, const Tipo tipo, QWidget *parent) : Dialog(parent), tipo(tipo), idVenda(idVenda), ui(new Ui::CadastrarNFe) {
  ui->setupUi(this);

  setWindowFlags(Qt::Window);

  setupTables();

  mapper.setModel(&modelViewProdutoEstoque);
  mapper.setSubmitPolicy(QDataWidgetMapper::AutoSubmit);

  mapper.addMapping(ui->doubleSpinBoxICMSvbc, modelViewProdutoEstoque.fieldIndex("vBC"));
  mapper.addMapping(ui->doubleSpinBoxICMSpicms, modelViewProdutoEstoque.fieldIndex("pICMS"));
  mapper.addMapping(ui->doubleSpinBoxICMSvicms, modelViewProdutoEstoque.fieldIndex("vICMS"));
  mapper.addMapping(ui->doubleSpinBoxICMSpmvast, modelViewProdutoEstoque.fieldIndex("pMVAST"));
  mapper.addMapping(ui->doubleSpinBoxICMSvbcst, modelViewProdutoEstoque.fieldIndex("vBCST"));
  mapper.addMapping(ui->doubleSpinBoxICMSpicmsst, modelViewProdutoEstoque.fieldIndex("pICMSST"));
  mapper.addMapping(ui->doubleSpinBoxICMSvicmsst, modelViewProdutoEstoque.fieldIndex("vICMSST"));
  mapper.addMapping(ui->lineEditIPIcEnq, modelViewProdutoEstoque.fieldIndex("cEnq"));
  mapper.addMapping(ui->doubleSpinBoxPISvbc, modelViewProdutoEstoque.fieldIndex("vBCPIS"));
  mapper.addMapping(ui->doubleSpinBoxPISppis, modelViewProdutoEstoque.fieldIndex("pPIS"));
  mapper.addMapping(ui->doubleSpinBoxPISvpis, modelViewProdutoEstoque.fieldIndex("vPIS"));
  mapper.addMapping(ui->doubleSpinBoxCOFINSvbc, modelViewProdutoEstoque.fieldIndex("vBCCOFINS"));
  mapper.addMapping(ui->doubleSpinBoxCOFINSpcofins, modelViewProdutoEstoque.fieldIndex("pCOFINS"));
  mapper.addMapping(ui->doubleSpinBoxCOFINSvcofins, modelViewProdutoEstoque.fieldIndex("vCOFINS"));

  ui->itemBoxLoja->setSearchDialog(SearchDialog::loja(this));

  const auto lojaACBr = UserSession::getSetting("User/lojaACBr");

  if (not lojaACBr) {
    emit errorSignal("A chave 'lojaACBr' não está configurada!");
    return;
  }

  ui->itemBoxLoja->setValue(lojaACBr.value());

  ui->lineEditModelo->setInputMask("99;_");
  ui->lineEditSerie->setInputMask("999;_");
  ui->lineEditCodigo->setInputMask("99999999;_");
  ui->lineEditNumero->setInputMask("999999999;_");
  ui->lineEditFormatoPagina->setInputMask("9;_");

  ui->itemBoxCliente->setSearchDialog(SearchDialog::cliente(this));
  ui->itemBoxEnderecoFaturamento->setSearchDialog(SearchDialog::enderecoCliente(this));
  ui->itemBoxEnderecoEntrega->setSearchDialog(SearchDialog::enderecoCliente(this));
  ui->itemBoxVeiculo->setSearchDialog(SearchDialog::veiculo(this));

  if (idVenda.isEmpty()) { emit errorSignal("idVenda vazio!"); }

  connect(&modelViewProdutoEstoque, &QAbstractItemModel::dataChanged, this, &CadastrarNFe::updateImpostos);
  connect(ui->itemBoxLoja, &ItemBox::textChanged, this, &CadastrarNFe::alterarCertificado);

  ui->frame_2->hide();
}

CadastrarNFe::~CadastrarNFe() { delete ui; }

void CadastrarNFe::setupTables() {
  modelVenda.setTable("venda");
  modelVenda.setEditStrategy(QSqlTableModel::OnManualSubmit);
  modelVenda.setFilter("idVenda = '" + idVenda + "'");

  if (not modelVenda.select()) { return; }

  modelLoja.setTable("loja");
  modelLoja.setEditStrategy(QSqlTableModel::OnManualSubmit);

  const auto lojaACBr = UserSession::getSetting("User/lojaACBr");

  if (not lojaACBr) {
    emit errorSignal("A chave 'lojaACBr' não está configurada!");
    return;
  }

  modelLoja.setFilter("idLoja = " + lojaACBr.value().toString());

  if (not modelLoja.select()) { return; }

  modelViewProdutoEstoque.setTable("view_produto_estoque");
  modelViewProdutoEstoque.setEditStrategy(QSqlTableModel::OnManualSubmit);

  modelViewProdutoEstoque.setHeaderData("fornecedor", "Fornecedor");
  modelViewProdutoEstoque.setHeaderData("produto", "Produto");
  modelViewProdutoEstoque.setHeaderData("obs", "Obs.");
  modelViewProdutoEstoque.setHeaderData("caixas", "Caixas");
  modelViewProdutoEstoque.setHeaderData("descUnitario", "R$ Unit.");
  modelViewProdutoEstoque.setHeaderData("quant", "Quant.");
  modelViewProdutoEstoque.setHeaderData("un", "Un.");
  modelViewProdutoEstoque.setHeaderData("unCaixa", "Un./Caixa");
  modelViewProdutoEstoque.setHeaderData("codComercial", "Cód. Com.");
  modelViewProdutoEstoque.setHeaderData("formComercial", "Form. Com.");
  modelViewProdutoEstoque.setHeaderData("total", "Total");
  modelViewProdutoEstoque.setHeaderData("codBarras", "Cód. Barras");
  modelViewProdutoEstoque.setHeaderData("ncm", "NCM");
  modelViewProdutoEstoque.setHeaderData("cfop", "CFOP");

  ui->tableItens->setModel(&modelViewProdutoEstoque);
  ui->tableItens->setItemDelegateForColumn("descUnitario", new ReaisDelegate(this));
  ui->tableItens->setItemDelegateForColumn("total", new ReaisDelegate(this));
  // TODO: these works only on some lines
  //  ui->tableItens->setItemDelegateForColumn("vBC", new ReaisDelegate(this));
  //  ui->tableItens->setItemDelegateForColumn("pICMS", new PorcentagemDelegate(this));
  //  ui->tableItens->setItemDelegateForColumn("vICMS", new ReaisDelegate(this));
  //  ui->tableItens->setItemDelegateForColumn("pMVAST", new PorcentagemDelegate(this));
  //  ui->tableItens->setItemDelegateForColumn("vBCST", new ReaisDelegate(this));
  //  ui->tableItens->setItemDelegateForColumn("pICMSST", new PorcentagemDelegate(this));
  //  ui->tableItens->setItemDelegateForColumn("vICMSST", new ReaisDelegate(this));
  ui->tableItens->setItemDelegateForColumn("vBCPIS", new ReaisDelegate(this));
  ui->tableItens->setItemDelegateForColumn("pPIS", new PorcentagemDelegate(this));
  ui->tableItens->setItemDelegateForColumn("vPIS", new ReaisDelegate(this));
  ui->tableItens->setItemDelegateForColumn("vBCCOFINS", new ReaisDelegate(this));
  ui->tableItens->setItemDelegateForColumn("pCOFINS", new PorcentagemDelegate(this));
  ui->tableItens->setItemDelegateForColumn("vCOFINS", new ReaisDelegate(this));
  ui->tableItens->hideColumn("idProduto");
  ui->tableItens->hideColumn("idVendaProduto");
  ui->tableItens->hideColumn("numeroPedido");
  ui->tableItens->hideColumn("itemPedido");
}

QString CadastrarNFe::gravarNota() {
  QString nfe;

  QTextStream stream(&nfe);

  stream << R"(NFE.CriarNFe(")" << endl;

  writeIdentificacao(stream);
  writeEmitente(stream);
  writeDestinatario(stream);
  writeProduto(stream);
  writeTotal(stream);
  writeTransportadora(stream);
  // TODO: adicionar pagamentos para NFe 4.0
  //  writePagamento(stream);
  writeVolume(stream);

  const QString infUsuario = ui->infCompUsuario->toPlainText().isEmpty() ? "" : ui->infCompUsuario->toPlainText();
  const QString infComp = (infUsuario.isEmpty() ? "" : infUsuario + ";") + ui->infCompSistema->toPlainText();

  stream << "[DadosAdicionais]" << endl;
  stream << "infCpl = " + infComp << endl;
  stream << R"(",0))" << endl; // dont return xml

  return nfe;
}

std::optional<int> CadastrarNFe::preCadastrarNota(const QString &fileName) {
  QFile file(fileName);

  if (not file.open(QFile::ReadOnly)) {
    emit errorSignal("Erro lendo XML: " + file.errorString());
    return {};
  }

  xml = file.readAll();

  QSqlQuery queryNota;
  queryNota.prepare("INSERT INTO nfe (numeroNFe, tipo, xml, status, chaveAcesso, valor) VALUES (:numeroNFe, :tipo, :xml, :status, :chaveAcesso, :valor)");
  queryNota.bindValue(":numeroNFe", ui->lineEditNumero->text());
  queryNota.bindValue(":tipo", "SAÍDA");
  queryNota.bindValue(":xml", xml);
  queryNota.bindValue(":status", "NOTA PENDENTE");
  queryNota.bindValue(":chaveAcesso", chaveNum);
  queryNota.bindValue(":valor", ui->doubleSpinBoxValorNota->value());

  if (not queryNota.exec()) {
    emit errorSignal("Erro guardando nota: " + queryNota.lastError().text());
    return {};
  }

  const QVariant id = queryNota.lastInsertId();

  if (queryNota.lastInsertId().isNull()) {
    emit errorSignal("Erro lastInsertId");
    return {};
  }

  return id.toInt();
}

bool CadastrarNFe::processarResposta(const QString &resposta, const QString &fileName, const int &idNFe) {
  // erro de comunicacao/rejeicao
  if (not resposta.contains("XMotivo=Autorizado o uso da NF-e")) {
    if (resposta.contains("Rejeição:")) {
      QSqlQuery queryNota;
      queryNota.prepare("DELETE FROM nfe WHERE idNFe = :idNFe");
      queryNota.bindValue(":idNFe", idNFe);

      if (not queryNota.exec()) {
        emit errorSignal("Erro removendo nota: " + queryNota.lastError().text());
        return false;
      }

      emit errorSignal("Resposta EnviarNFe: " + resposta);
      return false;
    }

    const auto respostaConsultar = ACBr::enviarComando("NFE.ConsultarNFe(" + fileName + ")");

    if (not respostaConsultar) { return false; }

    // erro de comunicacao/rejeicao
    if (not respostaConsultar->contains("XMotivo=Autorizado o uso da NF-e")) {
      QSqlQuery queryNota;
      queryNota.prepare("DELETE FROM nfe WHERE idNFe = :idNFe");
      queryNota.bindValue(":idNFe", idNFe);

      if (not queryNota.exec()) {
        emit errorSignal("Erro removendo nota: " + queryNota.lastError().text());
        return false;
      }

      emit errorSignal("Resposta ConsultarNFe: " + *respostaConsultar);
      return false;
    }
  }

  // reread the file now authorized
  if (resposta.contains("XMotivo=Autorizado o uso da NF-e")) {
    QFile file(fileName);

    if (not file.open(QFile::ReadOnly)) {
      emit errorSignal("Erro lendo XML: " + file.errorString());
      return false;
    }

    xml = file.readAll();
  }

  return true;
}

bool CadastrarNFe::verificarConfiguracaoEmail() {
  const auto emailContabilidade = UserSession::getSetting("User/emailContabilidade");

  if (not emailContabilidade) {
    emit errorSignal("A chave 'emailContabilidade' não está configurada!");
    return false;
  }

  const auto emailLogistica = UserSession::getSetting("User/emailLogistica");

  if (not emailLogistica) {
    emit errorSignal("A chave 'emailLogistica' não está configurada!");
    return false;
  }

  return true;
}

void CadastrarNFe::on_pushButtonEnviarNFE_clicked() {
  // TODO: 1ao clicar em enviar abrir um dialog mostrando as informacoes base para o usuario confirmar
  // TODO: colocar um qprogressdialog
  // TODO: quando um campo é maior que o permitido o ACBr retorna 'OK' e na linha seguinte 'Alertas:'
  // TODO: quando clicar em enviar dar um submitAll no model para salvar os dados preenchidos?

  // se os emails nao estiverem configurados avisar antes de gerar a nota
  const auto emailContabilidade = UserSession::getSetting("User/emailContabilidade");

  if (not emailContabilidade) {
    emit errorSignal("A chave 'emailContabilidade' não está configurada!");
    return;
  }

  const auto emailLogistica = UserSession::getSetting("User/emailLogistica");

  if (not emailLogistica) {
    emit errorSignal("A chave 'emailLogistica' não está configurada!");
    return;
  }

  //
  //  if (not verificarConfiguracaoEmail()) { return; }

  if (not validar()) { return; }

  if (not criarChaveAcesso()) { return; }

  const auto resposta = ACBr::enviarComando(gravarNota());

  if (not resposta) { return; }

  if (not resposta->contains("OK")) {
    emit errorSignal(*resposta);
    return;
  }

  const QString filePath = QString(*resposta).remove("OK: ").remove("\r").remove("\n");

  const auto idNFe = preCadastrarNota(filePath);

  if (not idNFe) { return; }

  const auto resposta2 = ACBr::enviarComando("NFE.EnviarNFe(" + filePath + ", 1, 1, 0, 1)");

  if (not resposta2) { return; }

  if (not processarResposta(*resposta2, filePath, *idNFe)) { return; }

  emit transactionStarted();

  if (not QSqlQuery("SET SESSION TRANSACTION ISOLATION LEVEL SERIALIZABLE").exec()) { return; }
  if (not QSqlQuery("START TRANSACTION").exec()) { return; }

  if (not cadastrar(*idNFe)) {
    QSqlQuery("ROLLBACK").exec();
    emit transactionEnded();
    return;
  }

  if (not QSqlQuery("COMMIT").exec()) { return; }

  emit transactionEnded();

  emit informationSignal(*resposta2);

  const QString assunto = "NFe - " + ui->lineEditNumero->text() + " - STACCATO REVESTIMENTOS COMERCIO E REPRESENTACAO LTDA";

  // TODO: enviar email separado para cliente
  if (not ACBr::enviarEmail(emailContabilidade.value().toString(), emailLogistica.value().toString(), assunto, filePath)) { return; }

  if (not ACBr::gerarDanfe(xml.toLatin1())) { return; }

  close();
}

bool CadastrarNFe::cadastrar(const int &idNFe) {
  QSqlQuery queryNFe;
  queryNFe.prepare("UPDATE nfe SET status = 'AUTORIZADO', xml = :xml WHERE idNFe = :idNFe");
  queryNFe.bindValue(":xml", xml);
  queryNFe.bindValue(":idNFe", idNFe);

  if (not queryNFe.exec()) {
    emit errorSignal("Erro marcando nota como 'AUTORIZADO': " + queryNFe.lastError().text());
    return false;
  }

  if (tipo == Tipo::Futura) {
    QSqlQuery query;
    query.prepare("UPDATE venda_has_produto SET idNFeFutura = :idNFeFutura WHERE idVendaProduto = :idVendaProduto");

    for (int row = 0; row < modelViewProdutoEstoque.rowCount(); ++row) {
      query.bindValue(":idNFeFutura", idNFe);
      query.bindValue(":idVendaProduto", modelViewProdutoEstoque.data(row, "idVendaProduto"));

      if (not query.exec()) {
        emit errorSignal("Erro salvando NFe nos produtos: " + query.lastError().text());
        return false;
      }
    }
  }

  if (tipo == Tipo::Normal) {
    // REFAC: use 'idVendaProduto IN'?
    QSqlQuery query1;
    query1.prepare("UPDATE pedido_fornecedor_has_produto SET status = 'EM ENTREGA' WHERE idVendaProduto = :idVendaProduto");

    QSqlQuery query2;
    query2.prepare("UPDATE venda_has_produto SET status = 'EM ENTREGA', idNFeSaida = :idNFeSaida WHERE idVendaProduto = :idVendaProduto");

    QSqlQuery query3;
    query3.prepare("UPDATE veiculo_has_produto SET status = 'EM ENTREGA', idNFeSaida = :idNFeSaida WHERE idVendaProduto = :idVendaProduto");

    for (int row = 0; row < modelViewProdutoEstoque.rowCount(); ++row) {
      query1.bindValue(":idVendaProduto", modelViewProdutoEstoque.data(row, "idVendaProduto"));

      if (not query1.exec()) {
        emit errorSignal("Erro atualizando status do pedido_fornecedor: " + query1.lastError().text());
        return false;
      }

      query2.bindValue(":idNFeSaida", idNFe);
      query2.bindValue(":idVendaProduto", modelViewProdutoEstoque.data(row, "idVendaProduto"));

      if (not query2.exec()) {
        emit errorSignal("Erro salvando NFe nos produtos: " + query2.lastError().text());
        return false;
      }

      query3.bindValue(":idVendaProduto", modelViewProdutoEstoque.data(row, "idVendaProduto"));
      query3.bindValue(":idNFeSaida", idNFe);

      if (not query3.exec()) {
        emit errorSignal("Erro atualizando carga veiculo: " + query3.lastError().text());
        return false;
      }
    }
  }

  return true;
}

void CadastrarNFe::updateImpostos() {
  // TODO: receber como parametro a coluna alterada, se for por exemplo valorCOFINS deve fazer o calculo reverso da base de calculo
  //  qDebug() << "a";
  // TODO: readd IPI?
  double baseICMS = 0;
  double valorICMS = 0;
  //  double valorIPI = 0;
  double valorPIS = 0;
  double valorCOFINS = 0;

  for (int row = 0; row < modelViewProdutoEstoque.rowCount(); ++row) {
    baseICMS += modelViewProdutoEstoque.data(row, "vBC").toDouble();
    valorICMS += modelViewProdutoEstoque.data(row, "vICMS").toDouble();
    //    valorIPI += modelProd.data(row, "vIPI").toDouble();
    valorPIS += modelViewProdutoEstoque.data(row, "vPIS").toDouble();
    valorCOFINS += modelViewProdutoEstoque.data(row, "vCOFINS").toDouble();
  }

  const double total = valorICMS + /*valorIPI +*/ valorPIS + valorCOFINS;

  ui->doubleSpinBoxBaseICMS->setValue(baseICMS);
  ui->doubleSpinBoxValorICMS->setValue(valorICMS);
  //  ui->doubleSpinBoxValorIPI->setValue(valorIPI);
  ui->doubleSpinBoxValorPIS->setValue(valorPIS);
  ui->doubleSpinBoxValorCOFINS->setValue(valorCOFINS);

  const QString endereco = ui->itemBoxEnderecoEntrega->getValue() == 1
                               ? "Não há/Retira"
                               : ui->lineEditDestinatarioLogradouro_2->text() + ", " + ui->lineEditDestinatarioNumero_2->text() + " " + ui->lineEditDestinatarioComplemento_2->text() + " - " +
                                     ui->lineEditDestinatarioBairro_2->text() + " - " + ui->lineEditDestinatarioCidade_2->text() + " - " + ui->lineEditDestinatarioUF_2->text() + " - " +
                                     ui->lineEditDestinatarioCEP_2->text();

  const QString texto = "Venda de código " + modelVenda.data(0, "idVenda").toString() + ";END. ENTREGA: " + endereco +
                        ";Informações Adicionais de Interesse do Fisco: ICMS RECOLHIDO ANTECIPADAMENTE CONFORME ARTIGO 313Y;Total Aproximado de tributos federais, estaduais e municipais: R$ " +
                        QLocale(QLocale::Portuguese).toString(total);

  ui->infCompSistema->setPlainText(texto);
}

bool CadastrarNFe::preencherNumeroNFe() {
  if (ui->itemBoxLoja->text().isEmpty()) { return true; }

  QSqlQuery queryCnpj;
  queryCnpj.prepare("SELECT cnpj FROM loja WHERE idLoja = :idLoja");
  queryCnpj.bindValue(":idLoja", ui->itemBoxLoja->getValue());

  if (not queryCnpj.exec() or not queryCnpj.first()) {
    emit errorSignal("Erro buscando CNPJ: " + queryCnpj.lastError().text());
    return false;
  }

  const QString cnpj = queryCnpj.value("cnpj").toString().remove(".").remove("/").remove("-");

  QSqlQuery queryNfe;

  if (not queryNfe.exec("SELECT COALESCE(MAX(numeroNFe), 0) + 1 AS numeroNFe FROM nfe WHERE tipo = 'SAÍDA' AND status = 'AUTORIZADO' AND mid(chaveAcesso, 7, 14) = '" + cnpj +
                        "' AND created BETWEEN DATE_ADD(CURDATE(), INTERVAL - 30 DAY) AND DATE_ADD(CURDATE(), INTERVAL 1 DAY)") or
      not queryNfe.first()) {
    emit errorSignal("Erro buscando idNFe: " + queryNfe.lastError().text());
    return false;
  }

  const int numeroNFe = queryNfe.value("numeroNFe").toInt();

  ui->lineEditNumero->setText(QString("%1").arg(numeroNFe, 9, 10, QChar('0')));
  ui->lineEditCodigo->setText(QString("%1").arg(numeroNFe, 8, 10, QChar('0')));

  return true;
}

void CadastrarNFe::prepararNFe(const QList<int> &items) {
  QString filter;

  for (const auto &item : items) {
    filter += QString(filter.isEmpty() ? "" : " OR ") + "idVendaProduto = " + QString::number(item);

    QSqlQuery query;

    if (not query.exec("SELECT NULL FROM estoque_has_consumo WHERE idVendaProduto = " + QString::number(item)) or not query.first()) {
      emit errorSignal("Erro buscando idVendaProduto " + QString::number(item));
      return;
    }
  }

  modelViewProdutoEstoque.setFilter(filter);

  if (not modelViewProdutoEstoque.select()) { return; }

  ui->tableItens->resizeColumnsToContents();

  preencherNumeroNFe();

  ui->lineEditModelo->setText("55");
  ui->lineEditSerie->setText("001");
  ui->lineEditEmissao->setText(QDate::currentDate().toString("dd/MM/yy"));
  ui->lineEditSaida->setText(QDate::currentDate().toString("dd/MM/yy"));
  ui->comboBoxTipo->setCurrentIndex(1);
  ui->lineEditFormatoPagina->setText("0");

  //-----------------------

  QSqlQuery queryEmitente;
  queryEmitente.prepare("SELECT razaoSocial, nomeFantasia, cnpj, inscEstadual, tel, tel2 FROM loja WHERE idLoja = :idLoja");
  queryEmitente.bindValue(":idLoja", ui->itemBoxLoja->getValue());

  if (not queryEmitente.exec() or not queryEmitente.first()) {
    emit errorSignal("Erro lendo dados do emitente: " + queryEmitente.lastError().text());
    return;
  }

  ui->lineEditEmitenteNomeRazao->setText(queryEmitente.value("razaoSocial").toString());
  ui->lineEditEmitenteFantasia->setText(queryEmitente.value("nomeFantasia").toString());
  ui->lineEditEmitenteCNPJ->setText(queryEmitente.value("cnpj").toString());
  ui->lineEditEmitenteInscEstadual->setText(queryEmitente.value("inscEstadual").toString());
  ui->lineEditEmitenteTel1->setText(queryEmitente.value("tel").toString());
  ui->lineEditEmitenteTel2->setText(queryEmitente.value("tel2").toString());

  QSqlQuery queryEmitenteEndereco;
  queryEmitenteEndereco.prepare("SELECT logradouro, numero, complemento, bairro, cidade, uf, cep FROM loja_has_endereco WHERE idLoja = :idLoja");
  queryEmitenteEndereco.bindValue(":idLoja", ui->itemBoxLoja->getValue());

  if (not queryEmitenteEndereco.exec() or not queryEmitenteEndereco.first()) {
    emit errorSignal("Erro lendo endereço do emitente: " + queryEmitenteEndereco.lastError().text());
    return;
  }

  ui->lineEditEmitenteLogradouro->setText(queryEmitenteEndereco.value("logradouro").toString());
  ui->lineEditEmitenteNumero->setText(queryEmitenteEndereco.value("numero").toString());
  ui->lineEditEmitenteComplemento->setText(queryEmitenteEndereco.value("complemento").toString());
  ui->lineEditEmitenteBairro->setText(queryEmitenteEndereco.value("bairro").toString());
  ui->lineEditEmitenteCidade->setText(queryEmitenteEndereco.value("cidade").toString());
  ui->lineEditEmitenteUF->setText(queryEmitenteEndereco.value("uf").toString());
  ui->lineEditEmitenteCEP->setText(queryEmitenteEndereco.value("cep").toString());

  //-----------------------

  QSqlQuery queryDestinatario;
  queryDestinatario.prepare("SELECT nome_razao, pfpj, cpf, cnpj, inscEstadual, tel, telCel FROM cliente WHERE idCliente = :idCliente");
  queryDestinatario.bindValue(":idCliente", modelVenda.data(0, "idCliente"));

  if (not queryDestinatario.exec() or not queryDestinatario.first()) {
    emit errorSignal("Erro lendo dados do cliente: " + queryDestinatario.lastError().text());
    return;
  }

  ui->lineEditDestinatarioNomeRazao->setText(queryDestinatario.value("nome_razao").toString());
  ui->lineEditDestinatarioCPFCNPJ->setText(queryDestinatario.value(queryDestinatario.value("pfpj").toString() == "PF" ? "cpf" : "cnpj").toString());
  ui->lineEditDestinatarioInscEst->setText(queryDestinatario.value("inscEstadual").toString());
  ui->lineEditDestinatarioTel1->setText(queryDestinatario.value("tel").toString());
  ui->lineEditDestinatarioTel2->setText(queryDestinatario.value("telCel").toString());

  ui->itemBoxCliente->setValue(modelVenda.data(0, "idCliente"));

  // endereco faturamento

  ui->itemBoxEnderecoFaturamento->setFilter("idCliente = " + modelVenda.data(0, "idCliente").toString() + " AND desativado = FALSE OR idEndereco = 1");
  ui->itemBoxEnderecoFaturamento->setValue(modelVenda.data(0, "idEnderecoFaturamento"));

  QSqlQuery queryDestinatarioEndereco;
  queryDestinatarioEndereco.prepare("SELECT cep, logradouro, numero, complemento, bairro, cidade, uf FROM cliente_has_endereco WHERE idEndereco = :idEndereco");
  queryDestinatarioEndereco.bindValue(":idEndereco", modelVenda.data(0, "idEnderecoFaturamento"));

  if (not queryDestinatarioEndereco.exec() or not queryDestinatarioEndereco.first()) {
    emit errorSignal("Erro lendo endereço do cliente: " + queryDestinatarioEndereco.lastError().text());
    return;
  }

  ui->lineEditDestinatarioLogradouro->setText(queryDestinatarioEndereco.value("logradouro").toString());
  ui->lineEditDestinatarioNumero->setText(queryDestinatarioEndereco.value("numero").toString());
  ui->lineEditDestinatarioComplemento->setText(queryDestinatarioEndereco.value("complemento").toString());
  ui->lineEditDestinatarioBairro->setText(queryDestinatarioEndereco.value("bairro").toString());
  ui->lineEditDestinatarioCidade->setText(queryDestinatarioEndereco.value("cidade").toString());
  ui->lineEditDestinatarioUF->setText(queryDestinatarioEndereco.value("uf").toString());
  ui->lineEditDestinatarioCEP->setText(queryDestinatarioEndereco.value("cep").toString());

  // endereco entrega

  ui->itemBoxEnderecoEntrega->setFilter("idCliente = " + modelVenda.data(0, "idCliente").toString() + " AND desativado = FALSE OR idEndereco = 1");
  ui->itemBoxEnderecoEntrega->setValue(modelVenda.data(0, "idEnderecoEntrega"));

  queryDestinatarioEndereco.prepare("SELECT cep, logradouro, numero, complemento, bairro, cidade, uf FROM cliente_has_endereco WHERE idEndereco = :idEndereco");
  queryDestinatarioEndereco.bindValue(":idEndereco", modelVenda.data(0, "idEnderecoEntrega"));

  if (not queryDestinatarioEndereco.exec() or not queryDestinatarioEndereco.first()) {
    emit errorSignal("Erro lendo endereço do cliente: " + queryDestinatarioEndereco.lastError().text());
    return;
  }

  ui->lineEditDestinatarioLogradouro_2->setText(queryDestinatarioEndereco.value("logradouro").toString());
  ui->lineEditDestinatarioNumero_2->setText(queryDestinatarioEndereco.value("numero").toString());
  ui->lineEditDestinatarioComplemento_2->setText(queryDestinatarioEndereco.value("complemento").toString());
  ui->lineEditDestinatarioBairro_2->setText(queryDestinatarioEndereco.value("bairro").toString());
  ui->lineEditDestinatarioCidade_2->setText(queryDestinatarioEndereco.value("cidade").toString());
  ui->lineEditDestinatarioUF_2->setText(queryDestinatarioEndereco.value("uf").toString());
  ui->lineEditDestinatarioCEP_2->setText(queryDestinatarioEndereco.value("cep").toString());

  //-----------------------

  double valorProdutos = 0;

  for (int row = 0; row < modelViewProdutoEstoque.rowCount(); ++row) { valorProdutos += modelViewProdutoEstoque.data(row, "total").toDouble(); }

  const double frete = modelVenda.data(0, "frete").toDouble();
  const double totalVenda = modelVenda.data(0, "total").toDouble() - frete;
  const double freteProporcional = valorProdutos / totalVenda * frete;

  ui->doubleSpinBoxValorProdutos->setValue(valorProdutos);
  ui->doubleSpinBoxValorFrete->setValue(freteProporcional);
  ui->doubleSpinBoxValorNota->setValue(valorProdutos + freteProporcional);

  //------------------------

  const auto porcentagemPIS = UserSession::fromLoja("porcentagemPIS");

  if (not porcentagemPIS) {
    emit errorSignal("Erro buscando % PIS!");
    return;
  }

  const auto porcentagemCOFINS = UserSession::fromLoja("porcentagemCOFINS");

  if (not porcentagemCOFINS) {
    emit errorSignal("Erro buscando % COFINS!");
    return;
  }

  for (int row = 0; row < modelViewProdutoEstoque.rowCount(); ++row) {
    for (int col = modelViewProdutoEstoque.fieldIndex("numeroPedido"); col < modelViewProdutoEstoque.columnCount(); ++col) {
      if (not modelViewProdutoEstoque.setData(row, col, 0)) { return; } // limpar campos dos imposto
    }

    if (not modelViewProdutoEstoque.setData(row, "cfop", "5403")) { return; }

    if (not modelViewProdutoEstoque.setData(row, "tipoICMS", "ICMS60")) { return; }
    if (not modelViewProdutoEstoque.setData(row, "cstICMS", "60")) { return; }

    const double total = modelViewProdutoEstoque.data(row, "total").toDouble();
    const double freteProduto = qFuzzyIsNull(total) ? 0 : total / ui->doubleSpinBoxValorProdutos->value() * ui->doubleSpinBoxValorFrete->value();

    if (not modelViewProdutoEstoque.setData(row, "vBCPIS", total + freteProduto)) { return; }
    if (not modelViewProdutoEstoque.setData(row, "cstPIS", "01")) { return; }
    if (not modelViewProdutoEstoque.setData(row, "pPIS", porcentagemPIS.value().toDouble())) { return; }
    if (not modelViewProdutoEstoque.setData(row, "vPIS", modelViewProdutoEstoque.data(row, "vBCPIS").toDouble() * modelViewProdutoEstoque.data(row, "pPIS").toDouble() / 100)) { return; }
    if (not modelViewProdutoEstoque.setData(row, "vBCCOFINS", total + freteProduto)) { return; }
    if (not modelViewProdutoEstoque.setData(row, "cstCOFINS", "01")) { return; }
    if (not modelViewProdutoEstoque.setData(row, "pCOFINS", porcentagemCOFINS.value().toDouble())) { return; }
    if (not modelViewProdutoEstoque.setData(row, "vCOFINS", modelViewProdutoEstoque.data(row, "vBCCOFINS").toDouble() * modelViewProdutoEstoque.data(row, "pCOFINS").toDouble() / 100)) { return; }
  }

  //

  // TODO: verificar na nota futura qual transportadora preencher
  if (tipo == Tipo::Normal) {
    QSqlQuery queryTransp;
    queryTransp.prepare(
        "SELECT t.cnpj, t.razaoSocial, t.inscEstadual, the.logradouro, the.numero, the.complemento, the.bairro, the.cidade, the.uf, thv.placa, thv.ufPlaca, t.antt FROM "
        "veiculo_has_produto vhp LEFT JOIN transportadora_has_veiculo thv ON vhp.idVeiculo = thv.idVeiculo LEFT JOIN transportadora t ON thv.idTransportadora = t.idTransportadora LEFT "
        "JOIN transportadora_has_endereco the ON t.idTransportadora = the.idTransportadora WHERE idVendaProduto = :idVendaProduto");
    queryTransp.bindValue(":idVendaProduto", items.first());

    if (not queryTransp.exec() or not queryTransp.first()) {
      emit errorSignal("Erro buscando dados da transportadora: " + queryTransp.lastError().text());
      return;
    }

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

    // somar pesos para os campos do transporte
    double caixas = 0;
    double peso = 0;

    QSqlQuery queryProduto;
    queryProduto.prepare("SELECT kgcx FROM produto WHERE idProduto = :idProduto");

    for (int row = 0; row < modelViewProdutoEstoque.rowCount(); ++row) {
      caixas += modelViewProdutoEstoque.data(row, "caixas").toDouble();

      queryProduto.bindValue(":idProduto", modelViewProdutoEstoque.data(row, "idProduto"));

      if (not queryProduto.exec() or not queryProduto.first()) {
        emit errorSignal("Erro buscando peso do produto: " + queryProduto.lastError().text());
        return;
      }

      peso += queryProduto.value("kgcx").toDouble() * modelViewProdutoEstoque.data(row, "caixas").toInt();
    }

    ui->spinBoxVolumesQuant->setValue(static_cast<int>(caixas));
    ui->lineEditVolumesEspecie->setText("Caixas");
    ui->doubleSpinBoxVolumesPesoBruto->setValue(peso);
    ui->doubleSpinBoxVolumesPesoLiq->setValue(peso);
  }

  // CFOP

  if (not listarCfop()) { return; }

  //

  // TODO: place in the right place

  //  ui->tabWidget_4->setTabEnabled(4, false); // hide icms interestadual

  if (not ui->lineEditDestinatarioUF_2->text().isEmpty()) {
    if (ui->lineEditDestinatarioUF_2->text() != ui->lineEditEmitenteUF->text()) { ui->comboBoxDestinoOperacao->setCurrentIndex(1); }
  }

  if (not validar()) { return; }

  //

  updateImpostos();

  setConnections();

  ui->comboBoxRegime->setCurrentText("Tributação Normal");
}

bool CadastrarNFe::criarChaveAcesso() {
  const QStringList listChave = {modelLoja.data(0, "codUF").toString(),
                                 QDate::currentDate().toString("yyMM"),
                                 clearStr(ui->lineEditEmitenteCNPJ->text()),
                                 ui->lineEditModelo->text(),
                                 ui->lineEditSerie->text(),
                                 ui->lineEditNumero->text(),
                                 "1",
                                 ui->lineEditCodigo->text()};

  chaveNum = listChave.join("");

  return calculaDigitoVerificador(chaveNum);
}

QString CadastrarNFe::clearStr(const QString &str) const { return QString(str).remove(".").remove("/").remove("-").remove(" ").remove("(").remove(")"); }

bool CadastrarNFe::calculaDigitoVerificador(QString &chave) {
  if (chave.size() != 43) {
    emit errorSignal("Erro no tamanho da chave: " + chave);
    return false;
  }

  int soma = 0;
  int mult = 4;

  for (const auto &i : std::as_const(chave)) {
    soma += i.digitValue() * mult--;
    mult = mult == 1 ? 9 : mult;
  }

  const int resto = soma % 11;

  chave += QString::number(resto < 2 ? 0 : 11 - resto);

  return true;
}

void CadastrarNFe::writeIdentificacao(QTextStream &stream) const {
  stream << "[Identificacao]" << endl;
  stream << "NaturezaOperacao = " + ui->comboBoxNatureza->currentText() << endl;
  stream << "Modelo = " + ui->lineEditModelo->text() << endl;
  stream << "Serie = " + ui->lineEditSerie->text() << endl;
  stream << "Codigo = " << ui->lineEditCodigo->text() << endl; // 1
  stream << "Numero = " << ui->lineEditNumero->text() << endl; // 1
  stream << "Emissao = " + QDate::currentDate().toString("dd/MM/yyyy") << endl;
  stream << "Saida = " + QDate::currentDate().toString("dd/MM/yyyy") << endl;
  stream << "Tipo = " + ui->comboBoxTipo->currentText().left(1) << endl;
  stream << "finNFe = " + ui->comboBoxFinalidade->currentText().left(1) << endl;
  stream << "FormaPag = " + ui->lineEditFormatoPagina->text() << endl;
  stream << "idDest = " + ui->comboBoxDestinoOperacao->currentText().left(1) << endl;
  stream << "indPres = 1" << endl;
  stream << "indFinal = 1" << endl;
  // TODO: fix
  //  stream << "cMunFG = 3505708" << endl;
}

void CadastrarNFe::writeEmitente(QTextStream &stream) const {
  stream << "[Emitente]" << endl;
  stream << "CNPJ = " + clearStr(modelLoja.data(0, "cnpj").toString()) << endl;
  //  stream << "CNPJ = 99999090910270" << endl;
  stream << "IE = " + modelLoja.data(0, "inscEstadual").toString() << endl;
  stream << "Razao = " + modelLoja.data(0, "razaoSocial").toString().left(60) << endl;
  stream << "Fantasia = " + modelLoja.data(0, "nomeFantasia").toString() << endl;
  stream << "Fone = " + modelLoja.data(0, "tel").toString() << endl;
  stream << "CEP = " + clearStr(queryLojaEnd.value("CEP").toString()) << endl;
  stream << "Logradouro = " + queryLojaEnd.value("logradouro").toString() << endl;
  stream << "Numero = " + queryLojaEnd.value("numero").toString() << endl;
  stream << "Complemento = " + queryLojaEnd.value("complemento").toString() << endl;
  stream << "Bairro = " + queryLojaEnd.value("bairro").toString() << endl;
  // TODO: fix
  //  stream << "cMun = 3505708" << endl;
  stream << "Cidade = " + queryLojaEnd.value("cidade").toString() << endl;
  stream << "UF = " + queryLojaEnd.value("uf").toString() << endl;
}

void CadastrarNFe::writeDestinatario(QTextStream &stream) const {
  stream << "[Destinatario]" << endl;
  stream << "NomeRazao = " + queryCliente.value("nome_razao").toString() << endl;

  if (queryCliente.value("pfpj").toString() == "PF") {
    stream << "CPF = " + clearStr(queryCliente.value("cpf").toString()) << endl;
    stream << "indIEDest = 9" << endl;
  }

  if (queryCliente.value("pfpj").toString() == "PJ") {
    stream << "CNPJ = " + clearStr(queryCliente.value("cnpj").toString()) << endl;

    const QString inscEst = queryCliente.value("inscEstadual").toString();

    stream << (inscEst == "ISENTO" or inscEst.isEmpty() ? "indIEDest = 9" : "IE = " + clearStr(inscEst)) << endl;
  }

  stream << "Fone = " + queryCliente.value("tel").toString() << endl;
  stream << "CEP = " + clearStr(queryEndereco.value("cep").toString()) << endl;
  stream << "Logradouro = " + queryEndereco.value("logradouro").toString().left(60) << endl;
  stream << "Numero = " + queryEndereco.value("numero").toString() << endl;
  stream << "Complemento = " + queryEndereco.value("complemento").toString().left(60) << endl;
  stream << "Bairro = " + queryEndereco.value("bairro").toString() << endl;
  stream << "cMun = " + queryIBGE.value("codigo").toString() << endl;
  stream << "Cidade = " + queryEndereco.value("cidade").toString() << endl;
  stream << "UF = " + queryEndereco.value("uf").toString() << endl;
}

void CadastrarNFe::writeProduto(QTextStream &stream) const {
  for (int row = 0; row < modelViewProdutoEstoque.rowCount(); ++row) {
    const QString numProd = QString("%1").arg(row + 1, 3, 10, QChar('0')); // padding with zeros
    stream << "[Produto" + numProd + "]" << endl;
    stream << "CFOP = " + modelViewProdutoEstoque.data(row, "cfop").toString() << endl;
    stream << "CEST = 1003001" << endl;
    stream << "NCM = " + modelViewProdutoEstoque.data(row, "ncm").toString() << endl;
    stream << "Codigo = " + modelViewProdutoEstoque.data(row, "codComercial").toString() << endl;
    const QString codBarras = modelViewProdutoEstoque.data(row, "codBarras").toString();
    stream << "EAN = " + (codBarras.isEmpty() ? "" : codBarras) << endl;
    const QString produto = modelViewProdutoEstoque.data(row, "produto").toString();
    QString formato = modelViewProdutoEstoque.data(row, "formComercial").toString();
    formato = formato.isEmpty() ? "" : " - " + formato;
    const QString caixas = modelViewProdutoEstoque.data(row, "caixas").toString();
    stream << "Descricao = " + produto + formato + " (" + caixas + " Cx.)" << endl;
    stream << "Unidade = " + modelViewProdutoEstoque.data(row, "un").toString() << endl;
    stream << "Quantidade = " + modelViewProdutoEstoque.data(row, "quant").toString() << endl;
    const double total = modelViewProdutoEstoque.data(row, "total").toDouble();
    const double quant = modelViewProdutoEstoque.data(row, "quant").toDouble();
    stream << "ValorUnitario = " + QString::number(total / quant, 'f', 10) << endl;
    stream << "ValorTotal = " + modelViewProdutoEstoque.data(row, "total").toString() << endl;
    const double frete = total / ui->doubleSpinBoxValorProdutos->value() * ui->doubleSpinBoxValorFrete->value();
    stream << "vFrete = " + QString::number(frete) << endl;

    stream << "[ICMS" + numProd + "]" << endl;
    stream << "CST = " + modelViewProdutoEstoque.data(row, "cstICMS").toString() << endl;
    stream << "Modalidade = " + modelViewProdutoEstoque.data(row, "modBC").toString() << endl;
    stream << "ValorBase = " + modelViewProdutoEstoque.data(row, "vBC").toString() << endl;
    const double aliquota = modelViewProdutoEstoque.data(row, "pICMS").toDouble();
    stream << "Aliquota = " + QString::number(aliquota, 'f', 2) << endl;
    stream << "Valor = " + modelViewProdutoEstoque.data(row, "vICMS").toString() << endl;

    stream << "[IPI" + numProd + "]" << endl;
    stream << "ClasseEnquadramento = " + modelViewProdutoEstoque.data(row, "cEnq").toString() << endl;
    stream << "CST = " + modelViewProdutoEstoque.data(row, "cstIPI").toString() << endl;

    stream << "[PIS" + numProd + "]" << endl;
    stream << "CST = " + modelViewProdutoEstoque.data(row, "cstPIS").toString() << endl;
    stream << "ValorBase = " + modelViewProdutoEstoque.data(row, "vBCPIS").toString() << endl;
    stream << "Aliquota = " + modelViewProdutoEstoque.data(row, "pPIS").toString() << endl;
    stream << "Valor = " + modelViewProdutoEstoque.data(row, "vPIS").toString() << endl;

    stream << "[COFINS" + numProd + "]" << endl;
    stream << "CST = " + modelViewProdutoEstoque.data(row, "cstCOFINS").toString() << endl;
    stream << "ValorBase = " + modelViewProdutoEstoque.data(row, "vBCCOFINS").toString() << endl;
    stream << "Aliquota = " + modelViewProdutoEstoque.data(row, "pCOFINS").toString() << endl;
    stream << "Valor = " + modelViewProdutoEstoque.data(row, "vCOFINS").toString() << endl;

    // PARTILHA ICMS

    if (ui->comboBoxDestinoOperacao->currentText().startsWith("2")) {
      stream << "[ICMSUFDest" + numProd + "]" << endl;
      stream << "vBCUFDest = " + modelViewProdutoEstoque.data(row, "vBCPIS").toString() << endl;
      stream << "pFCPUFDest = 2" << endl; // REFAC: depende do estado
      stream << "pICMSUFDest = " + queryPartilhaIntra.value("valor").toString() << endl;
      stream << "pICMSInter = " + queryPartilhaInter.value("valor").toString() << endl;

      const double diferencaICMS = (queryPartilhaIntra.value("valor").toDouble() - queryPartilhaInter.value("valor").toDouble()) / 100.;
      const double difal = modelViewProdutoEstoque.data(row, "vBCPIS").toDouble() * diferencaICMS;

      // REFAC: o valor depende do ano atual; a partir de 2019 é 100% para o estado de destino
      stream << "pICMSInterPart = 80" << endl;
      stream << "vFCPUFDest = " + QString::number(modelViewProdutoEstoque.data(row, "vBCPIS").toDouble() * 0.02) << endl; // 2% FCP
      stream << "vICMSUFDest = " + QString::number(difal * 0.8) << endl;
      stream << "vICMSUFRemet = " + QString::number(difal * 0.2) << endl;
    }

    //    http://www.asseinfo.com.br/blog/difal-diferencial-de-aliquota-icms/
  }
}

void CadastrarNFe::writeTotal(QTextStream &stream) const {
  stream << "[Total]" << endl;
  stream << "BaseICMS = " + QString::number(ui->doubleSpinBoxBaseICMS->value(), 'f', 2) << endl;
  stream << "ValorICMS = " + QString::number(ui->doubleSpinBoxValorICMS->value(), 'f', 2) << endl;
  stream << "ValorIPI = " + QString::number(ui->doubleSpinBoxValorIPI->value(), 'f', 2) << endl;
  stream << "ValorPIS = " + QString::number(ui->doubleSpinBoxValorPIS->value(), 'f', 2) << endl;
  stream << "ValorCOFINS = " + QString::number(ui->doubleSpinBoxValorCOFINS->value(), 'f', 2) << endl;
  stream << "ValorProduto = " + QString::number(ui->doubleSpinBoxValorProdutos->value(), 'f', 2) << endl;
  stream << "ValorFrete = " + QString::number(ui->doubleSpinBoxValorFrete->value(), 'f', 2) << endl;
  stream << "ValorNota = " + QString::number(ui->doubleSpinBoxValorNota->value(), 'f', 2) << endl;

  // PARTILHA ICMS

  if (ui->comboBoxDestinoOperacao->currentText().startsWith("2")) {
    double totalFcp = 0;
    double totalIcmsDest = 0;
    double totalIcmsOrig = 0;

    const double diferencaICMS = (queryPartilhaIntra.value("valor").toDouble() - queryPartilhaInter.value("valor").toDouble()) / 100.;

    for (int row = 0; row < modelViewProdutoEstoque.rowCount(); ++row) {
      totalFcp += modelViewProdutoEstoque.data(row, "vBCPIS").toDouble() * 0.02;

      const double difal = modelViewProdutoEstoque.data(row, "vBCPIS").toDouble() * diferencaICMS;

      totalIcmsDest += difal * 0.8;
      totalIcmsOrig += difal * 0.2;
    }

    stream << "vFCPUFDest = " + QString::number(totalFcp) << endl;
    stream << "vICMSUFDest = " + QString::number(totalIcmsDest) << endl;
    stream << "vICMSUFRemet = " + QString::number(totalIcmsOrig) << endl;
  }
}

void CadastrarNFe::writeTransportadora(QTextStream &stream) const {
  stream << "[Transportador]" << endl;
  stream << "FretePorConta = " << ui->comboBoxFreteConta->currentText().left(1) << endl;

  if (ui->lineEditTransportadorRazaoSocial->text() != "RETIRA") {
    stream << "NomeRazao = " << ui->lineEditTransportadorRazaoSocial->text() << endl;
    stream << "CnpjCpf = " << ui->lineEditTransportadorCpfCnpj->text() << endl;
    stream << "IE = " << ui->lineEditTransportadorInscEst->text() << endl;
    stream << "Endereco = " << ui->lineEditTransportadorEndereco->text() << endl;
    stream << "Cidade = " << ui->lineEditTransportadorMunicipio->text() << endl;
    stream << "UF = " << ui->lineEditTransportadorUf->text() << endl;
    stream << "ValorServico = " << endl;
    stream << "ValorBase = " << endl;
    stream << "Aliquota = " << endl;
    stream << "Valor = " << endl;
    stream << "CFOP = " << endl;
    stream << "CidadeCod = " << endl;
    stream << "Placa = " << ui->lineEditTransportadorPlaca->text().remove("-") << endl;
    stream << "UFPlaca = " << ui->lineEditTransportadorUfPlaca->text() << endl;
    stream << "RNTC = " << endl;
  }
}

void CadastrarNFe::writePagamento(QTextStream &stream) {
  stream << "[Pag001]" << endl;

  stream << "tPag = 01" << endl;
  stream << "vPag = " + QString::number(ui->doubleSpinBoxValorNota->value()) << endl;
}

void CadastrarNFe::writeVolume(QTextStream &stream) const {
  stream << "[Volume001]" << endl;

  stream << "Quantidade = " << ui->spinBoxVolumesQuant->text() << endl;
  stream << "Especie = " << ui->lineEditVolumesEspecie->text() << endl;
  stream << "Marca = " << ui->lineEditVolumesMarca->text() << endl;
  stream << "Numeracao = " << ui->lineEditVolumesNumeracao->text() << endl;
  stream << "PesoLiquido = " << QString::number(ui->doubleSpinBoxVolumesPesoLiq->value()) << endl;
  stream << "PesoBruto = " << QString::number(ui->doubleSpinBoxVolumesPesoBruto->value()) << endl;
}

void CadastrarNFe::on_tableItens_entered(const QModelIndex &) { ui->tableItens->resizeColumnsToContents(); }

void CadastrarNFe::on_tableItens_clicked(const QModelIndex &index) {
  unsetConnections();

  [=] {
    ui->frame_7->setEnabled(true);
    ui->frame_8->setEnabled(true);
    ui->frame_9->setEnabled(true);
    ui->frame_10->setEnabled(true);

    if (not listarCfop()) { return; }

    ui->comboBoxCfop->setCurrentIndex(ui->comboBoxCfop->findText(modelViewProdutoEstoque.data(index.row(), "cfop").toString(), Qt::MatchStartsWith));
    ui->comboBoxICMSOrig->setCurrentIndex(ui->comboBoxICMSOrig->findText(modelViewProdutoEstoque.data(index.row(), "orig").toString(), Qt::MatchStartsWith));
    ui->comboBoxSituacaoTributaria->setCurrentIndex(ui->comboBoxSituacaoTributaria->findText(modelViewProdutoEstoque.data(index.row(), "cstICMS").toString(), Qt::MatchStartsWith));
    // TODO: fix properly
    this->on_comboBoxSituacaoTributaria_currentTextChanged(ui->comboBoxSituacaoTributaria->currentText());
    ui->comboBoxICMSModBc->setCurrentIndex(modelViewProdutoEstoque.data(index.row(), "modBC").toInt() + 1);
    ui->comboBoxICMSModBcSt->setCurrentIndex(modelViewProdutoEstoque.data(index.row(), "modBCST").toInt() + 1);
    ui->comboBoxIPIcst->setCurrentIndex(ui->comboBoxIPIcst->findText(modelViewProdutoEstoque.data(index.row(), "cstIPI").toString(), Qt::MatchStartsWith));
    ui->comboBoxPIScst->setCurrentIndex(ui->comboBoxPIScst->findText(modelViewProdutoEstoque.data(index.row(), "cstPIS").toString(), Qt::MatchStartsWith));
    ui->comboBoxCOFINScst->setCurrentIndex(ui->comboBoxCOFINScst->findText(modelViewProdutoEstoque.data(index.row(), "cstCOFINS").toString(), Qt::MatchStartsWith));

    QSqlQuery queryCfop;
    if (ui->comboBoxTipo->currentText() == "0 Entrada") queryCfop.prepare("SELECT NAT FROM cfop_entr WHERE CFOP_DE = :cfop OR CFOP_FE = :cfop");
    if (ui->comboBoxTipo->currentText() == "1 Saída") queryCfop.prepare("SELECT NAT FROM cfop_sai WHERE CFOP_DE = :cfop OR CFOP_FE = :cfop");
    queryCfop.bindValue(":cfop", modelViewProdutoEstoque.data(index.row(), "cfop"));

    if (not queryCfop.exec()) {
      emit errorSignal("Erro buscando CFOP: " + queryCfop.lastError().text());
      return;
    }

    // -------------------------------------------------------------------------

    // ICMS Inter

    if (ui->comboBoxDestinoOperacao->currentText().startsWith("2")) {
      ui->doubleSpinBoxPercentualFcpDestino->setValue(2);
      ui->doubleSpinBoxBaseCalculoDestinatario->setValue(modelViewProdutoEstoque.data(index.row(), "vBCPIS").toDouble());
      ui->doubleSpinBoxAliquotaInternaDestinatario->setValue(queryPartilhaIntra.value("valor").toDouble());
      ui->doubleSpinBoxAliquotaInter->setValue(queryPartilhaInter.value("valor").toDouble());
      ui->doubleSpinBoxPercentualPartilha->setValue(80);

      const double diferencaICMS = (queryPartilhaIntra.value("valor").toDouble() - queryPartilhaInter.value("valor").toDouble()) / 100.;
      const double difal = modelViewProdutoEstoque.data(index.row(), "vBCPIS").toDouble() * diferencaICMS;

      ui->doubleSpinBoxPartilhaDestinatario->setValue(difal * 0.8);
      ui->doubleSpinBoxPartilhaRemetente->setValue(difal * 0.2);
      ui->doubleSpinBoxFcpDestino->setValue(modelViewProdutoEstoque.data(index.row(), "vBCPIS").toDouble() * 0.02);
    }

    // -------------------------------------------------------------------------

    mapper.setCurrentModelIndex(index);
  }();

  setConnections();
}

void CadastrarNFe::on_tabWidget_currentChanged(const int index) {
  if (index == 4) { updateImpostos(); }
}

void CadastrarNFe::on_doubleSpinBoxICMSvbc_valueChanged(const double) {
  const auto list = ui->tableItens->selectionModel()->selectedRows();

  if (list.isEmpty()) { return; }

  unsetConnections();
  [=] {
    ui->doubleSpinBoxICMSvicms->setValue(ui->doubleSpinBoxICMSvbc->value() * ui->doubleSpinBoxICMSpicms->value() / 100);

    const int row = list.first().row();

    if (not modelViewProdutoEstoque.setData(row, "vBC", ui->doubleSpinBoxICMSvbc->value())) { return; }
    if (not modelViewProdutoEstoque.setData(row, "pICMS", ui->doubleSpinBoxICMSpicms->value())) { return; }
    if (not modelViewProdutoEstoque.setData(row, "vICMS", ui->doubleSpinBoxICMSvicms->value())) { return; }
  }();
  setConnections();
}

void CadastrarNFe::on_doubleSpinBoxICMSvbcst_valueChanged(const double) {
  const auto list = ui->tableItens->selectionModel()->selectedRows();

  if (list.isEmpty()) { return; }

  unsetConnections();
  [=] {
    ui->doubleSpinBoxICMSvicmsst->setValue(ui->doubleSpinBoxICMSvbcst->value() * ui->doubleSpinBoxICMSpicmsst->value() / 100);

    const int row = list.first().row();

    if (not modelViewProdutoEstoque.setData(row, "vBCST", ui->doubleSpinBoxICMSvbcst->value())) { return; }
    if (not modelViewProdutoEstoque.setData(row, "pICMSST", ui->doubleSpinBoxICMSpicmsst->value())) { return; }
    if (not modelViewProdutoEstoque.setData(row, "vICMSST", ui->doubleSpinBoxICMSvicmsst->value())) { return; }
  }();
  setConnections();
}

void CadastrarNFe::on_doubleSpinBoxICMSpicms_valueChanged(const double) {
  const auto list = ui->tableItens->selectionModel()->selectedRows();

  if (list.isEmpty()) { return; }

  unsetConnections();
  [=] {
    ui->doubleSpinBoxICMSvicms->setValue(ui->doubleSpinBoxICMSvbc->value() * ui->doubleSpinBoxICMSpicms->value() / 100);

    const int row = list.first().row();

    if (not modelViewProdutoEstoque.setData(row, "vBC", ui->doubleSpinBoxICMSvbc->value())) { return; }
    if (not modelViewProdutoEstoque.setData(row, "pICMS", ui->doubleSpinBoxICMSpicms->value())) { return; }
    if (not modelViewProdutoEstoque.setData(row, "vICMS", ui->doubleSpinBoxICMSvicms->value())) { return; }
  }();
  setConnections();
}

void CadastrarNFe::on_doubleSpinBoxPISvbc_valueChanged(const double) {
  const auto list = ui->tableItens->selectionModel()->selectedRows();

  if (list.isEmpty()) { return; }

  unsetConnections();
  [=] {
    ui->doubleSpinBoxPISvpis->setValue(ui->doubleSpinBoxPISvbc->value() * ui->doubleSpinBoxPISppis->value() / 100);

    const int row = list.first().row();

    if (not modelViewProdutoEstoque.setData(row, "vBCPIS", ui->doubleSpinBoxPISvbc->value())) { return; }
    if (not modelViewProdutoEstoque.setData(row, "pPIS", ui->doubleSpinBoxPISppis->value())) { return; }
    if (not modelViewProdutoEstoque.setData(row, "vPIS", ui->doubleSpinBoxPISvpis->value())) { return; }
  }();
  setConnections();
}

void CadastrarNFe::on_doubleSpinBoxPISppis_valueChanged(const double) {
  const auto list = ui->tableItens->selectionModel()->selectedRows();

  if (list.isEmpty()) { return; }

  unsetConnections();
  [=] {
    ui->doubleSpinBoxPISvpis->setValue(ui->doubleSpinBoxPISvbc->value() * ui->doubleSpinBoxPISppis->value() / 100);

    const int row = list.first().row();

    if (not modelViewProdutoEstoque.setData(row, "vBCPIS", ui->doubleSpinBoxPISvbc->value())) { return; }
    if (not modelViewProdutoEstoque.setData(row, "pPIS", ui->doubleSpinBoxPISppis->value())) { return; }
    if (not modelViewProdutoEstoque.setData(row, "vPIS", ui->doubleSpinBoxPISvpis->value())) { return; }
  }();
  setConnections();
}

void CadastrarNFe::on_doubleSpinBoxCOFINSvbc_valueChanged(const double) {
  const auto list = ui->tableItens->selectionModel()->selectedRows();

  if (list.isEmpty()) { return; }

  unsetConnections();
  [=] {
    ui->doubleSpinBoxCOFINSvcofins->setValue(ui->doubleSpinBoxCOFINSvbc->value() * ui->doubleSpinBoxCOFINSpcofins->value() / 100);

    const int row = list.first().row();

    if (not modelViewProdutoEstoque.setData(row, "vBCCOFINS", ui->doubleSpinBoxCOFINSvbc->value())) { return; }
    if (not modelViewProdutoEstoque.setData(row, "pCOFINS", ui->doubleSpinBoxCOFINSpcofins->value())) { return; }
    if (not modelViewProdutoEstoque.setData(row, "vCOFINS", ui->doubleSpinBoxCOFINSvcofins->value())) { return; }
  }();
  setConnections();
}

void CadastrarNFe::on_doubleSpinBoxCOFINSpcofins_valueChanged(const double) {
  const auto list = ui->tableItens->selectionModel()->selectedRows();

  if (list.isEmpty()) { return; }

  unsetConnections();
  [=] {
    ui->doubleSpinBoxCOFINSvcofins->setValue(ui->doubleSpinBoxCOFINSvbc->value() * ui->doubleSpinBoxCOFINSpcofins->value() / 100);

    const int row = list.first().row();

    if (not modelViewProdutoEstoque.setData(row, "vBCCOFINS", ui->doubleSpinBoxCOFINSvbc->value())) { return; }
    if (not modelViewProdutoEstoque.setData(row, "pCOFINS", ui->doubleSpinBoxCOFINSpcofins->value())) { return; }
    if (not modelViewProdutoEstoque.setData(row, "vCOFINS", ui->doubleSpinBoxCOFINSvcofins->value())) { return; }
  }();
  setConnections();
}

void CadastrarNFe::on_itemBoxEnderecoFaturamento_textChanged(const QString &) {
  QSqlQuery queryDestinatarioEndereco;
  queryDestinatarioEndereco.prepare("SELECT logradouro, numero, complemento, bairro, cidade, uf, cep FROM cliente_has_endereco WHERE idEndereco = :idEndereco");
  queryDestinatarioEndereco.bindValue(":idEndereco", ui->itemBoxEnderecoFaturamento->getValue());

  if (not queryDestinatarioEndereco.exec() or not queryDestinatarioEndereco.first()) {
    emit errorSignal("Erro lendo endereço do cliente: " + queryDestinatarioEndereco.lastError().text());
    return;
  }

  ui->lineEditDestinatarioLogradouro->setText(queryDestinatarioEndereco.value("logradouro").toString());
  ui->lineEditDestinatarioNumero->setText(queryDestinatarioEndereco.value("numero").toString());
  ui->lineEditDestinatarioComplemento->setText(queryDestinatarioEndereco.value("complemento").toString());
  ui->lineEditDestinatarioBairro->setText(queryDestinatarioEndereco.value("bairro").toString());
  ui->lineEditDestinatarioCidade->setText(queryDestinatarioEndereco.value("cidade").toString());
  ui->lineEditDestinatarioUF->setText(queryDestinatarioEndereco.value("uf").toString());
  ui->lineEditDestinatarioCEP->setText(queryDestinatarioEndereco.value("cep").toString());
}

void CadastrarNFe::on_itemBoxEnderecoEntrega_textChanged(const QString &) {
  QSqlQuery queryDestinatarioEndereco;
  queryDestinatarioEndereco.prepare("SELECT logradouro, numero, complemento, bairro, cidade, uf, cep FROM cliente_has_endereco WHERE idEndereco = :idEndereco");
  queryDestinatarioEndereco.bindValue(":idEndereco", ui->itemBoxEnderecoEntrega->getValue());

  if (not queryDestinatarioEndereco.exec() or not queryDestinatarioEndereco.first()) {
    emit errorSignal("Erro lendo endereço do cliente: " + queryDestinatarioEndereco.lastError().text());
    return;
  }

  ui->lineEditDestinatarioLogradouro_2->setText(queryDestinatarioEndereco.value("logradouro").toString());
  ui->lineEditDestinatarioNumero_2->setText(queryDestinatarioEndereco.value("numero").toString());
  ui->lineEditDestinatarioComplemento_2->setText(queryDestinatarioEndereco.value("complemento").toString());
  ui->lineEditDestinatarioBairro_2->setText(queryDestinatarioEndereco.value("bairro").toString());
  ui->lineEditDestinatarioCidade_2->setText(queryDestinatarioEndereco.value("cidade").toString());
  ui->lineEditDestinatarioUF_2->setText(queryDestinatarioEndereco.value("uf").toString());
  ui->lineEditDestinatarioCEP_2->setText(queryDestinatarioEndereco.value("cep").toString());

  updateImpostos();
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

  const bool tribNormal = ui->comboBoxRegime->currentText() == "Tributação Normal";

  if (not modelViewProdutoEstoque.setData(list.first().row(), "tipoICMS", tribNormal ? "ICMS" + text.left(2) : "ICMSSN" + text.left(3))) { return; }
  if (not modelViewProdutoEstoque.setData(list.first().row(), "cstICMS", text.left(tribNormal ? 2 : 3))) { return; }

  // -------------------------------------------------------------------------

  if (text == "00 - Tributada integralmente") {
    ui->frame->show();
    ui->frame_2->hide();
  }

  if (text == "10 - Tributada e com cobrança do ICMS por substituição tributária") {}

  if (text == "20 - Com redução de base de cálculo") {}

  if (text == "30 - Isenta ou não tributada e com cobrança do ICMS por substituição tributária") {}

  if (text == "40 - Isenta") {}

  if (text == "41 - Não tributada") {}

  if (text == "50 - Suspensão") {}

  if (text == "51 - Diferimento") {}

  if (text == "60 - ICMS cobrado anteriormente por substituição tributária") {
    ui->frame->hide();
    ui->frame_2->show();

    ui->label_7->hide();
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

  if (not modelViewProdutoEstoque.setData(list.first().row(), "orig", index - 1)) { return; }
}

void CadastrarNFe::on_comboBoxICMSModBc_currentIndexChanged(const int index) {
  // modBC

  if (index == 0) { return; }

  const auto list = ui->tableItens->selectionModel()->selectedRows();

  if (list.isEmpty()) { return; }

  // REFAC: wrap in a unset/set so updateImpostos is called only once in the end?
  if (not modelViewProdutoEstoque.setData(list.first().row(), "modBC", index - 1)) { return; }

  if (ui->comboBoxICMSModBc->currentText() == "Valor da Operação") {
    ui->doubleSpinBoxICMSvbc->setValue(modelViewProdutoEstoque.data(list.first().row(), "vBCPIS").toDouble());
    if (not modelViewProdutoEstoque.setData(list.first().row(), "vBC", modelViewProdutoEstoque.data(list.first().row(), "vBCPIS").toDouble())) { return; }
    if (not modelViewProdutoEstoque.setData(list.first().row(), "pICMS", 7)) { return; }
    // TODO: verificar a aliquota entre estados e setar a porcentagem (caso seja interestadual)
  }

  updateImpostos();
}

void CadastrarNFe::on_comboBoxICMSModBcSt_currentIndexChanged(const int index) {
  // modBCST

  if (index == 0) { return; }

  const auto list = ui->tableItens->selectionModel()->selectedRows();

  if (list.isEmpty()) { return; }

  if (not modelViewProdutoEstoque.setData(list.first().row(), "modBCST", index - 1)) { return; }
}

void CadastrarNFe::on_doubleSpinBoxICMSvicms_valueChanged(const double) {
  const auto list = ui->tableItens->selectionModel()->selectedRows();

  if (list.isEmpty()) { return; }

  unsetConnections();
  [=] {
    ui->doubleSpinBoxICMSvbc->setValue(ui->doubleSpinBoxICMSvicms->value() * 100 / ui->doubleSpinBoxICMSpicms->value());

    const int row = list.first().row();

    if (not modelViewProdutoEstoque.setData(row, "vBC", ui->doubleSpinBoxICMSvbc->value())) { return; }
    if (not modelViewProdutoEstoque.setData(row, "pICMS", ui->doubleSpinBoxICMSpicms->value())) { return; }
    if (not modelViewProdutoEstoque.setData(row, "vICMS", ui->doubleSpinBoxICMSvicms->value())) { return; }
  }();
  setConnections();
}

void CadastrarNFe::on_doubleSpinBoxICMSvicmsst_valueChanged(const double) {
  const auto list = ui->tableItens->selectionModel()->selectedRows();

  if (list.isEmpty()) { return; }

  unsetConnections();
  [=] {
    ui->doubleSpinBoxICMSvbcst->setValue(ui->doubleSpinBoxICMSvicmsst->value() * 100 / ui->doubleSpinBoxICMSpicmsst->value());

    const int row = list.first().row();

    if (not modelViewProdutoEstoque.setData(row, "vBCST", ui->doubleSpinBoxICMSvbcst->value())) { return; }
    if (not modelViewProdutoEstoque.setData(row, "pICMSST", ui->doubleSpinBoxICMSpicmsst->value())) { return; }
    if (not modelViewProdutoEstoque.setData(row, "vICMSST", ui->doubleSpinBoxICMSvicmsst->value())) { return; }
  }();
  setConnections();
}

void CadastrarNFe::on_comboBoxIPIcst_currentTextChanged(const QString &text) {
  const auto list = ui->tableItens->selectionModel()->selectedRows();

  if (list.isEmpty()) { return; }

  if (not modelViewProdutoEstoque.setData(list.first().row(), "cstIPI", text.left(2))) { return; }
}

void CadastrarNFe::on_comboBoxPIScst_currentTextChanged(const QString &text) {
  const auto list = ui->tableItens->selectionModel()->selectedRows();

  if (list.isEmpty()) { return; }

  if (not modelViewProdutoEstoque.setData(list.first().row(), "cstPIS", text.left(2))) { return; }
}

void CadastrarNFe::on_doubleSpinBoxPISvpis_valueChanged(const double) {
  const auto list = ui->tableItens->selectionModel()->selectedRows();

  if (list.isEmpty()) { return; }

  unsetConnections();
  [=] {
    ui->doubleSpinBoxPISvbc->setValue(ui->doubleSpinBoxPISvpis->value() * 100 / ui->doubleSpinBoxPISppis->value());

    const int row = list.first().row();

    if (not modelViewProdutoEstoque.setData(row, "vBCPIS", ui->doubleSpinBoxPISvbc->value())) { return; }
    if (not modelViewProdutoEstoque.setData(row, "pPIS", ui->doubleSpinBoxPISppis->value())) { return; }
    if (not modelViewProdutoEstoque.setData(row, "vPIS", ui->doubleSpinBoxPISvpis->value())) { return; }
  }();
  setConnections();
}

void CadastrarNFe::on_comboBoxCOFINScst_currentTextChanged(const QString &text) {
  const auto list = ui->tableItens->selectionModel()->selectedRows();

  if (list.isEmpty()) { return; }

  if (not modelViewProdutoEstoque.setData(list.first().row(), "cstCOFINS", text.left(2))) { return; }
}

void CadastrarNFe::on_doubleSpinBoxCOFINSvcofins_valueChanged(const double) {
  const auto list = ui->tableItens->selectionModel()->selectedRows();

  if (list.isEmpty()) { return; }

  unsetConnections();
  [=] {
    ui->doubleSpinBoxCOFINSvbc->setValue(ui->doubleSpinBoxCOFINSvcofins->value() * 100 / ui->doubleSpinBoxCOFINSpcofins->value());

    const int row = list.first().row();

    if (not modelViewProdutoEstoque.setData(row, "vBCCOFINS", ui->doubleSpinBoxCOFINSvbc->value())) { return; }
    if (not modelViewProdutoEstoque.setData(row, "pCOFINS", ui->doubleSpinBoxCOFINSpcofins->value())) { return; }
    if (not modelViewProdutoEstoque.setData(row, "vCOFINS", ui->doubleSpinBoxCOFINSvcofins->value())) { return; }
  }();
  setConnections();
}

void CadastrarNFe::on_doubleSpinBoxICMSpicmsst_valueChanged(const double) {
  const auto list = ui->tableItens->selectionModel()->selectedRows();

  if (list.isEmpty()) { return; }

  unsetConnections();
  [=] {
    ui->doubleSpinBoxICMSvicmsst->setValue(ui->doubleSpinBoxICMSvbcst->value() * ui->doubleSpinBoxICMSpicmsst->value() / 100);

    const int row = list.first().row();

    if (not modelViewProdutoEstoque.setData(row, "vBCST", ui->doubleSpinBoxICMSvbcst->value())) { return; }
    if (not modelViewProdutoEstoque.setData(row, "pICMSST", ui->doubleSpinBoxICMSpicmsst->value())) { return; }
    if (not modelViewProdutoEstoque.setData(row, "vICMSST", ui->doubleSpinBoxICMSvicmsst->value())) { return; }
  }();
  setConnections();
}

void CadastrarNFe::on_itemBoxVeiculo_textChanged(const QString &) {
  QSqlQuery queryTransp;
  queryTransp.prepare("SELECT t.cnpj, t.razaoSocial, t.inscEstadual, the.logradouro, the.numero, the.complemento, the.bairro, the.cidade, the.uf, thv.placa, thv.ufPlaca, t.antt FROM "
                      "transportadora_has_veiculo thv LEFT JOIN transportadora t ON thv.idTransportadora = t.idTransportadora LEFT JOIN transportadora_has_endereco the ON the.idTransportadora = "
                      "t.idTransportadora WHERE thv.idVeiculo = :idVeiculo");
  queryTransp.bindValue(":idVeiculo", ui->itemBoxVeiculo->getValue());

  if (not queryTransp.exec() or not queryTransp.first()) {
    emit errorSignal("Erro buscando dados da transportadora: " + queryTransp.lastError().text());
    return;
  }

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

void CadastrarNFe::on_itemBoxCliente_textChanged(const QString &) {
  QSqlQuery query;
  query.prepare("SELECT nome_razao, pfpj, cpf, cnpj, inscEstadual, tel, telCel FROM cliente WHERE idCliente = :idCliente");
  query.bindValue(":idCliente", ui->itemBoxCliente->getValue());

  if (not query.exec() or not query.first()) {
    emit errorSignal("Erro buscando dados do cliente: " + query.lastError().text());
    return;
  }

  ui->lineEditDestinatarioNomeRazao->setText(query.value("nome_razao").toString());
  ui->lineEditDestinatarioCPFCNPJ->setText(query.value(query.value("pfpj").toString() == "PF" ? "cpf" : "cnpj").toString());
  ui->lineEditDestinatarioInscEst->setText(query.value("inscEstadual").toString());
  ui->lineEditDestinatarioTel1->setText(query.value("tel").toString());
  ui->lineEditDestinatarioTel2->setText(query.value("telCel").toString());

  ui->itemBoxEnderecoFaturamento->setFilter("idCliente = " + ui->itemBoxCliente->getValue().toString() + " AND desativado = FALSE OR idEndereco = 1");
  ui->itemBoxEnderecoFaturamento->setValue(1);

  ui->itemBoxEnderecoEntrega->setFilter("idCliente = " + ui->itemBoxCliente->getValue().toString() + " AND desativado = FALSE OR idEndereco = 1");

  ui->itemBoxEnderecoEntrega->setValue(1);
}

bool CadastrarNFe::validar() {
  // NOTE: implement validation rules

  // validacao do model

  // TODO: 5recalcular todos os valores dos itens para verificar se os dados batem (usar o 174058 de referencia)

  // -------------------------------------------------------------------------

  if (ui->itemBoxLoja->text().isEmpty()) {
    // assume no certificate in acbr
    emit errorSignal("Escolha um certificado para o ACBr!");
    return false;
  }

  // [Emitente]

  if (clearStr(modelLoja.data(0, "cnpj").toString()).isEmpty()) {
    emit errorSignal("CNPJ emitente vazio!");
    return false;
  }

  if (modelLoja.data(0, "razaoSocial").toString().isEmpty()) {
    emit errorSignal("Razão Social emitente vazio!");
    return false;
  }

  if (modelLoja.data(0, "nomeFantasia").toString().isEmpty()) {
    emit errorSignal("Nome Fantasia emitente vazio!");
    return false;
  }

  if (modelLoja.data(0, "tel").toString().isEmpty()) {
    emit errorSignal("Telefone emitente vazio!");
    return false;
  }

  queryLojaEnd.prepare("SELECT cep, logradouro, numero, complemento, bairro, cidade, uf FROM loja_has_endereco WHERE idLoja = :idLoja");
  queryLojaEnd.bindValue(":idLoja", modelLoja.data(0, "idLoja"));

  if (not queryLojaEnd.exec() or not queryLojaEnd.first()) {
    emit errorSignal("Erro lendo tabela de endereços da loja: " + queryLojaEnd.lastError().text());
    return false;
  }

  if (clearStr(queryLojaEnd.value("CEP").toString()).isEmpty()) {
    emit errorSignal("CEP vazio!");
    return false;
  }

  if (queryLojaEnd.value("logradouro").toString().isEmpty()) {
    emit errorSignal("Logradouro vazio!");
    return false;
  }

  if (queryLojaEnd.value("numero").toString().isEmpty()) {
    emit errorSignal("Número vazio!");
    return false;
  }

  if (queryLojaEnd.value("bairro").toString().isEmpty()) {
    emit errorSignal("Bairro vazio!");
    return false;
  }

  if (queryLojaEnd.value("cidade").toString().isEmpty()) {
    emit errorSignal("Cidade vazio!");
    return false;
  }

  if (queryLojaEnd.value("uf").toString().isEmpty()) {
    emit errorSignal("UF vazio!");
    return false;
  }

  // [Destinatario]

  if (modelVenda.data(0, "idCliente").toString().isEmpty()) {
    emit errorSignal("idCliente vazio!");
    return false;
  }

  queryCliente.prepare("SELECT nome_razao, pfpj, cpf, cnpj, inscEstadual, tel FROM cliente WHERE idCliente = :idCliente");
  queryCliente.bindValue(":idCliente", modelVenda.data(0, "idCliente"));

  if (not queryCliente.exec() or not queryCliente.first()) {
    emit errorSignal("Erro buscando endereço do destinatário: " + queryCliente.lastError().text());
    return false;
  }

  if (queryCliente.value("nome_razao").toString().isEmpty()) {
    emit errorSignal("Nome/Razão vazio!");
    return false;
  }

  if (queryCliente.value("pfpj").toString() == "PF") {
    if (clearStr(queryCliente.value("cpf").toString()).isEmpty()) {
      emit errorSignal("CPF vazio!");
      return false;
    }
  }

  if (queryCliente.value("pfpj").toString() == "PJ") {
    if (clearStr(queryCliente.value("cnpj").toString()).isEmpty()) {
      emit errorSignal("CNPJ destinatário vazio!");
      return false;
    }
  }

  queryEndereco.prepare("SELECT cep, logradouro, numero, complemento, bairro, cidade, uf FROM cliente_has_endereco WHERE idEndereco = :idEndereco");
  queryEndereco.bindValue(":idEndereco", ui->itemBoxEnderecoFaturamento->getValue());

  if (not queryEndereco.exec() or not queryEndereco.first()) {
    emit errorSignal("Erro buscando endereço cliente: " + queryEndereco.lastError().text());
    return false;
  }

  if (queryEndereco.value("cep").toString().isEmpty()) {
    emit errorSignal("CEP cliente vazio!");
    return false;
  }

  if (queryEndereco.value("logradouro").toString().isEmpty()) {
    emit errorSignal("Logradouro cliente vazio!");
    return false;
  }

  if (queryEndereco.value("numero").toString().isEmpty()) {
    emit errorSignal("Número endereço do cliente vazio!");
    return false;
  }

  if (queryEndereco.value("bairro").toString().isEmpty()) {
    emit errorSignal("Bairro do cliente vazio!");
    return false;
  }

  queryIBGE.prepare("SELECT codigo FROM cidade WHERE nome = :cidade AND uf = :uf");
  queryIBGE.bindValue(":cidade", queryEndereco.value("cidade"));
  queryIBGE.bindValue(":uf", queryEndereco.value("uf"));

  if (not queryIBGE.exec() or not queryIBGE.first()) {
    emit errorSignal("Erro buscando código do munícipio, verifique se a cidade/estado estão cadastrados corretamente!");
    return false;
  }

  if (queryEndereco.value("cidade").toString().isEmpty()) {
    emit errorSignal("Cidade cliente vazio!");
    return false;
  }

  if (queryEndereco.value("uf").toString().isEmpty()) {
    emit errorSignal("UF cliente vazio!");
    return false;
  }

  queryPartilhaInter.prepare("SELECT valor FROM icms WHERE origem = :origem AND destino = :destino");
  queryPartilhaInter.bindValue(":origem", queryLojaEnd.value("uf"));
  queryPartilhaInter.bindValue(":destino", queryEndereco.value("uf"));

  if (not queryPartilhaInter.exec() or not queryPartilhaInter.first()) {
    emit errorSignal("Erro buscando partilha ICMS: " + queryPartilhaInter.lastError().text());
    return false;
  }

  queryPartilhaIntra.prepare("SELECT valor FROM icms WHERE origem = :origem AND destino = :destino");
  queryPartilhaIntra.bindValue(":origem", queryEndereco.value("uf"));
  queryPartilhaIntra.bindValue(":destino", queryEndereco.value("uf"));

  if (not queryPartilhaIntra.exec() or not queryPartilhaIntra.first()) {
    emit errorSignal("Erro buscando partilha ICMS intra: " + queryPartilhaIntra.lastError().text());
    return false;
  }

  // [Produto]

  for (int row = 0; row < modelViewProdutoEstoque.rowCount(); ++row) {
    if (modelViewProdutoEstoque.data(row, "cfop").toString().isEmpty()) { emit warningSignal("CFOP vazio!"); }
    if (modelViewProdutoEstoque.data(row, "ncm").toString().isEmpty()) { emit warningSignal("NCM vazio!"); }
    if (modelViewProdutoEstoque.data(row, "codComercial").toString().isEmpty()) { emit warningSignal("Código vazio!"); }
    if (modelViewProdutoEstoque.data(row, "produto").toString().isEmpty()) { emit warningSignal("Descrição vazio!"); }
    if (modelViewProdutoEstoque.data(row, "un").toString().isEmpty()) { emit warningSignal("Unidade vazio!"); }
    if (modelViewProdutoEstoque.data(row, "quant").toString().isEmpty()) { emit warningSignal("Quantidade vazio!"); }
    if (qFuzzyIsNull(modelViewProdutoEstoque.data(row, "descUnitario").toDouble())) { emit warningSignal("Preço unitário = R$ 0!"); }
    if (modelViewProdutoEstoque.data(row, "total").toString().isEmpty()) { emit warningSignal("Total produto vazio!"); }
  }

  return true;
}

void CadastrarNFe::on_comboBoxCfop_currentTextChanged(const QString &text) {
  const auto list = ui->tableItens->selectionModel()->selectedRows();

  if (list.isEmpty()) { return; }

  if (not modelViewProdutoEstoque.setData(list.first().row(), "cfop", text.left(4))) { return; }
}

void CadastrarNFe::on_pushButtonConsultarCadastro_clicked() {
  const auto resposta = ACBr::enviarComando("NFE.ConsultaCadastro(" + ui->lineEditDestinatarioUF->text() + ", " + ui->lineEditDestinatarioCPFCNPJ->text() + ")");

  if (not resposta) { return; }

  if (resposta->contains("xMotivo=Consulta cadastro com uma ocorrência")) {
    QStringList list = resposta->mid(resposta->indexOf("IE=")).split("\n");
    const QString insc = list.first().remove("IE=");

    if (not insc.isEmpty()) {
      ui->lineEditDestinatarioInscEst->setText(insc);

      QSqlQuery query;
      query.prepare("UPDATE cliente SET inscEstadual = :inscEstadual WHERE idCliente = :idCliente");
      query.bindValue(":inscEstadual", insc);
      query.bindValue(":idCliente", modelVenda.data(0, "idCliente"));

      if (not query.exec()) {
        emit errorSignal("Erro atualizando Insc. Est.: " + query.lastError().text());
        return;
      }
    }
  }

  emit informationSignal(*resposta);
}

void CadastrarNFe::on_doubleSpinBoxValorFrete_valueChanged(const double value) {
  // TODO: 1refazer rateamento do frete
  Q_UNUSED(value)
}

void CadastrarNFe::alterarCertificado(const QString &text) {
  if (text.isEmpty()) { return; }

  QSqlQuery query;
  query.prepare("SELECT certificadoSerie, certificadoSenha FROM loja WHERE idLoja = :idLoja AND certificadoSerie IS NOT NULL");
  query.bindValue(":idLoja", ui->itemBoxLoja->getValue());

  if (not query.exec()) {
    emit errorSignal("Erro buscando certificado: " + query.lastError().text());
    return;
  }

  if (not query.first()) {
    emit errorSignal("A loja selecionada não possui certificado cadastrado no sistema!");
    return;
  }

  if (const auto resposta = ACBr::enviarComando("NFE.SetCertificado(" + query.value("certificadoSerie").toString() + "," + query.value("certificadoSenha").toString() + ")");
      not resposta or not resposta->contains("OK")) {
    emit errorSignal(*resposta);
    ui->itemBoxLoja->clear();
    return;
  }

  if (not preencherNumeroNFe()) { return; }

  QSqlQuery queryEmitente;
  queryEmitente.prepare("SELECT razaoSocial, nomeFantasia, cnpj, inscEstadual, tel, tel2 FROM loja WHERE idLoja = :idLoja");
  queryEmitente.bindValue(":idLoja", ui->itemBoxLoja->getValue());

  if (not queryEmitente.exec() or not queryEmitente.first()) {
    emit errorSignal("Erro lendo dados do emitente: " + queryEmitente.lastError().text());
    return;
  }

  ui->lineEditEmitenteNomeRazao->setText(queryEmitente.value("razaoSocial").toString());
  ui->lineEditEmitenteFantasia->setText(queryEmitente.value("nomeFantasia").toString());
  ui->lineEditEmitenteCNPJ->setText(queryEmitente.value("cnpj").toString());
  ui->lineEditEmitenteInscEstadual->setText(queryEmitente.value("inscEstadual").toString());
  ui->lineEditEmitenteTel1->setText(queryEmitente.value("tel").toString());
  ui->lineEditEmitenteTel2->setText(queryEmitente.value("tel2").toString());

  QSqlQuery queryEmitenteEndereco;
  queryEmitenteEndereco.prepare("SELECT logradouro, numero, complemento, bairro, cidade, uf, cep FROM loja_has_endereco WHERE idLoja = :idLoja");
  queryEmitenteEndereco.bindValue(":idLoja", ui->itemBoxLoja->getValue());

  if (not queryEmitenteEndereco.exec() or not queryEmitenteEndereco.first()) {
    emit errorSignal("Erro lendo endereço do emitente: " + queryEmitenteEndereco.lastError().text());
    return;
  }

  ui->lineEditEmitenteLogradouro->setText(queryEmitenteEndereco.value("logradouro").toString());
  ui->lineEditEmitenteNumero->setText(queryEmitenteEndereco.value("numero").toString());
  ui->lineEditEmitenteComplemento->setText(queryEmitenteEndereco.value("complemento").toString());
  ui->lineEditEmitenteBairro->setText(queryEmitenteEndereco.value("bairro").toString());
  ui->lineEditEmitenteCidade->setText(queryEmitenteEndereco.value("cidade").toString());
  ui->lineEditEmitenteUF->setText(queryEmitenteEndereco.value("uf").toString());
  ui->lineEditEmitenteCEP->setText(queryEmitenteEndereco.value("cep").toString());

  // TODO: 0pedir para trocar cartao e rodar teste no acbr para verificar se esta tudo ok e funcionando
}

void CadastrarNFe::setConnections() {
  connect(ui->comboBoxCOFINScst, &QComboBox::currentTextChanged, this, &CadastrarNFe::on_comboBoxCOFINScst_currentTextChanged);
  connect(ui->comboBoxCfop, &QComboBox::currentTextChanged, this, &CadastrarNFe::on_comboBoxCfop_currentTextChanged);
  connect(ui->comboBoxDestinoOperacao, &QComboBox::currentTextChanged, this, &CadastrarNFe::on_comboBoxDestinoOperacao_currentTextChanged);
  connect(ui->comboBoxICMSModBc, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CadastrarNFe::on_comboBoxICMSModBc_currentIndexChanged);
  connect(ui->comboBoxICMSModBcSt, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CadastrarNFe::on_comboBoxICMSModBcSt_currentIndexChanged);
  connect(ui->comboBoxICMSOrig, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CadastrarNFe::on_comboBoxICMSOrig_currentIndexChanged);
  connect(ui->comboBoxIPIcst, &QComboBox::currentTextChanged, this, &CadastrarNFe::on_comboBoxIPIcst_currentTextChanged);
  connect(ui->comboBoxPIScst, &QComboBox::currentTextChanged, this, &CadastrarNFe::on_comboBoxPIScst_currentTextChanged);
  connect(ui->comboBoxRegime, &QComboBox::currentTextChanged, this, &CadastrarNFe::on_comboBoxRegime_currentTextChanged);
  connect(ui->comboBoxSituacaoTributaria, &QComboBox::currentTextChanged, this, &CadastrarNFe::on_comboBoxSituacaoTributaria_currentTextChanged);
  connect(ui->doubleSpinBoxCOFINSpcofins, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &CadastrarNFe::on_doubleSpinBoxCOFINSpcofins_valueChanged);
  connect(ui->doubleSpinBoxCOFINSvbc, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &CadastrarNFe::on_doubleSpinBoxCOFINSvbc_valueChanged);
  connect(ui->doubleSpinBoxCOFINSvcofins, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &CadastrarNFe::on_doubleSpinBoxCOFINSvcofins_valueChanged);
  connect(ui->doubleSpinBoxICMSpicms, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &CadastrarNFe::on_doubleSpinBoxICMSpicms_valueChanged);
  connect(ui->doubleSpinBoxICMSpicmsst, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &CadastrarNFe::on_doubleSpinBoxICMSpicmsst_valueChanged);
  connect(ui->doubleSpinBoxICMSvbc, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &CadastrarNFe::on_doubleSpinBoxICMSvbc_valueChanged);
  connect(ui->doubleSpinBoxICMSvbcst, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &CadastrarNFe::on_doubleSpinBoxICMSvbcst_valueChanged);
  connect(ui->doubleSpinBoxICMSvicms, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &CadastrarNFe::on_doubleSpinBoxICMSvicms_valueChanged);
  connect(ui->doubleSpinBoxICMSvicmsst, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &CadastrarNFe::on_doubleSpinBoxICMSvicmsst_valueChanged);
  connect(ui->doubleSpinBoxPISppis, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &CadastrarNFe::on_doubleSpinBoxPISppis_valueChanged);
  connect(ui->doubleSpinBoxPISvbc, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &CadastrarNFe::on_doubleSpinBoxPISvbc_valueChanged);
  connect(ui->doubleSpinBoxPISvpis, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &CadastrarNFe::on_doubleSpinBoxPISvpis_valueChanged);
  connect(ui->doubleSpinBoxValorFrete, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &CadastrarNFe::on_doubleSpinBoxValorFrete_valueChanged);
  connect(ui->itemBoxCliente, &ItemBox::textChanged, this, &CadastrarNFe::on_itemBoxCliente_textChanged);
  connect(ui->itemBoxEnderecoEntrega, &ItemBox::textChanged, this, &CadastrarNFe::on_itemBoxEnderecoEntrega_textChanged);
  connect(ui->itemBoxEnderecoFaturamento, &ItemBox::textChanged, this, &CadastrarNFe::on_itemBoxEnderecoFaturamento_textChanged);
  connect(ui->itemBoxVeiculo, &ItemBox::textChanged, this, &CadastrarNFe::on_itemBoxVeiculo_textChanged);
  connect(ui->pushButtonConsultarCadastro, &QPushButton::clicked, this, &CadastrarNFe::on_pushButtonConsultarCadastro_clicked);
  connect(ui->pushButtonEnviarNFE, &QPushButton::clicked, this, &CadastrarNFe::on_pushButtonEnviarNFE_clicked);
  connect(ui->tabWidget, &QTabWidget::currentChanged, this, &CadastrarNFe::on_tabWidget_currentChanged);
  connect(ui->tableItens, &TableView::clicked, this, &CadastrarNFe::on_tableItens_clicked);
  connect(ui->tableItens, &TableView::entered, this, &CadastrarNFe::on_tableItens_entered);
}

void CadastrarNFe::unsetConnections() {
  disconnect(ui->comboBoxCOFINScst, &QComboBox::currentTextChanged, this, &CadastrarNFe::on_comboBoxCOFINScst_currentTextChanged);
  disconnect(ui->comboBoxCfop, &QComboBox::currentTextChanged, this, &CadastrarNFe::on_comboBoxCfop_currentTextChanged);
  disconnect(ui->comboBoxDestinoOperacao, &QComboBox::currentTextChanged, this, &CadastrarNFe::on_comboBoxDestinoOperacao_currentTextChanged);
  disconnect(ui->comboBoxICMSModBc, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CadastrarNFe::on_comboBoxICMSModBc_currentIndexChanged);
  disconnect(ui->comboBoxICMSModBcSt, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CadastrarNFe::on_comboBoxICMSModBcSt_currentIndexChanged);
  disconnect(ui->comboBoxICMSOrig, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CadastrarNFe::on_comboBoxICMSOrig_currentIndexChanged);
  disconnect(ui->comboBoxIPIcst, &QComboBox::currentTextChanged, this, &CadastrarNFe::on_comboBoxIPIcst_currentTextChanged);
  disconnect(ui->comboBoxPIScst, &QComboBox::currentTextChanged, this, &CadastrarNFe::on_comboBoxPIScst_currentTextChanged);
  disconnect(ui->comboBoxRegime, &QComboBox::currentTextChanged, this, &CadastrarNFe::on_comboBoxRegime_currentTextChanged);
  disconnect(ui->comboBoxSituacaoTributaria, &QComboBox::currentTextChanged, this, &CadastrarNFe::on_comboBoxSituacaoTributaria_currentTextChanged);
  disconnect(ui->doubleSpinBoxCOFINSpcofins, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &CadastrarNFe::on_doubleSpinBoxCOFINSpcofins_valueChanged);
  disconnect(ui->doubleSpinBoxCOFINSvbc, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &CadastrarNFe::on_doubleSpinBoxCOFINSvbc_valueChanged);
  disconnect(ui->doubleSpinBoxCOFINSvcofins, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &CadastrarNFe::on_doubleSpinBoxCOFINSvcofins_valueChanged);
  disconnect(ui->doubleSpinBoxICMSpicms, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &CadastrarNFe::on_doubleSpinBoxICMSpicms_valueChanged);
  disconnect(ui->doubleSpinBoxICMSpicmsst, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &CadastrarNFe::on_doubleSpinBoxICMSpicmsst_valueChanged);
  disconnect(ui->doubleSpinBoxICMSvbc, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &CadastrarNFe::on_doubleSpinBoxICMSvbc_valueChanged);
  disconnect(ui->doubleSpinBoxICMSvbcst, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &CadastrarNFe::on_doubleSpinBoxICMSvbcst_valueChanged);
  disconnect(ui->doubleSpinBoxICMSvicms, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &CadastrarNFe::on_doubleSpinBoxICMSvicms_valueChanged);
  disconnect(ui->doubleSpinBoxICMSvicmsst, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &CadastrarNFe::on_doubleSpinBoxICMSvicmsst_valueChanged);
  disconnect(ui->doubleSpinBoxPISppis, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &CadastrarNFe::on_doubleSpinBoxPISppis_valueChanged);
  disconnect(ui->doubleSpinBoxPISvbc, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &CadastrarNFe::on_doubleSpinBoxPISvbc_valueChanged);
  disconnect(ui->doubleSpinBoxPISvpis, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &CadastrarNFe::on_doubleSpinBoxPISvpis_valueChanged);
  disconnect(ui->doubleSpinBoxValorFrete, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &CadastrarNFe::on_doubleSpinBoxValorFrete_valueChanged);
  disconnect(ui->itemBoxCliente, &ItemBox::textChanged, this, &CadastrarNFe::on_itemBoxCliente_textChanged);
  disconnect(ui->itemBoxEnderecoEntrega, &ItemBox::textChanged, this, &CadastrarNFe::on_itemBoxEnderecoEntrega_textChanged);
  disconnect(ui->itemBoxEnderecoFaturamento, &ItemBox::textChanged, this, &CadastrarNFe::on_itemBoxEnderecoFaturamento_textChanged);
  disconnect(ui->itemBoxVeiculo, &ItemBox::textChanged, this, &CadastrarNFe::on_itemBoxVeiculo_textChanged);
  disconnect(ui->pushButtonConsultarCadastro, &QPushButton::clicked, this, &CadastrarNFe::on_pushButtonConsultarCadastro_clicked);
  disconnect(ui->pushButtonEnviarNFE, &QPushButton::clicked, this, &CadastrarNFe::on_pushButtonEnviarNFE_clicked);
  disconnect(ui->tabWidget, &QTabWidget::currentChanged, this, &CadastrarNFe::on_tabWidget_currentChanged);
  disconnect(ui->tableItens, &TableView::clicked, this, &CadastrarNFe::on_tableItens_clicked);
  disconnect(ui->tableItens, &TableView::entered, this, &CadastrarNFe::on_tableItens_entered);
}

bool CadastrarNFe::listarCfop() {
  const bool mesmaUF = ui->lineEditEmitenteUF->text() == ui->lineEditDestinatarioUF->text();
  const bool entrada = ui->comboBoxTipo->currentText() == "0 Entrada";

  const QString stringUF = (mesmaUF ? "CFOP_DE" : "CFOP_FE");
  const QString stringEntrada = (entrada ? "cfop_entr" : "cfop_sai");
  const QString query = "SELECT " + stringUF + ", NAT FROM " + stringEntrada + " WHERE " + stringUF + " != ''";

  QSqlQuery queryCfop;

  if (not queryCfop.exec(query)) {
    emit errorSignal("Erro buscando CFOP: " + queryCfop.lastError().text());
    return false;
  }

  ui->comboBoxCfop->clear();

  ui->comboBoxCfop->addItem("");

  while (queryCfop.next()) { ui->comboBoxCfop->addItem(queryCfop.value(stringUF).toString() + " - " + queryCfop.value("NAT").toString()); }

  return true;
}

void CadastrarNFe::on_comboBoxDestinoOperacao_currentTextChanged(const QString &text) { ui->tabWidget_4->setTabEnabled(4, text == "2 Operação interestadual"); }

// TODO: 5colocar NCM para poder ser alterado na caixinha em baixo
// TODO: 3criar logo para nota
// TODO: 5verificar com Anderson rateamento de frete
// TODO: 5bloquear edicao direto na tabela
// TODO: os produtos de reposicao devem sair na nota com o valor que foram vendidos originalmente
// TODO: quando mudar a finalidade operacao para devolucao mudar as tabelas de cfop

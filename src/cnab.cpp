#include "cnab.h"
#include "ui_cnab.h"

#include "application.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QSqlError>
#include <QSqlQuery>

CNAB::CNAB(QWidget *parent) : QDialog(parent), ui(new Ui::CNAB) { ui->setupUi(this); }

CNAB::~CNAB() { delete ui; }

void CNAB::writeBlanks(QTextStream &stream, const int count) {
  for (int i = 0; i < count; ++i) { stream << " "; }
}

void CNAB::writeZeros(QTextStream &stream, const int count) {
  for (int i = 0; i < count; ++i) { stream << "0"; }
}

void CNAB::writeText(QTextStream &stream, const QString &text, const int count) {
  QString temp = text.left(count);

  // pad text with count blanks to the right
  while (temp.size() < count) { temp.append(' '); }

  stream << temp;
}

void CNAB::writeNumber(QTextStream &stream, const int number, const int count) {
  stream << QString("%1").arg(number, count, 10, QChar('0')); // pad number with count zeros to the left
}

std::optional<QString> CNAB::remessaGareSantander240(QVector<Gare> gares) {
  QString arquivo;

  QTextStream stream(&arquivo);

  QSqlQuery query;

  if (not query.exec("SELECT idCnab, MAX(sequencial) AS sequencial FROM cnab WHERE banco = 'SANTANDER' AND tipo = 'REMESSA'") or not query.first()) {
    qApp->enqueueError("Erro buscando sequencial CNAB: " + query.lastError().text(), this);
    return {};
  }

  // header arquivo pag 8

  stream << "033";            // 9(03) código do banco
  stream << "0000";           // 9(04) lote de servico
  stream << "0";              // 9(01) tipo de registro
  writeBlanks(stream, 9);     // X(09) filler
  stream << "2";              // 9(01) tipo de inscricao da empresa 1 = CPF/2 = CNPJ
  stream << "09375013000110"; // 9(14) numero inscricao da empresa
  // X(20) codigo do convenio BBBB = Numero do Banco 033 AAAA = Codigo de agencia (sem DV) CCCCCCCCCCCC = Numero do convenio (alinhado a direita com zeros a esquerda)
  writeText(stream, "00334422004900528621", 20);
  writeNumber(stream, 4422, 5);                                      // 9(05) agencia mantenedora da conta
  writeText(stream, "", 1);                                          // X(01) digito verficador da agencia
  writeNumber(stream, 13001262, 12);                                 // 9(12) numero da conta corrente
  writeText(stream, "1", 1);                                         // X(01) digito verificador da conta
  writeText(stream, " ", 1);                                         // X(01) digito verificador da agencia/conta
  writeText(stream, "STACCATO REVESTIMENTOS COM E REPRES LTDA", 30); // X(30) nome da empresa
  writeText(stream, "BANCO SANTANDER", 30);                          // X(30) nome do banco
  writeBlanks(stream, 10);                                           // X(10) filler
  stream << "1";                                                     // 9(01) codigo remessa/retorno 1 = remessa/2 = retorno
  stream << qApp->serverDate().toString("ddMMyyyy");                 // 9(08) data de geracao do arquivo DDMMAAAA
  stream << qApp->serverDateTime().toString("HHmmss");               // 9(06) hora de geracao do arquivo HHMMSS
  writeNumber(stream, query.value("sequencial").toInt(), 6);         // 9(06) numero sequencial do arquivo
  stream << "060";                                                   // 9(03) numero da versao do layout do arquivo
  stream << "00000";                                                 // 9(05) densidade de gravacao do arquivo
  writeBlanks(stream, 20);                                           // X(20) uso reservado do banco
  writeBlanks(stream, 20);                                           // X(20) uso reservado da empresa
  writeBlanks(stream, 19);                                           // X(19) filler
  writeBlanks(stream, 10);                                           // X(10) ocorrencias para retorno
  stream << "\r\n";

  // header lote pag 20

  stream << "033";            // 9(03) codigo do banco
  writeNumber(stream, 1, 4);  // 9(04) lote de servico
  stream << "1";              // 9(01) tipo de registro
  stream << "C";              // X(01) tipo da operacao C = credito
  stream << "22";             // 9(02) tipo de servico
  stream << "22";             // 9(02) forma de lancamento
  stream << "010";            // 9(03) numero da versao do lote
  writeBlanks(stream, 1);     // X(01) filler
  stream << "2";              // 9(01) tipo inscricao empresa 1 = CPF/2 = CNPJ
  stream << "09375013000110"; // 9(14) numero inscricao da empresa
  // X(20) codigo do convenio BBBB = Numero do Banco 033 AAAA = Codigo de agencia (sem DV) CCCCCCCCCCCC = Numero do convenio (alinhado a direita com zeros a esquerda)
  writeText(stream, "00334422004900528621", 20);
  writeNumber(stream, 4422, 5);                                      // 9(05) agencia mantenedora da conta
  writeText(stream, "", 1);                                          // X(01) digito verficador da agencia
  writeNumber(stream, 13001262, 12);                                 // 9(12) numero da conta corrente
  writeText(stream, "1", 1);                                         // X(01) digito verificador da conta
  writeText(stream, " ", 1);                                         // X(01) digito verificador da agencia/conta
  writeText(stream, "STACCATO REVESTIMENTOS COM E REPRES LTDA", 30); // X(30) nome da empresa
  writeText(stream, "", 40);                                         // X(40) informacao 1 - mensagem
  writeText(stream, "ALAMEDA ARAGUAIA", 30);                         // X(30) endereco
  writeNumber(stream, 661, 5);                                       // 9(05) numero
  writeText(stream, "", 15);                                         // X(15) complemento do endereco
  writeText(stream, "BARUERI", 20);                                  // X(20) cidade
  stream << "06455";                                                 // 9(05) cep
  stream << "000";                                                   // 9(03) complemento do cep
  stream << "SP";                                                    // X(02) UF
  writeBlanks(stream, 8);                                            // X(08) filler
  writeBlanks(stream, 10);                                           // X(10) ocorrencias para retorno
  stream << "\r\n";

  int registro = 0;
  int total = 0;

  for (auto &gare : gares) {
    total += gare.valor;

    // lote Segmento N pag 21 e 22

    stream << "033";                                                   // 9(03) codigo do banco
    writeNumber(stream, 1, 4);                                         // 9(04) lote de servico
    stream << "3";                                                     // 9(01) tipo de registro
    writeNumber(stream, ++registro, 5);                                // 9(05) numero sequencial registro no lote
    stream << "N";                                                     // X(01) codigo segmento reg. detalhe
    writeNumber(stream, 0, 1);                                         // 9(01) tipo de movimento
    writeNumber(stream, 0, 2);                                         // 9(02) codigo da instrucao para movimento
    writeText(stream, gare.cnpjOrig + gare.numeroNF, 20);              // X(20) numero do documento cliente
    writeText(stream, "", 20);                                         // X(20) numero documento banco
    writeText(stream, "STACCATO REVESTIMENTOS COM E REPRES LTDA", 30); // X(30) nome do contribuinte
    writeNumber(stream, gare.dataVencimento, 8);                       // 9(08) data do pagamento
    writeNumber(stream, gare.valor, 15);                               // 9(13)V2 valor total do pagamento
    writeText(stream, "0632", 6);                                      // X(06) codigo da receita do tributo
    writeNumber(stream, 2, 2);                                         // 9(02) tipo de identificacao do contribuinte
    stream << "09375013000543";                                        // 9(14) identificacao do contribuinte
    writeText(stream, "22", 2);                                        // X(02) codigo de identificacao do tributo
    writeNumber(stream, gare.dataVencimento, 8);                       // 9(08) data de vencimento
    stream << "206398781114";                                          // 9(12) inscricao estadual/codigo do municipio/numero declaracao
    writeNumber(stream, 0, 13);                                        // 9(13) divida ativa/numero etiqueta
    writeNumber(stream, gare.mesAnoReferencia, 6);                     // 9(06) periodo de referencia
    writeNumber(stream, 0, 13);                                        // 9(13) numero da parcela/notificacao
    writeNumber(stream, gare.valor, 15);                               // 9(13)V2 valor da receita
    writeNumber(stream, 0, 14);                                        // 9(12)v2 valor dos juros/encargos
    writeNumber(stream, 0, 14);                                        // 9(12)V2 valor da multa
    writeBlanks(stream, 1);                                            // X(01) filler
    writeBlanks(stream, 10);                                           // X(10) ocorrencias para retorno
    stream << "\r\n";

    // lote Segmento W pag 24

    stream << "033";                                // 9(03) codigo banco
    writeNumber(stream, 1, 4);                      // 9(04) lote de servico
    stream << "3";                                  // 9(01) tipo de registro
    writeNumber(stream, ++registro, 5);             // 9(05) numero sequencial registro no lote
    stream << "W";                                  // X(01) codigo segmento registro detalhe
    writeNumber(stream, 0, 1);                      // 9(01) numero sequencial registro complementar
    writeNumber(stream, 9, 1);                      // 9(01) identifica o uso das informacoes 1 e 2
    writeText(stream, "NFE " + gare.numeroNF, 80);  // X(80) informacao complementar 1
    writeText(stream, "CNPJ " + gare.cnpjOrig, 80); // X(80) informacao complementar 2
    writeBlanks(stream, 50);                        // X(50) informacao complementar do tributo
    writeBlanks(stream, 2);                         // X(02) filler
    writeBlanks(stream, 10);                        // X(10) ocorrencias para retorno
    stream << "\r\n";

    // lote Segmento B pag 25

    stream << "033";                             // 9(03) codigo banco
    writeNumber(stream, 1, 4);                   // 9(04) lote de servico
    stream << "3";                               // 9(01) tipo de registro
    writeNumber(stream, ++registro, 5);          // 9(05) numero sequencial registro no lote
    stream << "B";                               // X(01) codigo segmento registro detalhe
    writeBlanks(stream, 3);                      // X(03) filler
    writeNumber(stream, 2, 1);                   // 9(01) tipo de inscricao do favorecido
    stream << "09375013000543";                  // 9(14) cnpj/cpf do favorecido
    writeText(stream, "RUA SALESOPOLIS", 30);    // X(30) logradouro do favorecido
    writeNumber(stream, 27, 5);                  // 9(05) numero do local do favorecido
    writeText(stream, "", 15);                   // X(15) complemento do local favorecido
    writeText(stream, "JARDIM CALIFORNIA", 15);  // X(15) bairro do favorecido
    writeText(stream, "BARUERI", 20);            // X(20) cidade do favorecido
    writeNumber(stream, 6409150, 8);             // 9(08) cep do favorecido
    writeText(stream, "SP", 2);                  // X(02) estado do favorecido
    writeNumber(stream, gare.dataVencimento, 8); // 9(08) data de vencimento
    writeNumber(stream, gare.valor, 15);         // 9(13)V2 valor do documento
    writeNumber(stream, 0, 15);                  // 9(13)V2 valor do abatimento
    writeNumber(stream, 0, 15);                  // 9(13)V2 valor do desconto
    writeNumber(stream, 0, 15);                  // 9(13)V2 valor da mora
    writeNumber(stream, 0, 15);                  // 9(13)V2 valor da multa
    writeNumber(stream, 0, 4);                   // 9(04) horario de envio de TED
    writeBlanks(stream, 11);                     // X(11) filler
    writeNumber(stream, 0, 4);                   // 9(04) codigo historico para credito
    writeNumber(stream, 0, 1);                   // 9(01) emissao de aviso ao favorecido
    writeBlanks(stream, 1);                      // X(01) filler
    writeText(stream, "", 1);                    // X(01) TED para instituicao financeira
    writeText(stream, "", 8);                    // X(08) identificacao da IF no SPB
    stream << "\r\n";
  }

  // trailer do lote pag 26

  stream << "033";                      // 9(03) codigo banco
  writeNumber(stream, 1, 4);            // 9(04) lote de servico
  stream << "5";                        // 9(01) tipo de registro
  writeBlanks(stream, 9);               // X(09) filler
  writeNumber(stream, 2 + registro, 6); // 9(06) quantidade registros do lote
  writeNumber(stream, total, 18);       // 9(16)V2 somatoria dos valores
  writeNumber(stream, 0, 18);           // 9(13)V5 somatoria quantidade de moedas
  writeNumber(stream, 0, 6);            // 9(06) numero aviso de debito
  writeBlanks(stream, 165);             // X(165) filler
  writeBlanks(stream, 10);              // X(10) ocorrencias para retorno
  stream << "\r\n";

  // trailer do arquivo pag 33

  stream << "033";                      // 9(03) codigo banco
  stream << "9999";                     // 9(04) lote de servico
  stream << "9";                        // 9(01) tipo de registro
  writeBlanks(stream, 9);               // X(09) filler
  writeNumber(stream, 1, 6);            // 9(06) quantidade lotes do arquivo
  writeNumber(stream, 4 + registro, 6); // 9(06) quantidade registros do arquivo
  writeBlanks(stream, 211);             // X(211) filler
  stream << "\r\n";

  QDir dir(QDir::currentPath() + "/cnab/santander/");

  if (not dir.exists() and not dir.mkpath(QDir::currentPath() + "/cnab/santander/")) {
    qApp->enqueueError("Erro ao criar a pasta CNAB Santander!");
    return {};
  }

  QFile file("cnab" + query.value("sequencial").toString() + ".rem");

  if (not file.open(QFile::WriteOnly)) {
    qApp->enqueueError(file.errorString(), this);
    return {};
  }

  file.write(arquivo.toUtf8());
  file.close();

  QSqlQuery query2;

  if (not query2.exec("UPDATE cnab SET conteudo = '" + arquivo.toUtf8() + "' WHERE banco = 'SANTANDER' AND sequencial = " + query.value("sequencial").toString())) {
    qApp->enqueueError("Erro guardando CNAB: " + query2.lastError().text(), this);
    return {};
  }

  if (not query2.exec("INSERT INTO cnab (tipo, banco, sequencial) VALUES ('REMESSA', 'SANTANDER', " + QString::number(query.value("sequencial").toInt() + 1) + ")")) {
    qApp->enqueueError("Erro guardando CNAB: " + query2.lastError().text(), this);
    return {};
  }

  qApp->enqueueInformation("Arquivo gerado com sucesso: cnab" + query.value("sequencial").toString() + ".rem", this);

  return query.value("idCnab").toString();
}

void CNAB::retornoGareSantander240() {}

std::optional<QString> CNAB::remessaGareItau240(QVector<Gare> gares) {
  QString arquivo;

  QTextStream stream(&arquivo);

  QSqlQuery query;

  if (not query.exec("SELECT idCnab, MAX(sequencial) AS sequencial FROM cnab WHERE banco = 'ITAU' AND tipo = 'REMESSA'") or not query.first()) {
    qApp->enqueueError("Erro buscando sequencial CNAB: " + query.lastError().text(), this);
    return {};
  }

  // header arquivo pag 11

  stream << "341";                                                   // 9(03) código do banco na compensacao
  stream << "0000";                                                  // 9(04) lote de servico
  stream << "0";                                                     // 9(01) registro header de arquivo
  writeBlanks(stream, 6);                                            // X(06) complemento de registro brancos
  stream << "080";                                                   // 9(03) numero da versao do layout do arquivo
  stream << "2";                                                     // 9(01) tipo de inscricao da empresa 1 = CPF/2 = CNPJ
  stream << "09375013000110";                                        // 9(14) cnpj empresa debitada - NOTA 1
  writeBlanks(stream, 20);                                           // X(20) complemento de registro brancos
  stream << "04807";                                                 // 9(05) numero agencia debitada - NOTA 1
  writeBlanks(stream, 1);                                            // X(01) complemento de registro brancos
  stream << "000000045749";                                          // 9(12) numero de c/c debitada - NOTA 1
  writeBlanks(stream, 1);                                            // X(01) complemento de registro brancos
  stream << "6";                                                     // 9(01) DAC da agencia/conta debitada - NOTA 1
  writeText(stream, "STACCATO REVESTIMENTOS COM E REPRES LTDA", 30); // X(30) nome da empresa
  writeText(stream, "BANCO ITAU SA", 30);                            // X(30) nome do banco
  writeBlanks(stream, 10);                                           // X(10) complemento de registro brancos
  stream << "1";                                                     // 9(01) codigo remessa/retorno 1 = remessa/2 = retorno
  stream << qApp->serverDate().toString("ddMMyyyy");                 // 9(08) data de geracao do arquivo DDMMAAAA
  stream << qApp->serverDateTime().toString("HHmmss");               // 9(06) hora de geracao do arquivo HHMMSS
  writeZeros(stream, 9);                                             // 9(09) complemento de registro zeros
  stream << "00000";                                                 // 9(05) densidade de gravacao do arquivo - NOTA 2
  writeBlanks(stream, 69);                                           // X(69) complemento de registro brancos
  stream << "\r\n";

  // header lote pag 34

  stream << "341";                                                   // 9(03) codigo do banco
  writeNumber(stream, 1, 4);                                         // 9(04) lote identificacao de pagtos - NOTA 3
  stream << "1";                                                     // 9(01) registro header de lote
  stream << "C";                                                     // X(01) tipo da operacao C = credito
  stream << "22";                                                    // 9(02) tipo de pagto - NOTA 4
  stream << "22";                                                    // 9(02) forma de pagto - NOTA 5
  stream << "030";                                                   // 9(03) numero da versao do layout do lote
  writeBlanks(stream, 1);                                            // X(01) complemento de registro brancos
  stream << "2";                                                     // 9(01) tipo inscricao empresa debitada 1 = CPF/2 = CNPJ
  stream << "09375013000110";                                        // 9(14) cnpj empresa ou cpf debitado - NOTA 1
  writeBlanks(stream, 20);                                           // X(20) complemento de registro brancos
  stream << "04807";                                                 // 9(05) numero agencia debitada - NOTA 1
  writeBlanks(stream, 1);                                            // X(01) complemento de registro brancos
  stream << "000000045749";                                          // 9(12) numero de c/c debitada - NOTA 1
  writeBlanks(stream, 1);                                            // X(01) complemento de registro brancos
  stream << "6";                                                     // 9(01) DAC da agencia/conta debitada - NOTA 1
  writeText(stream, "STACCATO REVESTIMENTOS COM E REPRES LTDA", 30); // X(30) nome da empresa debitada
  writeBlanks(stream, 30);                                           // X(30) finalidade dos pagtos do lote - NOTA 6
  writeBlanks(stream, 10);                                           // X(10) complemento historico c/c debitada - NOTA 7
  writeText(stream, "ALAMEDA ARAGUAIA", 30);                         // X(30) nome da rua, av, pça, etc
  writeNumber(stream, 661, 5);                                       // 9(05) numero do local
  writeText(stream, "", 15);                                         // X(15) casa, apto, sala, etc
  writeText(stream, "BARUERI", 20);                                  // X(20) nome da cidade
  stream << "06455000";                                              // 9(08) cep
  stream << "SP";                                                    // X(02) sigla do estado
  writeBlanks(stream, 8);                                            // X(08) complemento de registro brancos
  writeBlanks(stream, 10);                                           // X(10) codigo ocorrencias p/ retorno ***apenas retorno, informar com branco ou zero
  stream << "\r\n";

  int registro = 0;
  int total = 0;

  for (auto &gare : gares) {
    total += gare.valor;

    // lote Segmento N pag 35

    stream << "341";                                                   // 9(03) codigo do banco
    writeNumber(stream, 1, 4);                                         // 9(04) lote de servico
    stream << "3";                                                     // 9(01) registro detalhe de lote
    writeNumber(stream, ++registro, 5);                                // 9(05) numero sequencial registro no lote
    stream << "N";                                                     // X(01) codigo segmento reg. detalhe
    stream << "000";                                                   // 9(03) tipo de movimento
    stream << "05";                                                    // 9(02) identificacao do tributo
    stream << "0632";                                                  // 9(04) codigo da receita
    stream << "2";                                                     // 9(01) tipo de inscricao do contribuinte
    stream << "09375013000543";                                        // 9(14) cpf ou cnpj do contribuinte
    stream << "206398781114";                                          // 9(12) inscricao estadual
    writeNumber(stream, 0, 13);                                        // 9(13) divida ativa/numero etiqueta
    writeNumber(stream, gare.mesAnoReferencia, 6);                     // 9(06) mes/ano de referencia
    writeNumber(stream, 0, 13);                                        // 9(13) numero parcela/notificacao
    writeNumber(stream, gare.valor, 14);                               // 9(12)V9(02) valor da receita
    writeNumber(stream, 0, 14);                                        // 9(12)V9(02) valor dos juros
    writeNumber(stream, 0, 14);                                        // 9(12)V9(02) valor da multa
    writeNumber(stream, gare.valor, 14);                               // 9(12)V9(02) valor do pagamento
    writeNumber(stream, gare.dataVencimento, 8);                       // 9(08) data de vencimento
    writeNumber(stream, gare.dataVencimento, 8);                       // 9(08) data de pagamento
    writeBlanks(stream, 11);                                           // X(11) complemento de registro
    writeText(stream, "STACCATO REVESTIMENTOS COM E REPRES LTDA", 30); // X(30) nome do contribuinte
    writeText(stream, gare.cnpjOrig + gare.numeroNF, 20);              // X(20) seu numero: numero docto atribuido pela empresa
    writeBlanks(stream, 15);                                           // X(15) nosso numero: numero atribuido pelo banco ***apenas retorno, informar com branco ou zero
    writeBlanks(stream, 10);                                           // X(10) codigo de ocorrencias p/ retorno ***apenas retorno, informar com branco ou zero
    stream << "\r\n";

    // lote Segmento B pag 36

    stream << "341";                            // 9(03) codigo banco na compensacao
    writeNumber(stream, 1, 4);                  // 9(04) lote de servico
    stream << "3";                              // 9(01) registro detalhe do lote
    writeNumber(stream, ++registro, 5);         // 9(05) numero sequencial registro no lote
    stream << "B";                              // X(01) codigo segmento registro detalhe
    writeBlanks(stream, 3);                     // X(03) complemento de registro brancos
    stream << "2";                              // 9(01) tipo de inscricao do contribuinte
    stream << "09375013000543";                 // 9(14) cpf ou cnpj do contribuinte
    writeText(stream, "RUA SALESOPOLIS", 30);   // X(30) nome da rua, av, pça, etc
    writeNumber(stream, 27, 5);                 // 9(05) numero do local
    writeText(stream, "", 15);                  // X(15) casa, apto, etc
    writeText(stream, "JARDIM CALIFORNIA", 15); // X(15) bairro
    writeText(stream, "BARUERI", 20);           // X(20) nome da cidade
    stream << "06409150";                       // 9(08) cep
    stream << "SP";                             // X(02) sigla do estado
    writeText(stream, "01141919816", 11);       // X(11) ddd e numero do telefone
    writeNumber(stream, 0, 14);                 // 9(12)V(02) valor do acrescimo
    writeNumber(stream, 0, 14);                 // 9(12)V(02) valor do honorario
    writeBlanks(stream, 74);                    // X(74) complemento de registro brancos
    stream << "\r\n";

    // lote Segmento W pag 37

    stream << "341";                                // 9(03) codigo banco na compensacao
    writeNumber(stream, 1, 4);                      // 9(04) lote de servico
    stream << "3";                                  // 9(01) registro detalhe do lote
    writeNumber(stream, ++registro, 5);             // 9(05) numero sequencial registro no lote
    stream << "W";                                  // X(01) codigo segmento registro detalhe
    writeBlanks(stream, 2);                         // X(02) complemento de registro
    writeText(stream, "NFE " + gare.numeroNF, 40);  // X(40) informacao complementar 1
    writeText(stream, "CNPJ " + gare.cnpjOrig, 40); // X(40) informacao complementar 2
    writeBlanks(stream, 40);                        // X(40) informacao complementar 3
    writeBlanks(stream, 40);                        // X(40) informacao complementar 4
    writeBlanks(stream, 64);                        // X(64) complemento de registro brancos
    stream << "\r\n";
  }

  // trailer do lote pag 39

  stream << "341";                      // 9(03) codigo banco na compensacao
  writeNumber(stream, 1, 4);            // 9(04) lote de servico
  stream << "5";                        // 9(01) registro trailer de lote
  writeBlanks(stream, 9);               // X(09) complemento de registro brancos
  writeNumber(stream, 2 + registro, 6); // 9(06) quantidade registros do lote
  writeNumber(stream, total, 14);       // 9(12)V9(02) soma valor principal dos pagtos do lote
  writeNumber(stream, 0, 14);           // 9(12)V9(02) soma valores de outras entidades do lote
  writeNumber(stream, 0, 14);           // 9(12)V9(02) soma valores atualiz. monet/multa/mora
  writeNumber(stream, total, 14);       // 9(12)V9(02) soma valor dos pagamentos do lote
  writeBlanks(stream, 151);             // X(151) complemento de registro brancos
  writeBlanks(stream, 10);              // X(10) codigos ocorrencias p/ retorno ***apenas retorno, informar com branco ou zero
  stream << "\r\n";

  // trailer do arquivo pag 40

  stream << "341";                      // 9(03) codigo banco na compensacao
  stream << "9999";                     // 9(04) lote de servico
  stream << "9";                        // 9(01) registro trailer de arquivo
  writeBlanks(stream, 9);               // X(09) complemento de registro brancos
  writeNumber(stream, 1, 6);            // 9(06) quantidade lotes do arquivo - NOTA 17
  writeNumber(stream, 4 + registro, 6); // 9(06) quantidade registros do arquivo - NOTA 17
  writeBlanks(stream, 211);             // X(211) complemento de registro brancos
  stream << "\r\n";

  QDir dir(QDir::currentPath() + "/cnab/itau/");

  if (not dir.exists() and not dir.mkpath(QDir::currentPath() + "/cnab/itau/")) {
    qApp->enqueueError("Erro ao criar a pasta CNAB Itau!");
    return {};
  }

  QFile file(QDir::currentPath() + "/cnab/itau/cnab" + query.value("sequencial").toString() + ".rem");

  if (not file.open(QFile::WriteOnly)) {
    qApp->enqueueError(file.errorString(), this);
    return {};
  }

  file.write(arquivo.toUtf8());
  file.close();

  QSqlQuery query2;

  if (not query2.exec("UPDATE cnab SET conteudo = '" + arquivo.toUtf8() + "' WHERE banco = 'ITAU' AND sequencial = " + query.value("sequencial").toString())) {
    qApp->enqueueError("Erro guardando CNAB: " + query2.lastError().text(), this);
    return {};
  }

  if (not query2.exec("INSERT INTO cnab (tipo, banco, sequencial) VALUES ('REMESSA', 'ITAU', " + QString::number(query.value("sequencial").toInt() + 1) + ")")) {
    qApp->enqueueError("Erro guardando CNAB: " + query2.lastError().text(), this);
    return {};
  }

  qApp->enqueueInformation("Arquivo gerado com sucesso: cnab" + query.value("sequencial").toString() + ".rem", this);

  return query.value("idCnab").toString();
}

void CNAB::retornoGareItau240(const QString &filePath) {
  QFile file(filePath);

  if (not file.open(QFile::ReadOnly)) { return qApp->enqueueError("Erro lendo arquivo: " + file.errorString()); }

  QStringList lines;

  while (not file.atEnd()) { lines << file.readLine(); }

  file.close();

  QStringList resultado;

  if (not qApp->startTransaction("CNAB::retornoGareItau240")) { return; }

  for (auto const &line : lines) {
    // ocorrencias do header lote
    if (line.at(7) == '1') {

      QString ocorrencia1 = decodeCodeItau(line.mid(230, 2));
      if (not ocorrencia1.isEmpty()) { ocorrencia1.prepend("    -"); }
      QString ocorrencia2 = decodeCodeItau(line.mid(232, 2));
      if (not ocorrencia1.isEmpty() and not ocorrencia2.isEmpty()) { ocorrencia2.prepend("\n    -"); }
      QString ocorrencia3 = decodeCodeItau(line.mid(234, 2));
      if (not ocorrencia2.isEmpty() and not ocorrencia3.isEmpty()) { ocorrencia3.prepend("\n    -"); }
      QString ocorrencia4 = decodeCodeItau(line.mid(236, 2));
      if (not ocorrencia3.isEmpty() and not ocorrencia4.isEmpty()) { ocorrencia4.prepend("\n    -"); }
      QString ocorrencia5 = decodeCodeItau(line.mid(238, 2));
      if (not ocorrencia4.isEmpty() and not ocorrencia5.isEmpty()) { ocorrencia5.prepend("\n    -"); }

      const QString header = "Ocorrências header:\n" + ocorrencia1 + ocorrencia2 + ocorrencia3 + ocorrencia4 + ocorrencia5;

      if (not ocorrencia1.isEmpty()) { resultado << header; }
    }

    // ocorrencias do segmento N
    if (line.at(13) == 'N') {

      QString cnpj = line.mid(195, 14);
      QString nfe = line.mid(209, 6).trimmed();

      QString dataPgt = line.mid(150, 4) + "-" + line.mid(148, 2) + "-" + line.mid(146, 2); // DDMMAAAA

      QString ocorrencia1 = decodeCodeItau(line.mid(230, 2));
      if (not ocorrencia1.isEmpty()) { ocorrencia1.prepend("    -"); }
      QString ocorrencia2 = decodeCodeItau(line.mid(232, 2));
      if (not ocorrencia1.isEmpty() and not ocorrencia2.isEmpty()) { ocorrencia2.prepend("\n    -"); }
      QString ocorrencia3 = decodeCodeItau(line.mid(234, 2));
      if (not ocorrencia2.isEmpty() and not ocorrencia3.isEmpty()) { ocorrencia3.prepend("\n    -"); }
      QString ocorrencia4 = decodeCodeItau(line.mid(236, 2));
      if (not ocorrencia3.isEmpty() and not ocorrencia4.isEmpty()) { ocorrencia4.prepend("\n    -"); }
      QString ocorrencia5 = decodeCodeItau(line.mid(238, 2));
      if (not ocorrencia4.isEmpty() and not ocorrencia5.isEmpty()) { ocorrencia5.prepend("\n    -"); }

      const QString segmentoN = "CNPJ: " + cnpj + " NFe: " + nfe + " Status:\n" + ocorrencia1 + ocorrencia2 + ocorrencia3 + ocorrencia4 + ocorrencia5;

      if (not ocorrencia1.isEmpty()) { resultado << segmentoN; }

      if (segmentoN.contains("PAGAMENTO EFETUADO")) {
        QSqlQuery query1;

        if (not query1.exec("SELECT idNFe FROM nfe WHERE numeroNFe = " + nfe + " AND cnpjOrig = " + cnpj) or not query1.first()) {
          qApp->enqueueError("Erro dando baixa na GARE: " + query1.lastError().text(), this);
          return qApp->rollbackTransaction();
        }

        QSqlQuery query2;

        if (not query2.exec("UPDATE conta_a_pagar_has_pagamento SET status = 'PAGO GARE', valorReal = valor, dataRealizado = '" + dataPgt + "' WHERE idNFe = " + query1.value("idNFe").toString())) {
          qApp->enqueueError("Erro dando baixa na GARE: " + query2.lastError().text(), this);
          return qApp->rollbackTransaction();
        }
      }
    }

    // ocorrencias do trailer lote
    if (line.at(7) == '5') {

      QString ocorrencia1 = decodeCodeItau(line.mid(230, 2));
      if (not ocorrencia1.isEmpty()) { ocorrencia1.prepend("    -"); }
      QString ocorrencia2 = decodeCodeItau(line.mid(232, 2));
      if (not ocorrencia1.isEmpty() and not ocorrencia2.isEmpty()) { ocorrencia2.prepend("\n    -"); }
      QString ocorrencia3 = decodeCodeItau(line.mid(234, 2));
      if (not ocorrencia2.isEmpty() and not ocorrencia3.isEmpty()) { ocorrencia3.prepend("\n    -"); }
      QString ocorrencia4 = decodeCodeItau(line.mid(236, 2));
      if (not ocorrencia3.isEmpty() and not ocorrencia4.isEmpty()) { ocorrencia4.prepend("\n    -"); }
      QString ocorrencia5 = decodeCodeItau(line.mid(238, 2));
      if (not ocorrencia4.isEmpty() and not ocorrencia5.isEmpty()) { ocorrencia5.prepend("\n    -"); }

      const QString trailer = "Ocorrências trailer:\n" + ocorrencia1 + ocorrencia2 + ocorrencia3 + ocorrencia4 + ocorrencia5;

      if (not ocorrencia1.isEmpty()) { resultado << trailer; }
    }
  }

  QSqlQuery query2;

  if (not query2.exec("INSERT INTO cnab (tipo, banco, conteudo) VALUES ('RETORNO', 'ITAU', '" + lines.join("") + "')")) {
    qApp->enqueueError("Erro guardando CNAB: " + query2.lastError().text(), this);
    qApp->rollbackTransaction();
  }

  if (not qApp->endTransaction()) { return; }

  qApp->enqueueInformation(resultado.join("\n"), this);
}

QString CNAB::decodeCodeItau(const QString &code) {
  if (code == "00") { return "PAGAMENTO EFETUADO"; }
  if (code == "AE") { return "DATA DE PAGAMENTO ALTERADA"; }
  if (code == "AG") { return "NÚMERO DO LOTE INVÁLIDO"; }
  if (code == "AH") { return "NÚMERO SEQUENCIAL DO REGISTRO NO LOTE INVÁLIDO"; }
  if (code == "AI") { return "PRODUTO DEMONSTRATIVO DE PAGAMENTO NÃO CONTRATADO"; }
  if (code == "AJ") { return "TIPO DE MOVIMENTO INVÁLIDO"; }
  if (code == "AL") { return "CÓDIGO DO BANCO FAVORECIDO INVÁLIDO"; }
  if (code == "AM") { return "AGÊNCIA DO FAVORECIDO INVÁLIDA"; }
  if (code == "AN") { return "CONTA CORRENTE DO FAVORECIDO INVÁLIDA"; }
  if (code == "AO") { return "NOME DO FAVORECIDO INVÁLIDO"; }
  if (code == "AP") { return "DATA DE PAGAMENTO / DATA DE VALIDADE / HORA DE LANÇAMENTO / ARRECADAÇÃO / APURAÇÃO INVÁLIDA"; }
  if (code == "AQ") { return "QUANTIDADE DE REGISTROS MAIOR QUE 999999"; }
  if (code == "AR") { return "VALOR ARRECADADO / LANÇAMENTO INVÁLIDO"; }
  if (code == "BC") { return "NOSSO NÚMERO INVÁLIDO"; }
  if (code == "BD") { return "PAGAMENTO AGENDADO"; }
  if (code == "BE") { return "PAGAMENTO AGENDADO COM FORMA ALTERADA PARA OP"; }
  if (code == "BI") { return "CNPJ / CPF DO FAVORECIDO NO SEGMENTO J-52 OU B INVÁLIDO"; }
  if (code == "BL") { return "VALOR DA PARCELA INVÁLIDO"; }
  if (code == "CD") { return "CNPJ / CPF DIVERGENTE DO CADASTRADO"; }
  if (code == "CE") { return "PAGAMENTO CANCELADO"; }
  if (code == "CF") { return "VALOR DO DOCUMENTO INVÁLIDO"; }
  if (code == "CG") { return "VALOR DO ABATIMENTO INVÁLIDO"; }
  if (code == "CH") { return "VALOR DO DESCONTO INVÁLIDO"; }
  if (code == "CI") { return "CNPJ / CPF / IDENTIFICADOR / INSCRIÇÃO ESTADUAL / INSCRIÇÃO NO CAD / ICMS INVÁLIDO"; }
  if (code == "CJ") { return "VALOR DA MULTA INVÁLIDO"; }
  if (code == "CK") { return "TIPO DE INSCRIÇÃO INVÁLIDA"; }
  if (code == "CL") { return "VALOR DO INSS INVÁLIDO"; }
  if (code == "CM") { return "VALOR DO COFINS INVÁLIDO"; }
  if (code == "CN") { return "CONTA NÃO CADASTRADA"; }
  if (code == "CO") { return "VALOR DE OUTRAS ENTIDADES INVÁLIDO"; }
  if (code == "CP") { return "CONFIRMAÇÃO DE OP CUMPRIDA"; }
  if (code == "CQ") { return "SOMA DAS FATURAS DIFERE DO PAGAMENTO"; }
  if (code == "CR") { return "VALOR DO CSLL INVÁLIDO"; }
  if (code == "CS") { return "DATA DE VENCIMENTO DA FATURA INVÁLIDA"; }
  if (code == "DA") { return "NÚMERO DE DEPEND. SALÁRIO FAMÍLIA INVÁLIDO"; }
  if (code == "DB") { return "NÚMERO DE HORAS SEMANAIS INVÁLIDO"; }
  if (code == "DC") { return "SALÁRIO DE CONTRIBUIÇÃO INSS INVÁLIDO"; }
  if (code == "DD") { return "SALÁRIO DE CONTRIBUIÇÃO FGTS INVÁLIDO"; }
  if (code == "DE") { return "VALOR TOTAL DOS PROVENTOS INVÁLIDO"; }
  if (code == "DF") { return "VALOR TOTAL DOS DESCONTOS INVÁLIDO"; }
  if (code == "DG") { return "VALOR LÍQUIDO NÃO NÚMERICO"; }
  if (code == "DH") { return "VALOR LIQ. INFORMADO DIFERE DO CALCULADO"; }
  if (code == "DI") { return "VALOR DO SALÁRIO-BASE INVÁLIDO"; }
  if (code == "DJ") { return "BASE DE CÁLCULO IRRF INVÁLIDA"; }
  if (code == "DK") { return "BASE DE CÁLCULO FGTS INVÁLIDA"; }
  if (code == "DL") { return "FORMA DE PAGAMENTO INCOMPATÍVEL COM HOLERITE"; }
  if (code == "DM") { return "E-MAIL DO FAVORECIDO INVÁLIDO"; }
  if (code == "DV") { return "DOC / TED DEVOLVIDO PELO BANCO FAVORECIDO"; }
  if (code == "D0") { return "FINALIDADE DO HOLERITE INVÁLIDA"; }
  if (code == "D1") { return "MÊS DE COMPETÊNCIA DO HOLERITE INVÁLIDA"; }
  if (code == "D2") { return "DIA DA COMPETÊNCIA DO HOLERITE INVÁLIDA"; }
  if (code == "D3") { return "CENTRO DE CUSTO INVÁLIDO"; }
  if (code == "D4") { return "CAMPO NUMÉRICO DA FUNCIONAL INVÁLIDO"; }
  if (code == "D5") { return "DATA INÍCIO DE FÉRIAS NÃO NUMÉRICA"; }
  if (code == "D6") { return "DATA INÍCIO DE FÉRIAS INCONSISTENTE"; }
  if (code == "D7") { return "DATA FIM DE FÉRIAS NÃO NUMÉRICA"; }
  if (code == "D8") { return "DATA FIM DE FÉRIAS INCONSISTENTE"; }
  if (code == "D9") { return "NÚMERO DE DEPENDENTES IR INVÁLIDO"; }
  if (code == "EM") { return "CONFIRMAÇÃO DE OP EMITIDA"; }
  if (code == "EX") { return "DEVOLUÇÃO DE OP NÃO SACADA PELO FAVORECIDO"; }
  if (code == "E0") { return "TIPO DE MOVIMENTO HOLERITE INVÁLIDO"; }
  if (code == "E1") { return "VALOR 01 DO HOLERITE / INFORME INVÁLIDO"; }
  if (code == "E2") { return "VALOR 02 DO HOLERITE / INFORME INVÁLIDO"; }
  if (code == "E3") { return "VALOR 03 DO HOLERITE / INFORME INVÁLIDO"; }
  if (code == "E4") { return "VALOR 04 DO HOLERITE / INFORME INVÁLIDO"; }
  if (code == "FC") { return "PAGAMENTO EFETUADO ATRAVÉS DE FINANCIAMENTO COMPROR"; }
  if (code == "FD") { return "PAGAMENTO EFETUADO ATRAVÉS DE FINANCIAMENTO DESCOMPROR"; }
  if (code == "HÁ") { return "ERRO NO LOTE"; }
  if (code == "HM") { return "ERRO NO REGISTRO HEADER DE ARQUIVO"; }
  if (code == "IB") { return "VALOR DO DOCUMENTO INVÁLIDO"; }
  if (code == "IC") { return "VALOR DO ABATIMENTO INVÁLIDO"; }
  if (code == "ID") { return "VALOR DO DESCONTO INVÁLIDO"; }
  if (code == "IE") { return "VALOR DA MORA INVÁLIDO"; }
  if (code == "IF") { return "VALOR DA MULTA INVÁLIDO"; }
  if (code == "IG") { return "VALOR DA DEDUÇÃO INVÁLIDO"; }
  if (code == "IH") { return "VALOR DO ACRÉSCIMO INVÁLIDO"; }
  if (code == "II") { return "DATA DE VENCIMENTO INVÁLIDA"; }
  if (code == "IJ") { return "COMPETÊNCIA / PERÍODO REFERÊNCIA / PARCELA INVÁLIDA"; }
  if (code == "IK") { return "TRIBUTO NÃO LIQUIDÁVEL VIA SISPAG OU NÃO CONVENIADO COM ITAÚ"; }
  if (code == "IL") { return "CÓDIGO DE PAGAMENTO / EMPRESA / RECEITA INVÁLIDO"; }
  if (code == "IM") { return "TIPO X FORMA NÃO COMPATÍVEL"; }
  if (code == "IN") { return "BANCO / AGÊNCIA NÃO CADASTRADOS"; }
  if (code == "IO") { return "DAC / VALOR / COMPETÊNCIA / IDENTIFICADO DO LACRE INVÁLIDO"; }
  if (code == "IP") { return "DAC DO CÓDIGO DE BARRAS INVÁLIDO"; }
  if (code == "IQ") { return "DÍVIDA ATIVA OU NÚMERO DE ETIQUETA INVÁLIDO"; }
  if (code == "IR") { return "PAGAMENTO ALTERADO"; }
  if (code == "IS") { return "CONCESSIONÁRIA NÃO CONVENIADA COM ITAÚ"; }
  if (code == "IT") { return "VALOR DO TRIBUTO INVÁLIDO"; }
  if (code == "IU") { return "VALOR DA RECEITA BRUTA ACUMULADA INVÁLIDO"; }
  if (code == "IV") { return "NÚMERO DO DOCUMENTO ORIGEM / REFERÊNCIA INVÁLIDO"; }
  if (code == "IX") { return "CÓDIGO DO PRODUTO INVÁLIDO"; }
  if (code == "LA") { return "DATA DE PAGAMENTO DE UM LOTE ALTERADA"; }
  if (code == "LC") { return "LOTE DE PAGAMENTOS CANCELADO"; }
  if (code == "NA") { return "PAGAMENTO CANCELADO POR FALTA DE AUTORIZAÇÃO"; }
  if (code == "NB") { return "IDENTIFICAÇÃO DO TRIBUTO INVÁLIDA"; }
  if (code == "NC") { return "EXERCÍCIO (ANO BASE) INVÁLIDO"; }
  if (code == "ND") { return "CÓDIGO RENAVAM NÃO ENCONTRADO / INVÁLIDO"; }
  if (code == "NE") { return "UF INVÁLIDA"; }
  if (code == "NF") { return "CÓDIGO DO MUNICÍPIO INVÁLIDO"; }
  if (code == "NG") { return "PLACA INVÁLIDA"; }
  if (code == "NH") { return "OPÇÃO / PARCELA DE PAGAMENTO INVÁLIDA"; }
  if (code == "NI") { return "TRIBUTO JÁ FOI PAGO OU ESTÁ VENCIDO"; }
  if (code == "NR") { return "OPERAÇÃO NÃO REALIZADA"; }
  if (code == "PD") { return "AQUISIÇÃO CONFIRMADA (EQUIVALE A OCORRÊNCIA 02 NO LAYOUT DE RISCO SACADO)"; }
  if (code == "RJ") { return "REGISTRO REJEITADO"; }
  if (code == "RS") { return "PAGAMENTO DISPONÍVEL PARA ANTECIPAÇÃO NO RISCO SACADO - MODALIDADE RISCO SACADO PÓS AUTORIZADO"; }
  if (code == "SS") { return "PAGAMENTO CANCELADO POR INSUFICIÊNCIA DE SALDO / LIMITE DIÁRIO DE PAGTO"; }
  if (code == "TA") { return "LOTE NÃO ACEITO - TOTAIS DO LOTE COM DIFERENÇA"; }
  if (code == "TI") { return "TITULARIDADE INVÁLIDA"; }
  if (code == "X1") { return "FORMA INCOMPATÍVEL COM LAYOUT 010"; }
  if (code == "X2") { return "NÚMERO DA NOTA FISCAL INVÁLIDO"; }
  if (code == "X3") { return "IDENTIFICADOR DE NF / CNPJ INVÁLIDO"; }
  if (code == "X4") { return "FORMA 32 INVÁLIDA"; }

  return "";
}

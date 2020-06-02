#include "cnab.h"
#include "ui_cnab.h"

#include "application.h"

#include <QDebug>
#include <QFile>

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

void CNAB::remessaGareSantander240(QVector<Gare> gares) {
  QString arquivo;

  QTextStream stream(&arquivo);

  // TODO: guardar numero sequencial
  // TODO: verificar qual o numero do convenio com o banco

  // header arquivo pag 8

  stream << "033";            // 9(03) código do banco
  stream << "0000";           // 9(04) lote de servico
  stream << "0";              // 9(01) tipo de registro
  writeBlanks(stream, 9);     // X(09) filler
  stream << "2";              // 9(01) tipo de inscricao da empresa 1 = CPF/2 = CNPJ
  stream << "09375013000110"; // 9(14) numero inscricao da empresa
  // X(20) codigo do convenio BBBB = Numero do Banco 033 AAAA = Codigo de agencia (sem DV) CCCCCCCCCCCC = Numero do convenio (alinhado a direita com zeros a esquerda)
  writeText(stream, "BBBBAAAACCCCCCCCCCCC", 20);
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
  writeNumber(stream, -1, 6);                                        // 9(06) numero sequencial do arquivo
  stream << "060";                                                   // 9(03) numero da versao do layout do arquivo
  stream << "00000";                                                 // 9(05) densidade de gravacao do arquivo
  writeBlanks(stream, 20);                                           // X(20) uso reservado do banco
  writeBlanks(stream, 20);                                           // X(20) uso reservado da empresa
  writeBlanks(stream, 19);                                           // X(19) filler
  writeBlanks(stream, 10);                                           // X(10) ocorrencias para retorno
  stream << endl;

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
  stream << "09375013000543"; // 9(14) numero inscricao da empresa
  // X(20) codigo do convenio BBBB = Numero do Banco 033 AAAA = Codigo de agencia (sem DV) CCCCCCCCCCCC = Numero do convenio (alinhado a direita com zeros a esquerda)
  writeText(stream, "BBBBAAAACCCCCCCCCCCC", 20);
  writeNumber(stream, 4422, 5);                                      // 9(05) agencia mantenedora da conta
  writeText(stream, "", 1);                                          // X(01) digito verficador da agencia
  writeNumber(stream, 13001262, 12);                                 // 9(12) numero da conta corrente
  writeText(stream, "1", 1);                                         // X(01) digito verificador da conta
  writeText(stream, " ", 1);                                         // X(01) digito verificador da agencia/conta
  writeText(stream, "STACCATO REVESTIMENTOS COM E REPRES LTDA", 30); // X(30) nome da empresa
  writeText(stream, "", 40);                                         // X(40) informacao 1 - mensagem
  writeText(stream, "AV ANDROMEDA", 30);                             // X(30) endereco
  writeNumber(stream, 900, 5);                                       // 9(05) numero
  writeText(stream, "LOTE 13 QUADRA A", 15);                         // X(15) complemento do endereco
  writeText(stream, "BARUERI", 20);                                  // X(20) cidade
  stream << "06473000";                                              // 9(05) cep
  stream << "06473000";                                              // 9(03) complemento do cep
  stream << "SP";                                                    // X(02) UF
  writeBlanks(stream, 8);                                            // X(08) filler
  writeBlanks(stream, 10);                                           // X(10) ocorrencias para retorno
  stream << endl;

  int registro = 0;
  int total = 0;

  for (auto &gare : gares) {
    total += gare.valor;

    // lote Segmento N pag 21 e 22

    stream << "033";                    // 9(03) codigo do banco
    writeNumber(stream, 1, 4);          // 9(04) lote de servico
    stream << "3";                      // 9(01) tipo de registro
    writeNumber(stream, ++registro, 5); // 9(05) numero sequencial registro no lote
    stream << "N";                      // X(01) codigo segmento reg. detalhe
    writeNumber(stream, 0, 1);          // 9(01) tipo de movimento
    writeNumber(stream, 0, 2);          // 9(02) codigo da instrucao para movimento
    writeText(stream, "-1", 20);        // X(20) numero do documento cliente
    writeText(stream, "-1", 20);        // X(20) numero documento banco
    writeText(stream, "-1", 30);        // X(30) nome do contribuinte
    writeNumber(stream, -1, 8);         // 9(08) data do pagamento
    writeNumber(stream, -1, 13);        // 9(13)V2 valor total do pagamento
    writeText(stream, "-1", 6);         // X(06) codigo da receita do tributo
    writeNumber(stream, -1, 2);         // 9(02) tipo de identificacao do contribuinte
    writeNumber(stream, -1, 14);        // 9(14) identificacao do contribuinte
    writeText(stream, "-1", 2);         // X(02) codigo de identificacao do tributo
    writeNumber(stream, -1, 8);         // 9(08) data de vencimento
    writeNumber(stream, -1, 12);        // 9(12) inscricao estadual/codigo do municipio/numero declaracao
    writeNumber(stream, -1, 13);        // 9(13) divida ativa/numero etiqueta
    writeNumber(stream, -1, 6);         // 9(06) periodo de referencia
    writeNumber(stream, -1, 13);        // 9(13) numero da parcela/notificacao
    writeNumber(stream, -1, 15);        // 9(13)V2 valor da receita
    writeNumber(stream, -1, 14);        // 9(12)v2 valor dos juros/encargos
    writeNumber(stream, -1, 14);        // 9(12)V2 valor da multa
    writeBlanks(stream, 1);             // X(01) filler
    writeBlanks(stream, 10);            // X(10) ocorrencias para retorno
    stream << endl;

    // lote Segmento W pag 24

    stream << "033";                                // 9(03) codigo banco
    writeNumber(stream, 1, 4);                      // 9(04) lote de servico
    stream << "3";                                  // 9(01) tipo de registro
    writeNumber(stream, registro, 5);               // 9(05) numero sequencial registro no lote
    stream << "W";                                  // X(01) codigo segmento registro detalhe
    writeNumber(stream, -1, 1);                     // 9(01) numero sequencial registro complementar
    writeNumber(stream, -1, 1);                     // 9(01) identifica o uso das informacoes 1 e 2
    writeText(stream, "NFE " + gare.numeroNF, 80);  // X(80) informacao complementar 1
    writeText(stream, "CNPJ " + gare.cnpjOrig, 80); // X(80) informacao complementar 2
    writeBlanks(stream, 50);                        // X(50) informacao complementar do tributo
    writeBlanks(stream, 2);                         // X(02) filler
    writeBlanks(stream, 10);                        // X(10) ocorrencias para retorno
    stream << endl;

    // lote Segmento B pag 25

    stream << "033";                  // 9(03) codigo banco
    writeNumber(stream, 1, 4);        // 9(04) lote de servico
    stream << "3";                    // 9(01) tipo de registro
    writeNumber(stream, registro, 5); // 9(05) numero sequencial registro no lote
    stream << "B";                    // X(01) codigo segmento registro detalhe
    writeBlanks(stream, 3);           // X(03) filler
    writeNumber(stream, 2, 1);        // 9(01) tipo de inscricao do favorecido
    writeNumber(stream, -1, 14);      // 9(14) cnpj/cpf do favorecido
    writeText(stream, "-1", 30);      // X(30) logradouro do favorecido
    writeNumber(stream, -1, 5);       // 9(05) numero do local do favorecido
    writeText(stream, "-1", 15);      // X(15) complemento do local favorecido
    writeText(stream, "-1", 15);      // X(15) bairro do favorecido
    writeText(stream, "-1", 20);      // X(20) cidade do favorecido
    writeNumber(stream, -1, 8);       // 9(08) cep do favorecido
    writeText(stream, "-1", 2);       // X(02) estado do favorecido
    writeNumber(stream, -1, 8);       // 9(08) data de vencimento
    writeNumber(stream, -1, 15);      // 9(13)V2 valor do documento
    writeNumber(stream, -1, 15);      // 9(13)V2 valor do abatimento
    writeNumber(stream, -1, 15);      // 9(13)V2 valor do desconto
    writeNumber(stream, -1, 15);      // 9(13)V2 valor da mora
    writeNumber(stream, -1, 15);      // 9(13)V2 valor da multa
    writeNumber(stream, -1, 4);       // 9(04) horario de envio de TED
    writeBlanks(stream, 11);          // X(11) filler
    writeNumber(stream, -1, 4);       // 9(04) codigo historico para credito
    writeNumber(stream, -1, 1);       // 9(01) emissao de aviso ao favorecido
    writeBlanks(stream, 1);           // X(01) filler
    writeText(stream, "-1", 1);       // X(01) TED para instituicao financeira
    writeText(stream, "-1", 8);       // X(08) identificacao da IF no SPB
    stream << endl;
  }

  // trailer do lote pag 26

  stream << "033";                            // 9(03) codigo banco
  writeNumber(stream, 1, 4);                  // 9(04) lote de servico
  stream << "5";                              // 9(01) tipo de registro
  writeBlanks(stream, 9);                     // X(09) filler
  writeNumber(stream, 2 + (registro * 3), 6); // 9(06) quantidade registros do lote
  writeNumber(stream, total, 14);             // 9(16)V2 somatoria dos valores
  writeNumber(stream, 0, 14);                 // 9(13)V5 somatoria quantidade de moedas
  writeNumber(stream, -1, 14);                // 9(06) numero aviso de debito
  writeNumber(stream, total, 14);             // X(165) filler
  writeBlanks(stream, 10);                    // X(10) ocorrencias para retorno
  stream << endl;

  // trailer do arquivo pag 33

  stream << "033";                            // 9(03) codigo banco
  stream << "9999";                           // 9(04) lote de servico
  stream << "9";                              // 9(01) tipo de registro
  writeBlanks(stream, 9);                     // X(09) filler
  writeNumber(stream, 1, 6);                  // 9(06) quantidade lotes do arquivo
  writeNumber(stream, 4 + (registro * 3), 6); // 9(06) quantidade registros do arquivo
  writeBlanks(stream, 211);                   // X(211) filler
  stream << endl;

  QFile file("cnab.txt"); // TODO: alterar para numeroSequencial.rem

  if (not file.open(QFile::WriteOnly)) { return qApp->enqueueError(file.errorString(), this); }

  file.write(arquivo.toUtf8());
  file.close();
}

void CNAB::retornoGareSantander240() {}

void CNAB::remessaGareItau240(QVector<Gare> gares) {
  QString arquivo;

  QTextStream stream(&arquivo);

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
  stream << "09375013000543";                                        // 9(14) cnpj empresa ou cpf debitado - NOTA 1
  writeBlanks(stream, 20);                                           // X(20) complemento de registro brancos
  stream << "04807";                                                 // 9(05) numero agencia debitada - NOTA 1
  writeBlanks(stream, 1);                                            // X(01) complemento de registro brancos
  stream << "000000045749";                                          // 9(12) numero de c/c debitada - NOTA 1
  writeBlanks(stream, 1);                                            // X(01) complemento de registro brancos
  stream << "6";                                                     // 9(01) DAC da agencia/conta debitada - NOTA 1
  writeText(stream, "STACCATO REVESTIMENTOS COM E REPRES LTDA", 30); // X(30) nome da empresa debitada
  writeBlanks(stream, 30);                                           // X(30) finalidade dos pagtos do lote - NOTA 6
  writeBlanks(stream, 10);                                           // X(10) complemento historico c/c debitada - NOTA 7
  writeText(stream, "AV ANDROMEDA", 30);                             // X(30) nome da rua, av, pça, etc
  writeNumber(stream, 900, 5);                                       // 9(05) numero do local
  writeText(stream, "LOTE 13 QUADRA A", 15);                         // X(15) casa, apto, sala, etc
  writeText(stream, "BARUERI", 20);                                  // X(20) nome da cidade
  stream << "06473000";                                              // 9(08) cep
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

  stream << "341";                            // 9(03) codigo banco na compensacao
  writeNumber(stream, 1, 4);                  // 9(04) lote de servico
  stream << "5";                              // 9(01) registro trailer de lote
  writeBlanks(stream, 9);                     // X(09) complemento de registro brancos
  writeNumber(stream, 2 + (registro * 3), 6); // 9(06) quantidade registros do lote
  writeNumber(stream, total, 14);             // 9(12)V9(02) soma valor principal dos pagtos do lote
  writeNumber(stream, 0, 14);                 // 9(12)V9(02) soma valores de outras entidades do lote
  writeNumber(stream, 0, 14);                 // 9(12)V9(02) soma valores atualiz. monet/multa/mora
  writeNumber(stream, total, 14);             // 9(12)V9(02) soma valor dos pagamentos do lote
  writeBlanks(stream, 151);                   // X(151) complemento de registro brancos
  writeBlanks(stream, 10);                    // X(10) codigos ocorrencias p/ retorno ***apenas retorno, informar com branco ou zero
  stream << "\r\n";

  // trailer do arquivo pag 40

  stream << "341";                            // 9(03) codigo banco na compensacao
  stream << "9999";                           // 9(04) lote de servico
  stream << "9";                              // 9(01) registro trailer de arquivo
  writeBlanks(stream, 9);                     // X(09) complemento de registro brancos
  writeNumber(stream, 1, 6);                  // 9(06) quantidade lotes do arquivo - NOTA 17
  writeNumber(stream, 4 + (registro * 3), 6); // 9(06) quantidade registros do arquivo - NOTA 17
  writeBlanks(stream, 211);                   // X(211) complemento de registro brancos
  stream << "\r\n";

  QFile file("cnab.txt"); // TODO: alterar para numeroSequencial.rem

  if (not file.open(QFile::WriteOnly)) { return qApp->enqueueError(file.errorString(), this); }

  file.write(arquivo.toUtf8());
  file.close();
}

void CNAB::retornoGareItau240() {}

// TODO: alterar endereco da matriz para o endereco novo

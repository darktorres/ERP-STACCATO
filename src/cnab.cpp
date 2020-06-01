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

void CNAB::remessaGareSantander240(QVector<Gare> gares) {}

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
    writeNumber(stream, registro, 5);           // 9(05) numero sequencial registro no lote
    stream << "B";                              // X(01) codigo segmento registro detalhe
    writeBlanks(stream, 18);                    // X(18) complemento de registro brancos
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
    writeNumber(stream, registro, 5);               // 9(05) numero sequencial registro no lote
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

  QFile file("cnab.txt");

  if (not file.open(QFile::WriteOnly)) {
    qDebug() << "erro: " << file.errorString();
    return;
  }

  file.write(arquivo.toUtf8());
  file.close();
}

void CNAB::retornoGareItau240() {}

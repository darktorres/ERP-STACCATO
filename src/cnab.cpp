#include "cnab.h"

#include "application.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QSqlError>
#include <QSqlQuery>

CNAB::CNAB(QWidget *parent) : parent(parent) {}

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

void CNAB::writeNumber(QTextStream &stream, const ulong number, const int count) {
  stream << QString("%1").arg(number, count, 10, QChar('0')); // pad number with count zeros to the left
}

std::optional<QString> CNAB::remessaGareItau240(QVector<Gare> gares) {
  QString arquivo;

  QTextStream stream(&arquivo);

  QSqlQuery query;

  if (not query.exec("SELECT idCnab, MAX(sequencial) AS sequencial FROM cnab WHERE banco = 'ITAU' AND tipo = 'REMESSA'") or not query.first()) {
    qApp->enqueueException("Erro buscando sequencial CNAB: " + query.lastError().text(), parent);
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
  ulong total = 0;

  for (auto &gare : gares) {
    total += gare.valor;

    // lote Segmento N pag 35 e 68

    stream << "341";                                                      // 9(03) codigo do banco
    writeNumber(stream, 1, 4);                                            // 9(04) lote de servico
    stream << "3";                                                        // 9(01) registro detalhe de lote
    writeNumber(stream, ++registro, 5);                                   // 9(05) numero sequencial registro no lote
    stream << "N";                                                        // X(01) codigo segmento reg. detalhe
    stream << "000";                                                      // 9(03) tipo de movimento
    stream << "05";                                                       // 9(02) identificacao do tributo
    stream << "0632";                                                     // 9(04) codigo da receita
    stream << "2";                                                        // 9(01) tipo de inscricao do contribuinte
    stream << "09375013000543";                                           // 9(14) cpf ou cnpj do contribuinte
    stream << "206398781114";                                             // 9(12) inscricao estadual
    writeNumber(stream, 0, 13);                                           // 9(13) divida ativa/numero etiqueta
    writeNumber(stream, gare.mesAnoReferencia, 6);                        // 9(06) mes/ano de referencia
    writeNumber(stream, 0, 13);                                           // 9(13) numero parcela/notificacao
    writeNumber(stream, gare.valor, 14);                                  // 9(12)V9(02) valor da receita
    writeNumber(stream, 0, 14);                                           // 9(12)V9(02) valor dos juros
    writeNumber(stream, 0, 14);                                           // 9(12)V9(02) valor da multa
    writeNumber(stream, gare.valor, 14);                                  // 9(12)V9(02) valor do pagamento
    writeNumber(stream, gare.dataVencimento, 8);                          // 9(08) data de vencimento
    writeNumber(stream, gare.dataVencimento, 8);                          // 9(08) data de pagamento
    writeBlanks(stream, 11);                                              // X(11) complemento de registro
    writeText(stream, "STACCATO REVESTIMENTOS COM E REPRES LTDA", 30);    // X(30) nome do contribuinte
    writeText(stream, gare.cnpjOrig.left(8) + "   " + gare.numeroNF, 20); // X(20) seu numero: numero docto atribuido pela empresa
    writeBlanks(stream, 15);                                              // X(15) nosso numero: numero atribuido pelo banco ***apenas retorno, informar com branco ou zero
    writeBlanks(stream, 10);                                              // X(10) codigo de ocorrencias p/ retorno ***apenas retorno, informar com branco ou zero
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
    qApp->enqueueException("Erro ao criar a pasta CNAB Itau!", parent);
    return {};
  }

  QFile file(QDir::currentPath() + "/cnab/itau/cnab" + query.value("sequencial").toString() + ".rem");

  if (not file.open(QFile::WriteOnly)) {
    qApp->enqueueException(file.errorString(), parent);
    return {};
  }

  file.write(arquivo.toUtf8());
  file.close();

  QSqlQuery query2;

  if (not query2.exec("UPDATE cnab SET conteudo = '" + arquivo.toUtf8() + "' WHERE banco = 'ITAU' AND sequencial = " + query.value("sequencial").toString())) {
    qApp->enqueueException("Erro guardando CNAB: " + query2.lastError().text(), parent);
    return {};
  }

  if (not query2.exec("INSERT INTO cnab (tipo, banco, sequencial) VALUES ('REMESSA', 'ITAU', " + QString::number(query.value("sequencial").toInt() + 1) + ")")) {
    qApp->enqueueException("Erro guardando CNAB: " + query2.lastError().text(), parent);
    return {};
  }

  qApp->enqueueInformation("Arquivo gerado com sucesso: cnab" + query.value("sequencial").toString() + ".rem", parent);

  return query.value("idCnab").toString();
}

void CNAB::retornoGareItau240(const QString &filePath) {
  QFile file(filePath);

  if (not file.open(QFile::ReadOnly)) { return qApp->enqueueException("Erro lendo arquivo: " + file.errorString(), parent); }

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

      QString cnpj = line.mid(195, 8);
      QString nfe = line.mid(206, 9);

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

        if (not query1.exec("SELECT idNFe FROM nfe WHERE numeroNFe = " + nfe + " AND LEFT(cnpjOrig, 8) = " + cnpj) or not query1.first()) {
          qApp->enqueueException("Erro dando baixa na GARE: " + query1.lastError().text(), parent);
          return qApp->rollbackTransaction();
        }

        QSqlQuery query2;

        if (not query2.exec("UPDATE conta_a_pagar_has_pagamento SET status = 'PAGO GARE', valorReal = valor, dataRealizado = '" + dataPgt + "' WHERE idNFe = " + query1.value("idNFe").toString())) {
          qApp->enqueueException("Erro dando baixa na GARE: " + query2.lastError().text(), parent);
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
    qApp->enqueueException("Erro guardando CNAB: " + query2.lastError().text(), parent);
    qApp->rollbackTransaction();
  }

  if (not qApp->endTransaction()) { return; }

  qApp->enqueueInformation(resultado.join("\n"), parent);
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

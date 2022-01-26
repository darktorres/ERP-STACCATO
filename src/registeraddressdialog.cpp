#include "registeraddressdialog.h"

#include "application.h"

#include <QSqlQuery>

RegisterAddressDialog::RegisterAddressDialog(const QString &table, const QString &primaryKeyStr, QWidget *parent) : RegisterDialog(table, primaryKeyStr, parent) { setupTables(table); }

void RegisterAddressDialog::setupTables(const QString &table) {
  modelEnd.setTable(table + "_has_endereco");

  modelEnd.setHeaderData("descricao", "Descrição");
  modelEnd.setHeaderData("cep", "CEP");
  modelEnd.setHeaderData("logradouro", "Logradouro");
  modelEnd.setHeaderData("numero", "Número");
  modelEnd.setHeaderData("complemento", "Compl.");
  modelEnd.setHeaderData("bairro", "Bairro");
  modelEnd.setHeaderData("cidade", "Cidade");
  modelEnd.setHeaderData("uf", "UF");
  modelEnd.setHeaderData("desativado", "Desativado");

  //----------------------------------------------------------

  mapperEnd.setModel(&modelEnd);
  mapperEnd.setSubmitPolicy(QDataWidgetMapper::ManualSubmit);
}

QVariant RegisterAddressDialog::dataEnd(const QString &key) const { return modelEnd.data(currentRowEnd, key); }

void RegisterAddressDialog::setDataEnd(const QString &key, const QVariant &value) { modelEnd.setData(currentRowEnd, key, value); }

void RegisterAddressDialog::verificaEndereco(const QString &cidade, const QString &uf) {
  QSqlQuery query;
  query.prepare("SELECT codigo FROM cidade WHERE nome = :cidade AND uf = :uf");
  query.bindValue(":cidade", cidade);
  query.bindValue(":uf", uf);

  if (not query.exec()) { throw RuntimeException("Erro buscando código do munícipio!", this); }

  if (not query.first()) { throw RuntimeError("Não foi encontrado o código do munícipio, verifique se cidade/estado estão escritos corretamente!", this); }
}

bool RegisterAddressDialog::newRegister() {
  if (not RegisterDialog::newRegister()) { return false; }

  modelEnd.setFilter("0");

  currentRowEnd = -1;

  // TODO: replace with exception?
  return true;
}

int RegisterAddressDialog::getCodigoUF(const QString &uf) const {
  QString uf2 = uf.toLower();

  if (uf2 == "ro") { return 11; }
  if (uf2 == "ac") { return 12; }
  if (uf2 == "am") { return 13; }
  if (uf2 == "rr") { return 14; }
  if (uf2 == "pa") { return 15; }
  if (uf2 == "ap") { return 16; }
  if (uf2 == "to") { return 17; }
  if (uf2 == "ma") { return 21; }
  if (uf2 == "pi") { return 22; }
  if (uf2 == "ce") { return 23; }
  if (uf2 == "rn") { return 24; }
  if (uf2 == "pb") { return 25; }
  if (uf2 == "pe") { return 26; }
  if (uf2 == "al") { return 27; }
  if (uf2 == "se") { return 28; }
  if (uf2 == "ba") { return 29; }
  if (uf2 == "mg") { return 31; }
  if (uf2 == "es") { return 32; }
  if (uf2 == "rj") { return 33; }
  if (uf2 == "sp") { return 35; }
  if (uf2 == "pr") { return 41; }
  if (uf2 == "sc") { return 42; }
  if (uf2 == "rs") { return 43; }
  if (uf2 == "ms") { return 50; }
  if (uf2 == "mt") { return 51; }
  if (uf2 == "go") { return 52; }
  if (uf2 == "df") { return 53; }

  return 0;
}

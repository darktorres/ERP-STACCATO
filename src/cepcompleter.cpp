#include "cepcompleter.h"

#include "application.h"
#include "sqlquery.h"

#include <QSqlError>

void CepCompleter::buscaCEP(const QString &cep, QWidget *parent) {
  SqlQuery query;
  query.prepare("SELECT logradouro, complemento, bairro, cidade, uf, cep FROM cep.cep WHERE cep = :cep");
  query.bindValue(":cep", QString(cep).remove("-"));

  if (not query.exec()) { throw RuntimeException("Erro buscando CEP: " + query.lastError().text(), parent); }

  if (not query.first()) { throw RuntimeError("CEP não encontrado!", parent); }

  logradouro = query.value("logradouro").toString();
  complemento = query.value("complemento").toString();
  bairro = query.value("bairro").toString();
  cidade = query.value("cidade").toString();
  uf = query.value("uf").toString();
}

QString CepCompleter::getBairro() const { return bairro; }

QString CepCompleter::getCidade() const { return cidade; }

QString CepCompleter::getComplemento() const { return complemento; }

QString CepCompleter::getEndereco() const { return logradouro; }

QString CepCompleter::getUf() const { return uf; }

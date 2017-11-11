#ifndef CEPCOMPLETER_H
#define CEPCOMPLETER_H

#include <QString>

class CepCompleter final {

public:
  CepCompleter() = default;
  ~CepCompleter() = default;
  bool buscaCEP(const QString &cep);
  QString getBairro() const;
  QString getCidade() const;
  QString getEndereco() const;
  QString getUf() const;

private:
  QString cidade;
  QString endereco;
  QString bairro;
  QString uf;
};

#endif // CEPCOMPLETER_H

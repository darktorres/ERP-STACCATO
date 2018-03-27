#ifndef CEPCOMPLETER_H
#define CEPCOMPLETER_H

#include <QString>

class CepCompleter final {

public:
  CepCompleter() = default;
  ~CepCompleter() = default;
  auto buscaCEP(const QString &cep) -> bool;
  auto getBairro() const -> QString;
  auto getCidade() const -> QString;
  auto getEndereco() const -> QString;
  auto getUf() const -> QString;

private:
  QString cidade;
  QString endereco;
  QString bairro;
  QString uf;
};

#endif // CEPCOMPLETER_H

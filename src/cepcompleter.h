#pragma once

#include <QString>
#include <QWidget>

class CepCompleter final {

public:
  CepCompleter() = default;
  ~CepCompleter() = default;

  auto buscaCEP(const QString &cep, QWidget *parent) -> void;
  auto getBairro() const -> QString;
  auto getCidade() const -> QString;
  auto getComplemento() const -> QString;
  auto getEndereco() const -> QString;
  auto getUf() const -> QString;

private:
  // attributes
  QString bairro;
  QString cidade;
  QString complemento;
  QString logradouro;
  QString uf;
};

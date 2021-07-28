#pragma once

#include <QTextStream>

class CNAB {

public:
  struct Gare {
    int idNFe;
    int mesAnoReferencia;
    int dataVencimento;
    ulong valor;
    QString numeroNF;
    QString cnpjOrig;
  };

  struct Pagamento {
    enum class Tipo { Salario, Fornecedor } tipo;
    int codBanco;
    ulong valor;
    QString data;
    QString cpfDest;
    QString cnpjDest;
    ulong agencia;
    ulong conta;
    ulong dac;
    QString nome;
    QString codFornecedor;
  };

  explicit CNAB(QWidget *parent);
  ~CNAB() = default;

  auto remessaGareItau240(const QVector<Gare> &gares) -> QString;
  auto remessaPagamentoItau240(const QVector<Pagamento> &pagamentos) -> QString;
  auto retornoGareItau240(const QString &filePath) -> void;
  // TODO: adicionar funcoes para boleto e outros pagamentos

private:
  // attributes
  QWidget *parent;
  // methods
  auto decodeCodeItau(const QString &code) -> QString;
  auto writeBlanks(QTextStream &stream, const int count) -> void;
  auto writeNumber(QTextStream &stream, const ulong number, const int count) -> void;
  auto writeText(QTextStream &stream, const QString &text, const int count) -> void;
  auto writeZeros(QTextStream &stream, const int count) -> void;
};

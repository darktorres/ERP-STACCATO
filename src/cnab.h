#pragma once

#include <QDialog>
#include <QTextStream>

namespace Ui {
class CNAB;
}

class CNAB : public QDialog {
  Q_OBJECT

public:
  struct Gare {
    int idNFe;
    int mesAnoReferencia;
    int dataVencimento;
    int valor;
    QString numeroNF;
    QString cnpjOrig;
  };

  explicit CNAB(QWidget *parent = nullptr);
  auto remessaGareSantander240(QVector<Gare> gares) -> void;
  auto retornoGareSantander240() -> void;
  auto remessaGareItau240(QVector<Gare> gares) -> void;
  auto retornoGareItau240() -> void;
  // TODO: adicionar funcoes para boleto e outros pagamentos
  ~CNAB();

private:
  Ui::CNAB *ui;
  void writeBlanks(QTextStream &stream, const int count);
  void writeText(QTextStream &stream, const QString &text, const int count);
  void writeNumber(QTextStream &stream, const int number, const int count);
  void writeZeros(QTextStream &stream, const int count);
};

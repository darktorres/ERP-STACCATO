#pragma once

#include <QElapsedTimer>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVector>

#include <cmath>

// -----------------------------------------------------------------------------------------------
// Diagnóstico de "Erro nos valores!" (Orcamento/Venda).
//
// O dump anterior (montarLog) gravava um retrato mudo no instante do throw e nunca serviu para
// nada: não dizia QUAL comparação falhou nem por quanto, imprimia os números com 6 dígitos
// significativos (mais grosso que a tolerância de 0.1 que estava sendo testada, então dois valores
// diferentes apareciam idênticos) e não registrava nada sobre como o estado chegou ali. O que
// segue troca isso por veredito explícito + histórico das mutações.
//
// Estruturas compartilhadas porque orcamento.cpp e venda.cpp são gêmeos: OrcamentoTotais e
// VendaTotais são estruturalmente idênticos, apenas com nomes distintos para não colidir.
// -----------------------------------------------------------------------------------------------

// Retrato dos quatro totais canônicos do cabeçalho. Dado puro, sem Qt widgets.
struct TotaisSnapshot {
  double subTotalBruto = 0.;
  double subTotalLiq = 0.;
  double frete = 0.;
  double descontoReais = 0.;

  auto total() const -> double { return subTotalLiq - descontoReais + frete; }
};

// Uma comparação do invariante. Carrega SEMPRE o delta e a tolerância — nunca só um bool, que foi
// exatamente a informação que faltou em todos os dumps históricos.
struct TotaisCheck {
  QString nome;          // "bruto" | "liquido" | "itens" | "widgetTotal"
  double esperado = 0.;  // recomputado a partir de modelItem (ou lido do widget, em widgetTotal)
  double obtido = 0.;    // o que 'totais' afirma
  double tolerancia = 0.;

  auto delta() const -> double { return esperado - obtido; }
  auto falhou() const -> bool { return std::abs(delta()) > tolerancia; }
  // Quanto da tolerância a operação consumiu. Visível mesmo quando passa, para dar para ver o
  // quão perto do limite a operação normal roda.
  auto margemPct() const -> double { return tolerancia > 0. ? std::abs(delta()) / tolerancia * 100. : 0.; }
};

// Teste barato, sem construir nada. A checagem por render usa isto para sair cedo no caminho
// normal — montar flags/itens/mensagem só se justifica quando há o que registrar.
auto algumCheckFalhou(const QVector<TotaisCheck> &checks) -> bool;

// Ring buffer das mutações que produziram o estado atual. Uma instância por diálogo aberto.
// Só é formatado quando algum invariante quebra, então o custo em regime normal é o de guardar
// uma struct pequena por edição.
class TotaisTrace final {

public:
  TotaisTrace();

  auto registrar(const QString &origem, const QString &argumento, const TotaisSnapshot &resultado, const int itens, const double agregadoLiq) -> void;
  auto formatar() const -> QString;
  // Dedup por diálogo: true na primeira vez que esta assinatura aparece. Uma falha idêntica
  // repetida não gera linha nova; uma falha diferente gera.
  auto primeiraVez(const QString &assinatura) -> bool;
  auto limpar() -> void;

private:
  struct Entrada {
    int seq = 0;
    qint64 ms = 0;
    QString origem;
    QString argumento;
    TotaisSnapshot estado;
    int itens = 0;
    double agregadoLiq = 0.;
  };

  static constexpr int limiteEntradas = 60;

  QVector<Entrada> entradas;
  QSet<QString> assinaturasJaLogadas;
  QElapsedTimer relogio;
  int totalRegistrado = 0;
};

// Tudo que descreve uma quebra do invariante, pronto para virar uma linha de log.
struct TotaisDiagnostico {
  QString tela;       // "Orcamento" | "Venda"
  QString contexto;   // "save" | "render:load" | "render:edit"
  QString idRegistro; // idOrcamento / idVenda
  QVector<TotaisCheck> checks;
  TotaisSnapshot estado;
  QString flags;      // freteManual, tipo, readOnly, ... — o que muda o comportamento do cálculo
  QString itensSujos; // só as linhas ainda não persistidas; o resto já está no banco

  auto falhas() const -> QStringList;
  auto algumaFalhou() const -> bool;
  // contexto + checks que falharam + deltas a 2 casas. Chave de dedup do TotaisTrace.
  auto assinatura() const -> QString;
  auto formatar(const TotaisTrace &trace) const -> QString;
};

class Log final {

public:
  Log() = delete;

  static auto createLog(const QString &tipo, const QString &message) -> void;
  // Grava com tipo = "Totais" (valor próprio, separado das 302k linhas "Exceção" genéricas).
  static auto createLogTotais(const TotaisDiagnostico &diagnostico, const TotaisTrace &trace) -> void;
  // Identifica a execução do app. Distingue "o mesmo diálogo tentou de novo" de "dois problemas
  // diferentes" — a confusão exata dos dumps duplicados que apareciam no banco.
  static auto idSessao() -> QString;
  // Formatação monetária única: 4 casas, a precisão real dos dados (DECIMAL(15,4)).
  static auto dinheiro(const double valor) -> QString;
};

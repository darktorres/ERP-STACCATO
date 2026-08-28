#include "log.h"

#include "application.h"
#include "user.h"

#include <QDebug>
#include <QUuid>

void Log::createLog(const QString &tipo, const QString &message) {
  if (not QSqlDatabase::database().isOpen()) { return; }

  SqlQuery query;
  query.prepare("INSERT INTO log (idUsuario, versao, tipo, message) VALUES (:idUsuario, :versao, :tipo, :message)");
  query.bindValue(":idUsuario", User::idUsuario);
  query.bindValue(":versao", qApp->applicationVersion());
  query.bindValue(":tipo", tipo);
  query.bindValue(":message", message);

  query.exec();

  qDebug() << "log: (" + tipo + ") -> " + message;
}

QString Log::dinheiro(const double valor) { return QString::number(valor, 'f', 4); }

QString Log::idSessao() {
  // Uma por execução do app, criada na primeira chamada.
  static const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
  return id;
}

// -----------------------------------------------------------------------------------------------
// TotaisTrace
// -----------------------------------------------------------------------------------------------

TotaisTrace::TotaisTrace() { relogio.start(); }

void TotaisTrace::registrar(const QString &origem, const QString &argumento, const TotaisSnapshot &resultado, const int itens, const double agregadoLiq) {
  if (entradas.size() >= limiteEntradas) { entradas.removeFirst(); }

  // Entrada explícita em vez de append({...}): com as sobrecargas append(const T &)/append(T &&) do
  // QVector, uma braced-init-list fica ambígua.
  const Entrada entrada{++totalRegistrado, relogio.elapsed(), origem, argumento, resultado, itens, agregadoLiq};

  entradas.append(entrada);
}

void TotaisTrace::limpar() {
  entradas.clear();
  assinaturasJaLogadas.clear();
  totalRegistrado = 0;
  relogio.restart();
}

bool TotaisTrace::primeiraVez(const QString &assinatura) {
  if (assinaturasJaLogadas.contains(assinatura)) { return false; }

  assinaturasJaLogadas.insert(assinatura);

  return true;
}

QString TotaisTrace::formatar() const {
  if (entradas.isEmpty()) { return "trace: vazio (nenhuma mutação registrada neste diálogo)"; }

  QStringList linhas;

  linhas << "trace (últimas " + QString::number(entradas.size()) + " de " + QString::number(totalRegistrado) + " mutações, ms desde a abertura do diálogo):";

  for (const auto &entrada : entradas) {
    linhas << "  #" + QString::number(entrada.seq).rightJustified(3) +                             //
                  " +" + QString::number(entrada.ms).rightJustified(7) +                           //
                  "  " + entrada.origem.leftJustified(28) +                                        //
                  " arg=" + (entrada.argumento.isEmpty() ? QString("-") : entrada.argumento).leftJustified(12) + //
                  " bruto=" + Log::dinheiro(entrada.estado.subTotalBruto).rightJustified(13) +     //
                  " liq=" + Log::dinheiro(entrada.estado.subTotalLiq).rightJustified(13) +         //
                  " frete=" + Log::dinheiro(entrada.estado.frete).rightJustified(11) +             //
                  " desc=" + Log::dinheiro(entrada.estado.descontoReais).rightJustified(11) +      //
                  " itensLiq=" + Log::dinheiro(entrada.agregadoLiq).rightJustified(13) +           //
                  " n=" + QString::number(entrada.itens);
  }

  return linhas.join("\n");
}

// -----------------------------------------------------------------------------------------------
// TotaisDiagnostico
// -----------------------------------------------------------------------------------------------

bool algumCheckFalhou(const QVector<TotaisCheck> &checks) {
  for (const auto &check : checks) {
    if (check.falhou()) { return true; }
  }

  return false;
}

QStringList TotaisDiagnostico::falhas() const {
  QStringList nomes;

  for (const auto &check : checks) {
    if (check.falhou()) { nomes << check.nome; }
  }

  return nomes;
}

bool TotaisDiagnostico::algumaFalhou() const { return algumCheckFalhou(checks); }

QString TotaisDiagnostico::assinatura() const {
  QStringList partes;

  partes << contexto;

  for (const auto &check : checks) {
    if (check.falhou()) { partes << check.nome + "=" + QString::number(check.delta(), 'f', 2); }
  }

  return partes.join("|");
}

QString TotaisDiagnostico::formatar(const TotaisTrace &trace) const {
  const QStringList nomesFalhos = falhas();

  QStringList linhas;

  // Cabeçalho de uma linha só, parseável — é por ele que a triagem agrupa (db/triagem_log_totais.sql).
  linhas << "[TOTAIS] tela=" + tela + " id=" + idRegistro + " ctx=" + contexto + " sess=" + Log::idSessao() + " build=" + qApp->applicationVersion() +
                " falhou=" + (nomesFalhos.isEmpty() ? QString("-") : nomesFalhos.join(","));

  // Toda comparação sai com delta E tolerância. As que passam mostram quanto da tolerância
  // consumiram, para dar para ver o quão perto do limite a operação normal está rodando.
  for (const auto &check : checks) {
    linhas << "  " + check.nome.leftJustified(12) +                            //
                  " esperado=" + Log::dinheiro(check.esperado).rightJustified(14) + //
                  " obtido=" + Log::dinheiro(check.obtido).rightJustified(14) + //
                  " delta=" + Log::dinheiro(check.delta()).rightJustified(14) + //
                  " tol=" + Log::dinheiro(check.tolerancia).rightJustified(10) + //
                  (check.falhou() ? QString("  FALHOU") : "  OK (" + QString::number(check.margemPct(), 'f', 1) + "% da tol)");
  }

  linhas << "estado: bruto=" + Log::dinheiro(estado.subTotalBruto) + " liq=" + Log::dinheiro(estado.subTotalLiq) + " frete=" + Log::dinheiro(estado.frete) +
                " desc=" + Log::dinheiro(estado.descontoReais) + " total()=" + Log::dinheiro(estado.total());

  if (not flags.isEmpty()) { linhas << "flags: " + flags; }

  linhas << "";
  linhas << trace.formatar();

  if (not itensSujos.isEmpty()) {
    linhas << "";
    linhas << itensSujos;
  }

  return linhas.join("\n");
}

void Log::createLogTotais(const TotaisDiagnostico &diagnostico, const TotaisTrace &trace) {
  QString message = diagnostico.formatar(trace);

  // Dentro de uma transação um ROLLBACK posterior levaria este INSERT junto. Marca a linha para
  // que a ausência de pares não seja lida como "não aconteceu".
  if (qApp->getInTransaction()) { message.prepend("[gravado dentro de transação — pode ser perdido em rollback]\n"); }

  // log.message é TEXT (65535 bytes). Estourar faz o INSERT falhar inteiro — e createLog() ignora o
  // retorno de exec(), então a linha sumiria em silêncio, que é justamente o que se quer evitar aqui.
  // O cabeçalho e o veredito vêm primeiro, então cortar o fim preserva o que importa.
  constexpr int limiteBytes = 60000;

  if (message.toUtf8().size() > limiteBytes) {
    message = QString::fromUtf8(message.toUtf8().left(limiteBytes)) + "\n[truncado: mensagem excedeu " + QString::number(limiteBytes) + " bytes]";
  }

  createLog("Totais", message);
}

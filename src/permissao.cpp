
#include "application.h"
#include "permissao.h"
#include "tableview.h"
#include "user.h"

#include <QAbstractButton>
#include <QAction>
#include <QEvent>
#include <QMenu>
#include <QSqlError>
#include <QTabWidget>
#include <QWidget>

namespace {

// Desfaz qualquer setEnabled(true) que a regra de negocio aplique depois da inicializacao.
// Obrigatorio: widgetlogisticaentregas.cpp:270-293 reabilita os botoes a cada selectionChanged, e um
// setEnabled(false) so na inicializacao seria desfeito no primeiro clique na tabela.
class GuardaEnabled final : public QObject {

public:
  explicit GuardaEnabled(QObject *parent) : QObject(parent) {}

private:
  auto eventFilter(QObject *watched, QEvent *event) -> bool final {
    if (event->type() == QEvent::EnabledChange) {
      // Sem recursao infinita: o setEnabled(false) reentra aqui com isEnabled() ja false.
      if (auto *widget = qobject_cast<QWidget *>(watched); widget and widget->isEnabled()) { widget->setEnabled(false); }
    }

    return QObject::eventFilter(watched, event);
  }
};

auto guarda() -> GuardaEnabled * {
  static auto *instancia = new GuardaEnabled(qApp);
  return instancia;
}

auto travarControle(QWidget *widget) -> void {
  widget->setEnabled(false);
  widget->installEventFilter(guarda());
}

auto percorrer(QObject *obj, const QString &prefixo) -> void {
  for (QObject *filho : obj->children()) {
    // Subarvore de outra tela (ex.: WidgetPagamentos dentro da Venda) - ela cuida dos proprios
    // controles, com o proprio prefixo. findChildren() nao permitiria essa poda.
    if (filho->property("prefixoPermissao").isValid()) { continue; }

    // QAbstractButton, nao QPushButton: cobre QCheckBox/QRadioButton/QToolButton de graca. E o que
    // alcanca widgetgalpao/checkBoxEdicao, que libera a edicao do galpao.
    if (auto *botao = qobject_cast<QAbstractButton *>(filho)) {
      if (not Permissao::tem(prefixo + "." + Permissao::sufixoDaChave(botao->objectName()))) { travarControle(botao); }
    }

    if (auto *tabela = qobject_cast<TableView *>(filho)) {
      if (not Permissao::tem(prefixo + ".editarTabela")) { tabela->setSomenteLeitura(true); }
    }

    if (auto *abas = qobject_cast<QTabWidget *>(filho)) {
      for (int i = 0; i < abas->count(); ++i) {
        const QString sufixo = Permissao::sufixoDaChave(abas->widget(i)->objectName());

        if (not Permissao::tem(prefixo + "." + sufixo + ".ver")) { abas->setTabEnabled(i, false); }
      }
    }

    percorrer(filho, prefixo);
  }
}

} // namespace

QString Permissao::sufixoDaChave(const QString &objectName) {
  QString nome = objectName;

  for (const QString &prefixo : {"pushButton", "toolButton", "checkBox", "radioButton", "action", "menu", "tab"}) {
    if (nome.startsWith(prefixo) and nome.size() > prefixo.size()) {
      nome = nome.mid(prefixo.size());
      break;
    }
  }

  // 'Gerenciar_dados_bancarios' -> 'GerenciarDadosBancarios'
  while (true) {
    const int pos = nome.indexOf('_');

    if (pos == -1 or pos + 1 >= nome.size()) { break; }

    nome = nome.left(pos) + nome.at(pos + 1).toUpper() + nome.mid(pos + 2);
  }

  nome.remove('_');

  if (not nome.isEmpty()) { nome[0] = nome.at(0).toLower(); }

  return nome;
}

void Permissao::carregar() {
  cache.clear();
  ativo = true;
  perfilExiste = false;

  SqlQuery queryConfig;

  // Interruptor de emergencia: 'UPDATE config SET permissoesAtivas = 0' libera tudo no proximo login,
  // sem rebuild. Se a coluna ainda nao existe (build novo em banco velho), o catch deixa 'ativo'
  // true e o cache vazio - e ai a checagem de perfilExiste abaixo e que decide.
  if (queryConfig.exec("SELECT permissoesAtivas FROM config WHERE idConfig = 1") and queryConfig.first()) { ativo = queryConfig.value("permissoesAtivas").toBool(); }

  if (not ativo) { return; }

  SqlQuery queryPerfil;
  queryPerfil.prepare("SELECT COUNT(*) AS total FROM perfil_has_permissao WHERE tipo = :tipo");
  queryPerfil.bindValue(":tipo", User::tipo);

  if (not queryPerfil.exec()) { throw RuntimeException("Erro lendo perfil de permissoes: " + queryPerfil.lastError().text()); }

  if (not queryPerfil.first()) { throw RuntimeException("Erro lendo perfil de permissoes!"); }

  perfilExiste = queryPerfil.value("total").toInt() > 0;

  if (not perfilExiste) { return; } // fail-closed: tem() vai negar tudo

  SqlQuery query;
  query.prepare("SELECT p.chave, COALESCE(o.permitido, f.permitido, 0) AS permitido "
                "FROM permissao p "
                "LEFT JOIN perfil_has_permissao f ON f.idPermissao = p.idPermissao AND f.tipo = :tipo "
                "LEFT JOIN usuario_has_permissao_override o ON o.idPermissao = p.idPermissao AND o.idUsuario = :idUsuario");
  query.bindValue(":tipo", User::tipo);
  query.bindValue(":idUsuario", User::idUsuario);

  if (not query.exec()) { throw RuntimeException("Erro lendo permissoes: " + query.lastError().text()); }

  while (query.next()) { cache.insert(query.value("chave").toString(), query.value("permitido").toBool()); }
}

bool Permissao::tem(const QString &chave) {
  if (not ativo) { return true; }        // interruptor global
  if (not perfilExiste) { return false; } // tipo sem perfil configurado

  // Chave ausente do catalogo = permitido. O catalogo e gerado de ui/*.ui, entao ausencia e descuido,
  // nunca decisao - e travar um botao novo em producao e pior que libera-lo.
  return cache.value(chave, true);
}

void Permissao::exigir(const QString &chave) {
  if (tem(chave)) { return; }

  throw RuntimeError("Usuario nao possui permissao para esta operacao!");
}

void Permissao::aplicarTela(QWidget *tela, const QString &prefixo) {
  if (not tela) { return; }

  // Marca a raiz antes de descer: e o que faz a poda funcionar e o que permite a mesma classe receber
  // prefixos diferentes por instancia (WidgetFinanceiroContas e Pagar numa e Receber na outra).
  tela->setProperty("prefixoPermissao", prefixo);

  percorrer(tela, prefixo);
}

void Permissao::aplicarMenu(QWidget *janela, const QString &prefixo) {
  if (not janela) { return; }

  for (QAction *acao : janela->findChildren<QAction *>()) {
    if (acao->objectName().isEmpty()) { continue; }

    if (not tem(prefixo + "." + sufixoDaChave(acao->objectName()))) { acao->setVisible(false); }
  }

  // O QMenu 'Importar tabela fornecedor' e gateado pela propria menuAction() (mainwindow.cpp:70).
  for (QMenu *submenu : janela->findChildren<QMenu *>()) {
    if (submenu->objectName().isEmpty()) { continue; }

    if (not tem(prefixo + "." + sufixoDaChave(submenu->objectName()))) { submenu->menuAction()->setVisible(false); }
  }
}

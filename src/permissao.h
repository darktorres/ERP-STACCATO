#pragma once

#include <QHash>
#include <QString>

class QWidget;

// Sistema de permissoes granular: uma chave por botao/aba/tabela/acao de menu.
//
// Chave = "<modulo>.<tela>.<acao>", derivada do objectName do widget:
//   pushButtonCancelarEntrega -> logistica.entregas.cancelarEntrega
//   tabAgendarEntrega         -> logistica.agendarEntrega.ver
//   actionCadastrarUsuario    -> menu.cadastrarUsuario
//
// Efetivo = COALESCE(override do usuario, perfil do usuario.tipo, 0), com tres regras de borda:
//   config.permissoesAtivas = 0      -> permite tudo (saida de emergencia sem rebuild)
//   chave ausente do catalogo        -> permite (botao novo nao trava a tela)
//   tipo sem perfil configurado      -> nega tudo (fail-closed)
class Permissao final {

public:
  Permissao() = delete;

  // Carrega o cache do usuario logado numa unica query. Chamada em Application::dbConnect, depois de
  // userLogin() - nao em dbReconnect(), que nao refaz o login. Serve tambem de refresh apos salvar
  // na tela de gerenciamento.
  static auto carregar() -> void;

  static auto tem(const QString &chave) -> bool;

  // Checagem autoritativa para caminhos que nao passam pelo estado 'enabled' do botao (Ctrl+S).
  static auto exigir(const QString &chave) -> void;

  // Percorre a arvore de 'tela' e trava os controles negados. Marca 'tela' com a propriedade
  // 'prefixoPermissao', que serve de raiz e de poda: subarvore que ja tem a propriedade pertence a
  // outra tela e nao e visitada. Onde ha aninhamento, chamar nos filhos ANTES do pai.
  static auto aplicarTela(QWidget *tela, const QString &prefixo) -> void;

  // Equivalente para QAction/QMenu. One-shot: nada no projeto reabilita QAction.
  static auto aplicarMenu(QWidget *janela, const QString &prefixo) -> void;

  // Normaliza objectName -> sufixo da chave. Precisa bater exatamente com a mesma funcao em
  // db/gerar_catalogo_permissoes.py, senao o catalogo nao casa com o runtime.
  static auto sufixoDaChave(const QString &objectName) -> QString;

private:
  inline static QHash<QString, bool> cache;
  inline static bool ativo = true;         // config.permissoesAtivas
  inline static bool perfilExiste = false; // false => nega tudo
};

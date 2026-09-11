#pragma once

#include "sqlquerymodel.h"

#include <QDialog>
#include <QHash>
#include <QPair>

class QTreeWidget;
class QTreeWidgetItem;

namespace Ui {
class GerenciarPermissoes;
}

// Tela de administração do catálogo de permissões, em três abas:
//   Perfis     - matriz árvore x tipos, para configurar o padrão de cada tipo de usuário
//   Usuários   - exceções por usuário, mostrando perfil / exceção / efetivo lado a lado
//   Quem pode? - busca reversa: dada uma permissão, quem consegue executá-la hoje
class GerenciarPermissoes final : public QDialog {
  Q_OBJECT

public:
  explicit GerenciarPermissoes(QWidget *parent);
  ~GerenciarPermissoes();

  // Abre já na aba de exceções, com o usuário selecionado (usado pelo Cadastro de Usuário).
  auto mostrarUsuario(const QString &idUsuario) -> void;

private:
  // attributes
  bool carregando = false;
  QStringList tipos;
  // (tipo, idPermissao) -> permitido. Edição fica em memória até o Salvar, para o admin poder
  // revisar e descartar antes de gravar 4270 linhas.
  QHash<QPair<QString, int>, bool> pendentesPerfil;
  // idPermissao -> permitido; -1 = "Herda" (remove o override).
  QHash<int, int> pendentesUsuario;
  SqlQueryModel modelQuemPode;
  Ui::GerenciarPermissoes *ui;
  // methods
  auto aplicarFiltroPerfis() -> void;
  auto atualizarContadorPerfis() -> void;
  auto atualizarContadorUsuario() -> void;
  auto carregarPerfis() -> void;
  auto carregarTipos() -> void;
  auto carregarUsuario() -> void;
  auto carregarUsuarios() -> void;
  auto carregarPermissoesCombo() -> void;
  auto closeEvent(QCloseEvent *event) -> void final;
  auto criarArvore(QTreeWidget *arvore, const bool comCheck) -> QHash<int, QTreeWidgetItem *>;
  auto efetivoUsuario(const int idPermissao) const -> bool;
  auto idUsuarioSelecionado() const -> QString;
  auto marcarSubArvore(QTreeWidgetItem *item, const int coluna, const Qt::CheckState estado) -> void;
  auto on_pushButtonCopiarPerfil_clicked() -> void;
  auto on_pushButtonDescartarUsuario_clicked() -> void;
  auto on_pushButtonDescartar_clicked() -> void;
  auto on_pushButtonSalvarUsuario_clicked() -> void;
  auto on_pushButtonSalvar_clicked() -> void;
  auto on_treePerfis_itemChanged(QTreeWidgetItem *item, const int coluna) -> void;
  auto on_treeUsuario_itemChanged(QTreeWidgetItem *item, const int coluna) -> void;
  auto perfilAtual(const QString &tipo, const int idPermissao) const -> bool;
  auto propagarEstadoPais(QTreeWidgetItem *item, const int coluna) -> void;
  auto setConnections() -> void;
  auto unsetConnections() -> void;
  auto temPendencias() const -> bool;
};

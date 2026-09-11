#include "gerenciarpermissoes.h"
#include "ui_gerenciarpermissoes.h"

#include "application.h"
#include "comboboxdelegate.h"
#include "permissao.h"
#include "user.h"

#include <QCloseEvent>
#include <QInputDialog>
#include <QMessageBox>
#include <QSqlError>
#include <QTreeWidget>

#include <functional>

namespace {

struct Item {
  int idPermissao = 0;
  QString chave;
  QString modulo;
  QString tela;
  QString descricao;
  QString tipoAcao;
  bool escrita = true;
};

QVector<Item> catalogo;
QHash<QPair<QString, int>, bool> perfilBase;  // (tipo, idPermissao)
QHash<int, bool> overrideBase;                // idPermissao -> permitido (do usuário selecionado)

auto rotulo(const Item &item) -> QString { return item.escrita ? item.descricao + "  ✎" : item.descricao; }

} // namespace

GerenciarPermissoes::GerenciarPermissoes(QWidget *parent) : QDialog(parent), ui(new Ui::GerenciarPermissoes) {
  ui->setupUi(this);

  // Guarda autoritativo: os dois pontos de entrada (mainwindow.cpp, cadastrousuario.cpp) so escondem
  // o botao/acao, que e higiene de UI, nao seguranca - ver permissao.h.
  Permissao::exigir("menu.gerenciarPermissoes");

  setWindowFlags(Qt::Window);

  carregarTipos();
  carregarPerfis();
  carregarUsuarios();
  carregarPermissoesCombo();

  ui->treeUsuario->setItemDelegateForColumn(2, new ComboBoxDelegate(ComboBoxDelegate::Tipo::HerdaSimNao, this));

  setConnections();
}

GerenciarPermissoes::~GerenciarPermissoes() { delete ui; }

void GerenciarPermissoes::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->lineEditBuscar, &LineEdit::delayedTextChanged, this, &GerenciarPermissoes::aplicarFiltroPerfis, connectionType);
  connect(ui->radioButtonTodas, &QRadioButton::toggled, this, &GerenciarPermissoes::aplicarFiltroPerfis, connectionType);
  connect(ui->radioButtonEscrita, &QRadioButton::toggled, this, &GerenciarPermissoes::aplicarFiltroPerfis, connectionType);
  connect(ui->radioButtonNegadas, &QRadioButton::toggled, this, &GerenciarPermissoes::aplicarFiltroPerfis, connectionType);
  connect(ui->radioButtonAlteradas, &QRadioButton::toggled, this, &GerenciarPermissoes::aplicarFiltroPerfis, connectionType);
  connect(ui->pushButtonExpandir, &QPushButton::clicked, ui->treePerfis, &QTreeWidget::expandAll, connectionType);
  connect(ui->pushButtonRecolher, &QPushButton::clicked, ui->treePerfis, &QTreeWidget::collapseAll, connectionType);
  connect(ui->pushButtonCopiarPerfil, &QPushButton::clicked, this, &GerenciarPermissoes::on_pushButtonCopiarPerfil_clicked, connectionType);
  connect(ui->pushButtonSalvar, &QPushButton::clicked, this, &GerenciarPermissoes::on_pushButtonSalvar_clicked, connectionType);
  connect(ui->pushButtonDescartar, &QPushButton::clicked, this, &GerenciarPermissoes::on_pushButtonDescartar_clicked, connectionType);
  connect(ui->treePerfis, &QTreeWidget::itemChanged, this, &GerenciarPermissoes::on_treePerfis_itemChanged, connectionType);

  connect(ui->comboBoxUsuario, qOverload<int>(&QComboBox::currentIndexChanged), this, &GerenciarPermissoes::carregarUsuario, connectionType);
  connect(ui->checkBoxSoExcecoes, &QCheckBox::toggled, this, &GerenciarPermissoes::carregarUsuario, connectionType);
  connect(ui->treeUsuario, &QTreeWidget::itemChanged, this, &GerenciarPermissoes::on_treeUsuario_itemChanged, connectionType);
  connect(ui->pushButtonSalvarUsuario, &QPushButton::clicked, this, &GerenciarPermissoes::on_pushButtonSalvarUsuario_clicked, connectionType);
  connect(ui->pushButtonDescartarUsuario, &QPushButton::clicked, this, &GerenciarPermissoes::on_pushButtonDescartarUsuario_clicked, connectionType);

  connect(ui->comboBoxPermissao, qOverload<int>(&QComboBox::currentIndexChanged), this, &GerenciarPermissoes::carregarPermissoesCombo, connectionType);
}

void GerenciarPermissoes::unsetConnections() {
  disconnect(ui->treePerfis, &QTreeWidget::itemChanged, this, &GerenciarPermissoes::on_treePerfis_itemChanged);
  disconnect(ui->treeUsuario, &QTreeWidget::itemChanged, this, &GerenciarPermissoes::on_treeUsuario_itemChanged);
}

// ---------------------------------------------------------------------------------------------
// Carga
// ---------------------------------------------------------------------------------------------

void GerenciarPermissoes::carregarTipos() {
  // União do que existe no banco com o que o combo do Cadastro de Usuário permite criar. Só o banco
  // não basta: ASSISTENTE ADMINISTRATIVO e GERENTE FINANCEIRO têm 0 usuários hoje, mas são criáveis,
  // e um tipo sem perfil fica travado (fail-closed).
  // QStringList explícito: com lista de inicialização nua o MSVC não decide entre os operator= de
  // QStringList e os de QList<QString>.
  tipos = QStringList{"ADMINISTRADOR", "ADMINISTRATIVO", "ASSISTENTE ADMINISTRATIVO", "DIRETOR", "GERENTE DEPARTAMENTO",
                      "GERENTE FINANCEIRO", "GERENTE LOJA", "OPERACIONAL", "VENDEDOR", "VENDEDOR ESPECIAL"};

  SqlQuery query;

  if (query.exec("SELECT DISTINCT tipo FROM usuario WHERE tipo IS NOT NULL AND tipo <> '' ORDER BY tipo")) {
    while (query.next()) {
      const QString tipo = query.value("tipo").toString();

      if (not tipos.contains(tipo)) { tipos << tipo; }
    }
  }
}

void GerenciarPermissoes::carregarPerfis() {
  catalogo.clear();
  perfilBase.clear();
  pendentesPerfil.clear();

  SqlQuery query;

  if (not query.exec("SELECT idPermissao, chave, modulo, tela, descricao, tipoAcao, escrita FROM permissao ORDER BY modulo, tela, ordem, chave")) {
    throw RuntimeException("Erro lendo catálogo de permissões: " + query.lastError().text(), this);
  }

  while (query.next()) {
    Item item;
    item.idPermissao = query.value("idPermissao").toInt();
    item.chave = query.value("chave").toString();
    item.modulo = query.value("modulo").toString();
    item.tela = query.value("tela").toString();
    item.descricao = query.value("descricao").toString();
    item.tipoAcao = query.value("tipoAcao").toString();
    item.escrita = query.value("escrita").toBool();
    catalogo << item;
  }

  SqlQuery queryPerfil;

  if (not queryPerfil.exec("SELECT tipo, idPermissao, permitido FROM perfil_has_permissao")) {
    throw RuntimeException("Erro lendo perfis: " + queryPerfil.lastError().text(), this);
  }

  while (queryPerfil.next()) {
    perfilBase.insert({queryPerfil.value("tipo").toString(), queryPerfil.value("idPermissao").toInt()}, queryPerfil.value("permitido").toBool());
  }

  unsetConnections();

  ui->treePerfis->clear();
  ui->treePerfis->setColumnCount(1 + tipos.size());

  QStringList cabecalho;
  cabecalho << "Permissão";
  for (const QString &tipo : tipos) { cabecalho << tipo; }
  ui->treePerfis->setHeaderLabels(cabecalho);

  const auto folhas = criarArvore(ui->treePerfis, true);

  for (const Item &item : catalogo) {
    QTreeWidgetItem *folha = folhas.value(item.idPermissao);

    if (not folha) { continue; }

    for (int i = 0; i < tipos.size(); ++i) {
      folha->setCheckState(i + 1, perfilAtual(tipos.at(i), item.idPermissao) ? Qt::Checked : Qt::Unchecked);
    }
  }

  for (int i = 0; i < ui->treePerfis->topLevelItemCount(); ++i) {
    for (int coluna = 1; coluna <= tipos.size(); ++coluna) { propagarEstadoPais(ui->treePerfis->topLevelItem(i), coluna); }
  }

  // Perfil sem nenhuma permissão significa "esse tipo não abre nada" - com fail-closed isso trava
  // todo mundo daquele tipo, então tem que saltar aos olhos.
  for (int i = 0; i < tipos.size(); ++i) {
    int concedidas = 0;

    for (const Item &item : catalogo) {
      if (perfilAtual(tipos.at(i), item.idPermissao)) { ++concedidas; }
    }

    if (concedidas == 0) {
      ui->treePerfis->headerItem()->setForeground(i + 1, QBrush(QColor(200, 0, 0)));
      ui->treePerfis->headerItem()->setText(i + 1, tipos.at(i) + " (vazio!)");
    }
  }

  ui->treePerfis->expandToDepth(0);
  for (int coluna = 0; coluna < ui->treePerfis->columnCount(); ++coluna) { ui->treePerfis->resizeColumnToContents(coluna); }

  setConnections();
  atualizarContadorPerfis();
}

QHash<int, QTreeWidgetItem *> GerenciarPermissoes::criarArvore(QTreeWidget *arvore, const bool comCheck) {
  QHash<int, QTreeWidgetItem *> folhas;
  QHash<QString, QTreeWidgetItem *> modulos;
  QHash<QString, QTreeWidgetItem *> telas;

  for (const Item &item : catalogo) {
    QTreeWidgetItem *noModulo = modulos.value(item.modulo);

    if (not noModulo) {
      noModulo = new QTreeWidgetItem(arvore, {item.modulo});
      modulos.insert(item.modulo, noModulo);
    }

    const QString chaveTela = item.modulo + "" + item.tela;
    QTreeWidgetItem *noTela = telas.value(chaveTela);

    if (not noTela) {
      noTela = new QTreeWidgetItem(noModulo, {item.tela});
      telas.insert(chaveTela, noTela);
    }

    auto *folha = new QTreeWidgetItem(noTela, {rotulo(item)});
    folha->setData(0, Qt::UserRole, item.idPermissao);
    folha->setToolTip(0, item.chave);

    if (comCheck) {
      folha->setFlags(folha->flags() | Qt::ItemIsUserCheckable);
      // Nós pai também recebem checkbox, mas o estado é calculado por propagarEstadoPais - sem
      // ItemIsAutoTristate, que recalcularia por conta própria e brigaria com a propagação.
      noModulo->setFlags(noModulo->flags() | Qt::ItemIsUserCheckable);
      noTela->setFlags(noTela->flags() | Qt::ItemIsUserCheckable);
    }

    folhas.insert(item.idPermissao, folha);
  }

  return folhas;
}

// ---------------------------------------------------------------------------------------------
// Aba Perfis
// ---------------------------------------------------------------------------------------------

bool GerenciarPermissoes::perfilAtual(const QString &tipo, const int idPermissao) const {
  const QPair<QString, int> chave{tipo, idPermissao};

  if (pendentesPerfil.contains(chave)) { return pendentesPerfil.value(chave); }

  return perfilBase.value(chave, false);
}

void GerenciarPermissoes::marcarSubArvore(QTreeWidgetItem *item, const int coluna, const Qt::CheckState estado) {
  if (item->childCount() == 0) {
    const int idPermissao = item->data(0, Qt::UserRole).toInt();
    const QString tipo = tipos.at(coluna - 1);
    const bool permitido = (estado == Qt::Checked);

    if (perfilBase.value({tipo, idPermissao}, false) == permitido) {
      pendentesPerfil.remove({tipo, idPermissao});
    } else {
      pendentesPerfil.insert({tipo, idPermissao}, permitido);
    }

    item->setCheckState(coluna, estado);
    return;
  }

  for (int i = 0; i < item->childCount(); ++i) { marcarSubArvore(item->child(i), coluna, estado); }
}

void GerenciarPermissoes::propagarEstadoPais(QTreeWidgetItem *item, const int coluna) {
  if (item->childCount() == 0) { return; }

  bool algumMarcado = false;
  bool algumDesmarcado = false;

  for (int i = 0; i < item->childCount(); ++i) {
    QTreeWidgetItem *filho = item->child(i);
    propagarEstadoPais(filho, coluna);

    // Filho parcial já basta para o pai ser parcial - por isso não dá para contar só marcados.
    switch (filho->checkState(coluna)) {
    case Qt::PartiallyChecked: algumMarcado = algumDesmarcado = true; break;
    case Qt::Checked: algumMarcado = true; break;
    default: algumDesmarcado = true; break;
    }
  }

  item->setCheckState(coluna, (algumMarcado and algumDesmarcado) ? Qt::PartiallyChecked : (algumMarcado ? Qt::Checked : Qt::Unchecked));
}

void GerenciarPermissoes::on_treePerfis_itemChanged(QTreeWidgetItem *item, const int coluna) {
  if (carregando or coluna == 0) { return; }

  unsetConnections();

  // Clicar num nó Módulo/Tela aplica a todos os filhos daquela coluna - é assim que se libera um
  // módulo inteiro sem 40 cliques.
  marcarSubArvore(item, coluna, item->checkState(coluna) == Qt::Checked ? Qt::Checked : Qt::Unchecked);

  for (int i = 0; i < ui->treePerfis->topLevelItemCount(); ++i) { propagarEstadoPais(ui->treePerfis->topLevelItem(i), coluna); }

  setConnections();
  atualizarContadorPerfis();

  if (ui->radioButtonAlteradas->isChecked()) { aplicarFiltroPerfis(); }
}

void GerenciarPermissoes::atualizarContadorPerfis() {
  ui->labelAlteracoes->setText(pendentesPerfil.isEmpty() ? "" : QString("● %1 alteração(ões) não salva(s)").arg(pendentesPerfil.size()));
  ui->labelAlteracoes->setStyleSheet(pendentesPerfil.isEmpty() ? "" : "color: rgb(200, 120, 0); font-weight: bold;");
}

void GerenciarPermissoes::aplicarFiltroPerfis() {
  const QString busca = ui->lineEditBuscar->text().trimmed();

  QHash<int, const Item *> porId;
  for (const Item &item : catalogo) { porId.insert(item.idPermissao, &item); }

  std::function<bool(QTreeWidgetItem *)> filtrar = [&](QTreeWidgetItem *item) -> bool {
    if (item->childCount() > 0) {
      bool algumVisivel = false;

      for (int i = 0; i < item->childCount(); ++i) {
        if (filtrar(item->child(i))) { algumVisivel = true; }
      }

      // Mantém o pai visível quando algum filho passou, senão o resultado da busca aparece solto.
      item->setHidden(not algumVisivel);
      return algumVisivel;
    }

    const Item *dados = porId.value(item->data(0, Qt::UserRole).toInt());

    if (not dados) {
      item->setHidden(true);
      return false;
    }

    bool visivel = busca.isEmpty() or dados->descricao.contains(busca, Qt::CaseInsensitive) or dados->chave.contains(busca, Qt::CaseInsensitive);

    if (visivel and ui->radioButtonEscrita->isChecked()) { visivel = dados->escrita; }

    if (visivel and ui->radioButtonNegadas->isChecked()) {
      bool algumNegado = false;

      for (const QString &tipo : tipos) {
        if (not perfilAtual(tipo, dados->idPermissao)) { algumNegado = true; }
      }

      visivel = algumNegado;
    }

    if (visivel and ui->radioButtonAlteradas->isChecked()) {
      bool alterado = false;

      for (const QString &tipo : tipos) {
        if (pendentesPerfil.contains({tipo, dados->idPermissao})) { alterado = true; }
      }

      visivel = alterado;
    }

    item->setHidden(not visivel);
    return visivel;
  };

  for (int i = 0; i < ui->treePerfis->topLevelItemCount(); ++i) { filtrar(ui->treePerfis->topLevelItem(i)); }

  if (not busca.isEmpty() or not ui->radioButtonTodas->isChecked()) { ui->treePerfis->expandAll(); }
}

void GerenciarPermissoes::on_pushButtonCopiarPerfil_clicked() {
  bool ok = false;

  const QString origem = QInputDialog::getItem(this, "Copiar perfil", "Copiar DE:", tipos, 0, false, &ok);
  if (not ok) { return; }

  const QString destino = QInputDialog::getItem(this, "Copiar perfil", "Copiar PARA:", tipos, 0, false, &ok);
  if (not ok or origem == destino) { return; }

  // Só para a memória: o admin revisa na matriz e decide se salva.
  for (const Item &item : catalogo) {
    const bool valor = perfilAtual(origem, item.idPermissao);

    if (perfilBase.value({destino, item.idPermissao}, false) == valor) {
      pendentesPerfil.remove({destino, item.idPermissao});
    } else {
      pendentesPerfil.insert({destino, item.idPermissao}, valor);
    }
  }

  carregando = true;
  const int coluna = tipos.indexOf(destino) + 1;
  unsetConnections();

  std::function<void(QTreeWidgetItem *)> repintar = [&](QTreeWidgetItem *item) {
    if (item->childCount() == 0) {
      item->setCheckState(coluna, perfilAtual(destino, item->data(0, Qt::UserRole).toInt()) ? Qt::Checked : Qt::Unchecked);
      return;
    }

    for (int i = 0; i < item->childCount(); ++i) { repintar(item->child(i)); }
  };

  for (int i = 0; i < ui->treePerfis->topLevelItemCount(); ++i) {
    repintar(ui->treePerfis->topLevelItem(i));
    propagarEstadoPais(ui->treePerfis->topLevelItem(i), coluna);
  }

  setConnections();
  carregando = false;
  atualizarContadorPerfis();

  qApp->enqueueInformation(QString("Perfil de %1 copiado para %2. Revise e clique em Salvar.").arg(origem, destino), this);
}

void GerenciarPermissoes::on_pushButtonSalvar_clicked() {
  if (pendentesPerfil.isEmpty()) { return; }

  qApp->startTransaction("GerenciarPermissoes::salvarPerfis");

  SqlQuery query;
  query.prepare("REPLACE INTO perfil_has_permissao (tipo, idPermissao, permitido) VALUES (:tipo, :idPermissao, :permitido)");

  for (auto it = pendentesPerfil.constBegin(); it != pendentesPerfil.constEnd(); ++it) {
    query.bindValue(":tipo", it.key().first);
    query.bindValue(":idPermissao", it.key().second);
    query.bindValue(":permitido", it.value());

    if (not query.exec()) { throw RuntimeException("Erro salvando perfil: " + query.lastError().text(), this); }
  }

  qApp->endTransaction();

  // Reflete na sessão do próprio admin; os outros usuários pegam no próximo login.
  Permissao::carregar();

  carregarPerfis();

  qApp->enqueueInformation("Perfis salvos! As alterações valem para os outros usuários no próximo login deles.", this);
}

void GerenciarPermissoes::on_pushButtonDescartar_clicked() {
  if (pendentesPerfil.isEmpty()) { return; }

  pendentesPerfil.clear();
  carregarPerfis();
}

// ---------------------------------------------------------------------------------------------
// Aba Usuários
// ---------------------------------------------------------------------------------------------

void GerenciarPermissoes::carregarUsuarios() {
  carregando = true;

  ui->comboBoxUsuario->clear();

  SqlQuery query;

  if (not query.exec("SELECT idUsuario, nome, tipo FROM usuario WHERE desativado = FALSE ORDER BY nome")) {
    throw RuntimeException("Erro lendo usuários: " + query.lastError().text(), this);
  }

  while (query.next()) {
    ui->comboBoxUsuario->addItem(query.value("nome").toString() + " — " + query.value("tipo").toString(), query.value("idUsuario"));
  }

  carregando = false;

  carregarUsuario();
}

QString GerenciarPermissoes::idUsuarioSelecionado() const { return ui->comboBoxUsuario->currentData().toString(); }

bool GerenciarPermissoes::efetivoUsuario(const int idPermissao) const {
  if (pendentesUsuario.contains(idPermissao)) {
    const int valor = pendentesUsuario.value(idPermissao);

    if (valor >= 0) { return valor == 1; }
  } else if (overrideBase.contains(idPermissao)) {
    return overrideBase.value(idPermissao);
  }

  const QString tipo = ui->comboBoxUsuario->currentText().section(" — ", 1);

  return perfilAtual(tipo, idPermissao);
}

void GerenciarPermissoes::carregarUsuario() {
  if (carregando) { return; }

  const QString idUsuario = idUsuarioSelecionado();

  if (idUsuario.isEmpty()) { return; }

  carregando = true;
  unsetConnections();

  overrideBase.clear();
  pendentesUsuario.clear();

  SqlQuery query;
  query.prepare("SELECT idPermissao, permitido FROM usuario_has_permissao_override WHERE idUsuario = :idUsuario");
  query.bindValue(":idUsuario", idUsuario);

  if (not query.exec()) { throw RuntimeException("Erro lendo exceções: " + query.lastError().text(), this); }

  while (query.next()) { overrideBase.insert(query.value("idPermissao").toInt(), query.value("permitido").toBool()); }

  const QString tipo = ui->comboBoxUsuario->currentText().section(" — ", 1);
  ui->labelPerfilUsuario->setText(QString("perfil %1  ·  %2 exceção(ões)").arg(tipo).arg(overrideBase.size()));

  ui->treeUsuario->clear();
  const auto folhas = criarArvore(ui->treeUsuario, false);

  for (const Item &item : catalogo) {
    QTreeWidgetItem *folha = folhas.value(item.idPermissao);

    if (not folha) { continue; }

    const bool doPerfil = perfilAtual(tipo, item.idPermissao);
    const bool temOverride = overrideBase.contains(item.idPermissao);

    folha->setText(1, doPerfil ? "Permitido" : "Negado");
    folha->setForeground(1, QBrush(QColor(130, 130, 130)));
    folha->setText(2, temOverride ? (overrideBase.value(item.idPermissao) ? "Sim" : "Não") : "Herda");
    folha->setFlags(folha->flags() | Qt::ItemIsEditable);

    const bool efetivo = efetivoUsuario(item.idPermissao);
    folha->setText(3, efetivo ? "PERMITIDO" : "NEGADO");
    if (temOverride) { folha->setText(3, folha->text(3) + " ●"); }

    // Mostrar só as exceções: com a migração de paridade, 125 usuários nascem com exceção, e a
    // lista completa esconderia justamente o que interessa.
    if (ui->checkBoxSoExcecoes->isChecked() and not temOverride) { folha->setHidden(true); }
  }

  if (ui->checkBoxSoExcecoes->isChecked()) {
    std::function<bool(QTreeWidgetItem *)> podar = [&](QTreeWidgetItem *item) -> bool {
      if (item->childCount() == 0) { return not item->isHidden(); }

      bool algum = false;

      for (int i = 0; i < item->childCount(); ++i) {
        if (podar(item->child(i))) { algum = true; }
      }

      item->setHidden(not algum);
      return algum;
    };

    for (int i = 0; i < ui->treeUsuario->topLevelItemCount(); ++i) { podar(ui->treeUsuario->topLevelItem(i)); }

    ui->treeUsuario->expandAll();
  } else {
    ui->treeUsuario->expandToDepth(0);
  }

  for (int coluna = 0; coluna < ui->treeUsuario->columnCount(); ++coluna) { ui->treeUsuario->resizeColumnToContents(coluna); }

  carregando = false;
  setConnections();
  atualizarContadorUsuario();
}

void GerenciarPermissoes::on_treeUsuario_itemChanged(QTreeWidgetItem *item, const int coluna) {
  if (carregando or coluna != 2 or item->childCount() > 0) { return; }

  const int idPermissao = item->data(0, Qt::UserRole).toInt();
  const QString texto = item->text(2);
  const int valor = (texto == "Sim") ? 1 : (texto == "Não") ? 0 : -1;

  const bool tinha = overrideBase.contains(idPermissao);
  const int original = tinha ? (overrideBase.value(idPermissao) ? 1 : 0) : -1;

  if (valor == original) {
    pendentesUsuario.remove(idPermissao);
  } else {
    pendentesUsuario.insert(idPermissao, valor);
  }

  carregando = true;
  unsetConnections();

  const bool efetivo = efetivoUsuario(idPermissao);
  item->setText(3, QString(efetivo ? "PERMITIDO" : "NEGADO") + (valor >= 0 ? " ●" : ""));

  setConnections();
  carregando = false;

  atualizarContadorUsuario();
}

void GerenciarPermissoes::atualizarContadorUsuario() {
  ui->labelAlteracoesUsuario->setText(pendentesUsuario.isEmpty() ? "" : QString("● %1 alteração(ões)").arg(pendentesUsuario.size()));
  ui->labelAlteracoesUsuario->setStyleSheet(pendentesUsuario.isEmpty() ? "" : "color: rgb(200, 120, 0); font-weight: bold;");
}

void GerenciarPermissoes::on_pushButtonSalvarUsuario_clicked() {
  if (pendentesUsuario.isEmpty()) { return; }

  const QString idUsuario = idUsuarioSelecionado();

  qApp->startTransaction("GerenciarPermissoes::salvarExcecoes");

  SqlQuery queryInsere;
  queryInsere.prepare("REPLACE INTO usuario_has_permissao_override (idUsuario, idPermissao, permitido) VALUES (:idUsuario, :idPermissao, :permitido)");

  SqlQuery queryRemove;
  queryRemove.prepare("DELETE FROM usuario_has_permissao_override WHERE idUsuario = :idUsuario AND idPermissao = :idPermissao");

  for (auto it = pendentesUsuario.constBegin(); it != pendentesUsuario.constEnd(); ++it) {
    if (it.value() < 0) { // "Herda" = sem linha de override
      queryRemove.bindValue(":idUsuario", idUsuario);
      queryRemove.bindValue(":idPermissao", it.key());

      if (not queryRemove.exec()) { throw RuntimeException("Erro removendo exceção: " + queryRemove.lastError().text(), this); }

      continue;
    }

    queryInsere.bindValue(":idUsuario", idUsuario);
    queryInsere.bindValue(":idPermissao", it.key());
    queryInsere.bindValue(":permitido", it.value());

    if (not queryInsere.exec()) { throw RuntimeException("Erro salvando exceção: " + queryInsere.lastError().text(), this); }
  }

  qApp->endTransaction();

  if (idUsuario == User::idUsuario) { Permissao::carregar(); }

  carregarUsuario();

  qApp->enqueueInformation("Exceções salvas! Valem para o usuário no próximo login dele.", this);
}

void GerenciarPermissoes::on_pushButtonDescartarUsuario_clicked() {
  if (pendentesUsuario.isEmpty()) { return; }

  pendentesUsuario.clear();
  carregarUsuario();
}

void GerenciarPermissoes::mostrarUsuario(const QString &idUsuario) {
  ui->tabWidget->setCurrentWidget(ui->tabUsuarios);

  const int indice = ui->comboBoxUsuario->findData(idUsuario);

  if (indice != -1) { ui->comboBoxUsuario->setCurrentIndex(indice); }
}

// ---------------------------------------------------------------------------------------------
// Aba Quem pode?
// ---------------------------------------------------------------------------------------------

void GerenciarPermissoes::carregarPermissoesCombo() {
  if (ui->comboBoxPermissao->count() == 0) {
    carregando = true;

    for (const Item &item : catalogo) {
      ui->comboBoxPermissao->addItem(item.modulo + " › " + item.tela + " › " + item.descricao, item.idPermissao);
    }

    carregando = false;
  }

  const int idPermissao = ui->comboBoxPermissao->currentData().toInt();

  if (idPermissao == 0) { return; }

  // Mesma regra de Permissao::tem(): interruptor global primeiro.
  SqlQuery queryConfig;
  bool ativo = true;

  if (queryConfig.exec("SELECT permissoesAtivas FROM config WHERE idConfig = 1") and queryConfig.first()) { ativo = queryConfig.value("permissoesAtivas").toBool(); }

  if (not ativo) {
    modelQuemPode.setQuery("SELECT u.nome AS `Nome`, u.tipo AS `Tipo`, l.descricao AS `Loja`, "
                           "'Interruptor global ●' AS `Origem` "
                           "FROM usuario u "
                           "LEFT JOIN loja l ON l.idLoja = u.idLoja "
                           "WHERE u.desativado = FALSE "
                           "ORDER BY u.tipo, u.nome");

    modelQuemPode.select();

    ui->tableQuemPode->setModel(&modelQuemPode);

    ui->labelResumoQuemPode->setText("→ todos os usuários (config.permissoesAtivas = 0, interruptor global libera tudo)");

    return;
  }

  // COALESCE invertido: quem, hoje, tem a permissão - e se veio do perfil ou de uma exceção. O JOIN
  // com perfisComTipo exclui tipo sem nenhuma linha em perfil_has_permissao: Permissao::tem() nega
  // tudo pra esse tipo (fail-closed), mesmo com uma exceção solta em usuario_has_permissao_override.
  modelQuemPode.setQuery(QString("SELECT u.nome AS `Nome`, u.tipo AS `Tipo`, l.descricao AS `Loja`, "
                                 "IF(o.idPermissao IS NULL, 'Perfil', 'Exceção ●') AS `Origem` "
                                 "FROM usuario u "
                                 "LEFT JOIN loja l ON l.idLoja = u.idLoja "
                                 "JOIN (SELECT tipo FROM perfil_has_permissao GROUP BY tipo) perfisComTipo ON perfisComTipo.tipo = u.tipo "
                                 "LEFT JOIN perfil_has_permissao f ON f.tipo = u.tipo AND f.idPermissao = %1 "
                                 "LEFT JOIN usuario_has_permissao_override o ON o.idUsuario = u.idUsuario AND o.idPermissao = %1 "
                                 "WHERE u.desativado = FALSE AND COALESCE(o.permitido, f.permitido, 0) = 1 "
                                 "ORDER BY u.tipo, u.nome")
                             .arg(idPermissao));

  modelQuemPode.select();

  ui->tableQuemPode->setModel(&modelQuemPode);

  SqlQuery query;
  query.prepare("SELECT u.tipo, COUNT(*) AS total, SUM(o.idPermissao IS NOT NULL) AS porExcecao FROM usuario u "
                "JOIN (SELECT tipo FROM perfil_has_permissao GROUP BY tipo) perfisComTipo ON perfisComTipo.tipo = u.tipo "
                "LEFT JOIN perfil_has_permissao f ON f.tipo = u.tipo AND f.idPermissao = :id "
                "LEFT JOIN usuario_has_permissao_override o ON o.idUsuario = u.idUsuario AND o.idPermissao = :id2 "
                "WHERE u.desativado = FALSE AND COALESCE(o.permitido, f.permitido, 0) = 1 GROUP BY u.tipo ORDER BY total DESC");
  query.bindValue(":id", idPermissao);
  query.bindValue(":id2", idPermissao);

  QStringList partes;
  int total = 0;
  int porExcecao = 0;

  if (query.exec()) {
    while (query.next()) {
      total += query.value("total").toInt();
      porExcecao += query.value("porExcecao").toInt();
      partes << QString("%1 %2").arg(query.value("total").toInt()).arg(query.value("tipo").toString());
    }
  }

  ui->labelResumoQuemPode->setText(QString("→ %1 usuário(s): %2%3")
                                       .arg(total)
                                       .arg(partes.join(", "))
                                       .arg(porExcecao > 0 ? QString("   (%1 via exceção ●)").arg(porExcecao) : ""));
}

// ---------------------------------------------------------------------------------------------

bool GerenciarPermissoes::temPendencias() const { return not pendentesPerfil.isEmpty() or not pendentesUsuario.isEmpty(); }

void GerenciarPermissoes::closeEvent(QCloseEvent *event) {
  if (not temPendencias()) { return QDialog::closeEvent(event); }

  QMessageBox msgBox(QMessageBox::Question, "Atenção!", "Há alterações não salvas. Descartar?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.button(QMessageBox::Yes)->setText("Descartar");
  msgBox.button(QMessageBox::No)->setText("Voltar");

  if (msgBox.exec() == QMessageBox::No) { return event->ignore(); }

  QDialog::closeEvent(event);
}

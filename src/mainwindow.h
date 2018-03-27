#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>

namespace Ui {
class MainWindow;
}

class MainWindow final : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow();
  auto updateTables() -> void;

private:
  // attributes
  Ui::MainWindow *ui;
  QPushButton *pushButtonStatus;
  // methods
  auto event(QEvent *event) -> bool;
  auto gerarEnviarRelatorio() -> void;
  auto on_actionCadastrarCliente_triggered() -> void;
  auto on_actionCadastrarFornecedor_triggered() -> void;
  auto on_actionCadastrarProdutos_triggered() -> void;
  auto on_actionCadastrarProfissional_triggered() -> void;
  auto on_actionCadastrarUsuario_triggered() -> void;
  auto on_actionCalculadora_triggered() -> void;
  auto on_actionCalcular_frete_triggered() -> void;
  auto on_actionClaro_triggered() -> void;
  auto on_actionConfiguracoes_triggered() -> void;
  auto on_actionCriarOrcamento_triggered() -> void;
  auto on_actionEscuro_triggered() -> void;
  auto on_actionEstoque_triggered() -> void;
  auto on_actionGerenciar_Lojas_triggered() -> void;
  auto on_actionGerenciar_Transportadoras_triggered() -> void;
  auto on_actionGerenciar_preco_estoque_triggered() -> void;
  auto on_actionProdutos_triggered() -> void;
  auto on_actionPromocao_triggered() -> void;
  auto on_actionSobre_triggered() -> void;
  auto on_tabWidget_currentChanged(const int) -> void;
  auto verifyDb() -> void;
};

#endif // MAINWINDOW_H

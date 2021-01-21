#pragma once

#include <QProgressDialog>

namespace Ui {
class SendMail;
}

class SendMail final : public QDialog {
  Q_OBJECT

public:
  enum class Tipo { GerarCompra, Teste, Vazio };

  explicit SendMail(const Tipo tipo, const QString &arquivo = QString(), const QString &fornecedor = QString(), QWidget *parent = nullptr);
  explicit SendMail(const Tipo tipo, QWidget *parent = nullptr);
  ~SendMail() final;

private:
  // attributes
  QProgressDialog *progress;
  QString const fornecedor;
  QStringList files;
  Tipo tipo;
  Ui::SendMail *ui;
  // methods
  auto failureStatus(const QString &status) -> void;
  auto mailSent(const QString &status) -> void;
  auto on_pushButtonBuscar_clicked() -> void;
  auto on_pushButtonEnviar_clicked() -> void;
  auto setConnections() -> void;
  auto successStatus() -> void;
};

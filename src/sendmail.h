#ifndef SENDMAIL_H
#define SENDMAIL_H

#include <QDialog>
#include <QProgressDialog>

namespace Ui {
class SendMail;
}

class SendMail final : public QDialog {
  Q_OBJECT

public:
  enum class Tipo { GerarCompra, CancelarNFe, Vazio };
  explicit SendMail(const Tipo tipo, const QString &arquivo = QString(), const QString &fornecedor = QString(), QWidget *parent = nullptr);
  ~SendMail() final;

private slots:
  void on_pushButtonBuscar_clicked();
  void on_pushButtonEnviar_clicked();
  void mailSent(const QString &status);

private:
  // attributes
  const QString fornecedor;
  QProgressDialog *progress;
  QStringList files;
  Tipo tipo;
  Ui::SendMail *ui;
  // methods
  auto failureStatus(const QString &status) -> void;
  auto successStatus() -> void;
};

#endif // SENDMAIL_H

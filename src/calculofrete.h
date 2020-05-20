#pragma once

#include <QDialog>
#include <QNetworkAccessManager>

namespace Ui {
class CalculoFrete;
}

class CalculoFrete : public QDialog {
  Q_OBJECT

public:
  explicit CalculoFrete(QWidget *parent);
  ~CalculoFrete();
  auto setCliente(const QVariant &idCliente) -> void;
  auto getDistancia() -> double;

private:
  // attributes
  Ui::CalculoFrete *ui;
  QNetworkAccessManager networkManager;
  const QString searchUrl = "https://maps.googleapis.com/maps/api/distancematrix/xml?origins=%1&destinations=%2&key=%3";
  // methods
  void handleNetworkData(QNetworkReply *networkReply);
  void on_itemBoxCliente_textChanged(const QString &);
  void on_pushButton_clicked();
};

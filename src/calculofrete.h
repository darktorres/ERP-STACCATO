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
  QNetworkAccessManager networkManager;
  QString const searchUrl = "https://maps.googleapis.com/maps/api/distancematrix/xml?origins=%1&destinations=%2&key=%3";
  Ui::CalculoFrete *ui;
  // methods
  auto handleNetworkData(QNetworkReply *networkReply) -> void;
  auto on_itemBoxCliente_textChanged(const QString &) -> void;
  auto on_pushButton_clicked() -> void;
  auto setConnections() -> void;
};

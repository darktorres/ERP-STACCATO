#pragma once

#include <QDate>
#include <QString>

class QWidget;

class Romaneio {

public:
  Romaneio() = delete;

  static auto gerar(const QString &idVenda, const QDate &dataEntrega, QWidget *parent) -> QString;
};

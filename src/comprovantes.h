#pragma once

#include "sqlquerymodel.h"

#include <QDialog>
#include <QStyledItemDelegate>

class CustomDelegate final : public QStyledItemDelegate {

public:
  explicit CustomDelegate(QObject *parent = nullptr);
  ~CustomDelegate() = default;

private:
  auto displayText(const QVariant &value, const QLocale &) const -> QString final;
};

// --------------------------------------------------------------------------

namespace Ui {
class Comprovantes;
}

class Comprovantes : public QDialog {
  Q_OBJECT

public:
  explicit Comprovantes(const QString &idVenda, QWidget *parent = nullptr);
  ~Comprovantes();

private:
  // attributes
  Ui::Comprovantes *ui;
  SqlQueryModel model;
  const QString idVenda;
  // methods
  auto on_pushButtonAbrir_clicked() -> void;
  auto setFilter(const QString &idVenda) -> void;
};

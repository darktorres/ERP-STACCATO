#pragma once

#include "sqlquerymodel.h"

#include <QDialog>
#include <QStyledItemDelegate>

namespace Ui {
class Comprovantes;
}

class Comprovantes : public QDialog {
  Q_OBJECT

  class CustomDelegate final : public QStyledItemDelegate {

  public:
    explicit CustomDelegate(QObject *parent);
    ~CustomDelegate() = default;

  private:
    auto displayText(const QVariant &value, const QLocale &) const -> QString final;
  };

  // --------------------------------------------------------------------------

public:
  explicit Comprovantes(const QString &idVenda, QWidget *parent);
  ~Comprovantes();

private:
  // attributes
  SqlQueryModel model;
  const QString idVenda;
  Ui::Comprovantes *ui;
  // methods
  auto on_pushButtonAbrir_clicked() -> void;
  auto setFilter(const QString &idVenda) -> void;
};

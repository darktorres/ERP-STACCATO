#pragma once

#include "sqlquerymodel.h"

#include <QDialog>
#include <QStyledItemDelegate>

namespace Ui {
class Comprovantes;
}

class CustomDelegate final : public QStyledItemDelegate {
  Q_OBJECT

public:
  explicit CustomDelegate(QObject *parent);
  ~CustomDelegate() = default;

private:
  auto displayText(const QVariant &value, const QLocale &locale) const -> QString final;
};

class Comprovantes : public QDialog {
  Q_OBJECT

public:
  explicit Comprovantes(const QString &idVenda, QWidget *parent);
  ~Comprovantes();

private:
  // attributes
  QString const m_idVenda;
  SqlQueryModel model;
  Ui::Comprovantes *ui;
  // methods
  auto on_pushButtonAbrir_clicked() -> void;
  auto setConnections() -> void;
  auto setFilter(const QString &idVenda) -> void;
};

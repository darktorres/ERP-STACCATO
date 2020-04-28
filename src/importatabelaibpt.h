#pragma once

#include "sqltablemodel.h"

#include <QDialog>

class ImportaTabelaIBPT final : public QDialog {
public:
  explicit ImportaTabelaIBPT(QWidget *parent);
  ~ImportaTabelaIBPT() = default;
  auto importar() -> void;

private:
  // attributes
  SqlTableModel model;
  // methods
  auto setupTables() -> void;
};

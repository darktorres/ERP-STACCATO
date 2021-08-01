#pragma once

#include <QWidget>

namespace Ui {
class TabNFe;
}

class TabNFe final : public QWidget {
  Q_OBJECT

public:
  explicit TabNFe(QWidget *parent);
  ~TabNFe();

  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  Ui::TabNFe *ui;
  // methods
  auto on_tabWidgetNfe_currentChanged() -> void;
  auto setConnections() -> void;
};

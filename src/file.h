#pragma once

#include <QFile>

class File final : public QFile {
  Q_OBJECT

public:
  explicit File(const QString &name, QObject *parent = nullptr);

  auto open(OpenMode mode) -> bool;
};

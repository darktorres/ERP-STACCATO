#pragma once

#include <QFile>

class File final : public QFile {

public:
  explicit File(const QString &name);

  auto open(OpenMode mode) -> bool;
};

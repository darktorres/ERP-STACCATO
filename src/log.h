#pragma once

#include <QString>

class Log final {

public:
  Log() = delete;
  static auto createLog(const QString &message) -> bool;
};

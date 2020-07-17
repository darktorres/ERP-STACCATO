#pragma once

#include <QString>

class Log final {

public:
  Log() = delete;
  static auto createLog(const QString &tipo, const QString &message, const bool silent = false) -> bool;
};

#pragma once

#include <QString>

class Log final {

public:
  Log() = delete;
  static auto createLog(const QString &message, const bool silent = false) -> bool;
};

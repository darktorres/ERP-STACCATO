#ifndef LOG_H
#define LOG_H

#include <QString>

class Log final {

public:
  Log() = delete;
  static auto createLog(const QString &message) -> bool;
};

#endif // LOG_H

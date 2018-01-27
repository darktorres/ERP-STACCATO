#ifndef LOG_H
#define LOG_H

#include <QString>

class Log final {

public:
  Log() = delete;
  static bool createLog(const QString &message);
};

#endif // LOG_H

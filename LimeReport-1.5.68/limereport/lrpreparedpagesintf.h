#ifndef LRPREPAREDPAGESINTF_H
#define LRPREPAREDPAGESINTF_H
#include "lrglobal.h"
namespace LimeReport {
class LIMEREPORT_EXPORT IPreparedPages {
public:
  IPreparedPages() = default;

  virtual ~IPreparedPages(){};
  virtual bool loadFromFile(const QString &fileName) = 0;
  virtual bool loadFromString(const QString data) = 0;
  virtual bool loadFromByteArray(QByteArray *data) = 0;
  virtual bool saveToFile(const QString &fileName) = 0;
  virtual QString saveToString() = 0;
  virtual QByteArray saveToByteArray() = 0;
  virtual void clear() = 0;
  Q_DISABLE_COPY(IPreparedPages)
};
} // namespace LimeReport
#endif // LRPREPAREDPAGESINTF_H

#ifndef LRTEAROFFBAND_H
#define LRTEAROFFBAND_H

#include "lrbanddesignintf.h"

namespace LimeReport {

class TearOffBand : public BandDesignIntf {
  Q_OBJECT
public:
  TearOffBand(QObject *owner = nullptr, QGraphicsItem *parent = nullptr);
  virtual BaseDesignIntf *createSameTypeItem(QObject *owner = nullptr, QGraphicsItem *parent = nullptr);

protected:
  QColor bandColor() const;
  virtual bool isUnique() const { return true; }
};

} // namespace LimeReport

#endif // TEAROFFBAND_H

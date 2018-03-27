/** \file QDecQuad.cc
 * Definitions for the class QDecQuad.
 *
 * (C) Copyright by Semih Cemiloglu
 * All rights reserved, see COPYRIGHT file for details.
 *
 * $Id: QDecQuad.cc 111 2006-06-19 03:45:40Z semihc $
 *
 *
 */

#include "QDecQuad.hh"
extern "C" {
#include "decimal128.h"
}
#include "QDecDouble.hh"
#include "QDecNumber.hh"
#include "QDecPacked.hh"
#include <QTextStream>
#include <stdlib.h>

QDecQuad &QDecQuad::fromDouble(double d) {
  char str[MaxStrSize] = {0};

#if defined(_MSC_VER)
  _snprintf(str, MaxStrSize, "%.*g", QDecNumDigits, d);
#else
  snprintf(str, MaxStrSize, "%.*g", QDecNumDigits, d);
#endif

  return fromString(str);
}

QDecQuad &QDecQuad::fromHexString(const char *str) {
  QByteArray ba = QByteArray::fromHex(str);
  int size = sizeof(m_data);
  char *p = reinterpret_cast<char *>(&m_data);
  int i = 0;
  int j = size - 1;
  for (; i < size; i++, j--) p[j] = ba.at(i);

  return *this;
}

QDecQuad &QDecQuad::fromQDecDouble(const QDecDouble &d) {
  decDoubleToWider(d.data(), &m_data);
  return *this;
}

QDecQuad &QDecQuad::fromQDecNumber(const QDecNumber &n, QDecContext *c) {
  decQuadFromNumber(&m_data, n.data(), CXT(c));
  return *this;
}

QDecQuad &QDecQuad::fromQDecPacked(const QDecPacked &p) {
  fromQDecNumber(p.toQDecNumber());
  return *this;
}

double QDecQuad::toDouble() const {
  char str[MaxStrSize] = {0};
  decQuadToString(&m_data, str);
  return strtod(str, nullptr);
}

QDecDouble QDecQuad::toQDecDouble(QDecContext *c) const {
  decDouble d;
  return decDoubleFromWider(&d, &m_data, CXT(c));
}

QDecPacked QDecQuad::toQDecPacked() const { return QDecPacked(toQDecNumber()); }

QDecNumber QDecQuad::toQDecNumber() const {
  decNumber n;
  return decQuadToNumber(&m_data, &n);
}

QTextStream &operator<<(QTextStream &ts, const QDecQuad &d) {
  ts << d.toString();
  return ts;
}

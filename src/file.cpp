#include "file.h"
#include "application.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>

File::File(const QString &name) : QFile(name) {}

bool File::open(QIODevice::OpenMode mode) {
  if (mode & (QFile::WriteOnly)) {
    const QFileInfo info(fileName());
    const QDir dir = info.absoluteDir();

    // TODO: verificar como tratar quando a unidade de rede mapeada não estiver disponivel

    if (not dir.exists() and not dir.mkpath(info.absolutePath())) { throw RuntimeException("Erro criando pasta: " + info.absolutePath()); }
  }

  return QFile::open(mode);
}

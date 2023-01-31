#pragma once

#ifdef Q_OS_WIN

#include <windows.h>

class ACBrLib {

public:
  static auto gerarDanfe(const int idNFe) -> void;
  static auto gerarDanfe(const QString &fileContent, const bool openFile) -> void;

private:
  static auto check_result(HMODULE nHandler, const int ret) -> void;
  static auto trim(std::string &buffer) -> void;
};

#else

#include <QVariant>

class ACBrLib {

public:
  static auto gerarDanfe(const int idNFe) -> void;
  static auto gerarDanfe(const QString &fileContent, const bool openFile) -> void;
};

#endif

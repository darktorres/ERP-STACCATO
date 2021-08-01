#pragma once

#if _WIN32

#include <QByteArray>
#include <windows.h>

#define BUFFER_LEN 256

class ACBrLib {

public:
  static auto gerarDanfe(const int idNFe) -> void;
  static auto gerarDanfe(const QByteArray &fileContent, const bool openFile = true) -> void;

private:
  static auto check_result(HMODULE nHandler, const int ret) -> void;
  static auto trim(std::string &buffer) -> void;
};

#else

#include <QByteArray>

class ACBrLib {

public:
  static auto gerarDanfe(const int idNFe) -> void;
  static auto gerarDanfe(const QByteArray &fileContent, const bool openFile = true) -> void;
};

#endif

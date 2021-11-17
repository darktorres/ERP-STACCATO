#ifdef Q_OS_WIN

#include "acbrlib.h"

#include "application.h"
#include "sqlquery.h"

#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QSqlError>
#include <QUrl>

#define BUFFER_LEN 256

typedef int (*NFE_Inicializar)(const char *eArqConfig, const char *eChaveCrypt);
typedef int (*NFE_Finalizar)();
typedef int (*NFE_UltimoRetorno)(const char *sMensagem, int *esTamanho);
typedef int (*NFE_CarregarXML)(const char *eArquivoOuXML);
typedef int (*NFE_ImprimirPDF)();

void ACBrLib::gerarDanfe(const int idNFe) {
  if (idNFe == 0) { throw RuntimeError("Produto não possui NFe!"); }

  SqlQuery query;
  query.prepare("SELECT xml FROM nfe WHERE idNFe = :idNFe");
  query.bindValue(":idNFe", idNFe);

  if (not query.exec()) { throw RuntimeException("Erro buscando XML da NFe: " + query.lastError().text()); }

  if (not query.first()) { throw RuntimeException("Não encontrado XML para NFe de id: " + QString::number(idNFe)); }

  gerarDanfe(query.value("xml"), true);
}

void ACBrLib::gerarDanfe(const QVariant &fileContent, const bool openFile) {
  const auto fileContent2 = fileContent.toByteArray();

  if (fileContent2.contains("<resNFe")) { throw RuntimeError("XML resumido, não é possível gerar DANFE!"); }

  HMODULE nHandler = LoadLibraryW(L"ACBrNFe32.dll");

  if (not nHandler) { throw RuntimeException("Erro carregando ACBrLib!"); }

  NFE_Inicializar method_inicializar = reinterpret_cast<NFE_Inicializar>(GetProcAddress(nHandler, "NFE_Inicializar"));

  if (not method_inicializar) { throw RuntimeException("Erro ACBrLib NFE_Inicializar!"); }

  int ret;

  ret = method_inicializar("", "");

  check_result(nHandler, ret);

  // ---------------------------------------------

  NFE_CarregarXML method_carregar_xml = reinterpret_cast<NFE_CarregarXML>(GetProcAddress(nHandler, "NFE_CarregarXML"));

  if (not method_carregar_xml) { throw RuntimeException("Erro ACBrLib NFE_CarregarXML!"); }

  ret = method_carregar_xml(fileContent2);

  check_result(nHandler, ret);

  // ---------------------------------------------

  NFE_ImprimirPDF method_imprimir_pdf = reinterpret_cast<NFE_ImprimirPDF>(GetProcAddress(nHandler, "NFE_ImprimirPDF"));

  if (not method_imprimir_pdf) { throw RuntimeException("Erro ACBrLib NFE_ImprimirPDF!"); }

  ret = method_imprimir_pdf();

  check_result(nHandler, ret);

  // ---------------------------------------------

  NFE_Finalizar method_finalizar = reinterpret_cast<NFE_Finalizar>(GetProcAddress(nHandler, "NFE_Finalizar"));

  if (not method_finalizar) { throw RuntimeException("Erro ACBrLib NFE_Finalizar!"); }

  ret = method_finalizar();

  check_result(nHandler, ret);

  FreeLibrary(nHandler);

  // ---------------------------------------------

  const QString chaveAcesso = fileContent2.mid(fileContent2.indexOf("Id=") + 7, 44);
  const QString filePath = QDir::currentPath() + "/pdf/" + chaveAcesso + "-nfe.pdf";

  if (openFile) {
    if (not QDesktopServices::openUrl(QUrl::fromLocalFile(filePath))) { throw RuntimeException("Erro abrindo PDF!"); }
  }
}

void ACBrLib::trim(std::string &buffer) {
  buffer.erase(buffer.begin(), std::find_if(buffer.begin(), buffer.end(), [](unsigned char c) { return not std::isspace(c); }));

  buffer.erase(std::find_if(buffer.rbegin(), buffer.rend(), [](unsigned char c) { return not std::isspace(c); }).base(), buffer.end());
}

void ACBrLib::check_result(HMODULE nHandler, const int ret) {
  if (ret >= 0) { return; }

  std::string buffer(BUFFER_LEN, ' ');
  int bufferLen = BUFFER_LEN;

  NFE_UltimoRetorno method = reinterpret_cast<NFE_UltimoRetorno>(GetProcAddress(nHandler, "NFE_UltimoRetorno"));

  if (not method) { throw RuntimeException("Erro ACBrLib NFE_UltimoRetorno!"); }

  method(buffer.c_str(), &bufferLen);

  if (bufferLen > BUFFER_LEN) {
    buffer.clear();
    buffer.resize(bufferLen, ' ');
    method(buffer.c_str(), &bufferLen);
  }

  trim(buffer);

  const QString retorno = QString::fromStdString(buffer);

  if (retorno.contains("0 NFe(s) Carregada(s)")) { throw RuntimeException("Não foi possível ler o XML!"); }

  if (retorno.contains("Unable to create file")) { throw RuntimeException("Não foi possível criar o arquivo, verifique se ele não está aberto!"); }

  throw RuntimeException("Erro ACBrLib: " + retorno);
}

#else

#include "acbrlib.h"

#include "application.h"
#include "sqlquery.h"
#include "xml_viewer.h"

#include <QDebug>
#include <QSqlError>

void ACBrLib::gerarDanfe(const int idNFe) {
  if (idNFe == 0) { throw RuntimeError("Produto não possui NFe!"); }

  SqlQuery query;
  query.prepare("SELECT xml FROM nfe WHERE idNFe = :idNFe");
  query.bindValue(":idNFe", idNFe);

  if (not query.exec()) { throw RuntimeException("Erro buscando XML da NFe: " + query.lastError().text()); }

  if (not query.first()) { throw RuntimeException("Não encontrado XML para NFe de id: " + QString::number(idNFe)); }

  auto *viewer = new XML_Viewer(query.value("xml").toString(), nullptr);
  viewer->setAttribute(Qt::WA_DeleteOnClose);
}

void ACBrLib::gerarDanfe(const QVariant &fileContent, const bool openFile) {
  Q_UNUSED(openFile)

  auto *viewer = new XML_Viewer(fileContent.toString(), nullptr);
  viewer->setAttribute(Qt::WA_DeleteOnClose);
}

#endif

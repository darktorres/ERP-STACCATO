#include "webdav.h"

#include "application.h"
#include "user.h"

#include <QAuthenticator>
#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

auto uploadWebDav(const QString &url, const QByteArray &conteudo, QWidget *parent) -> QString {
  if (conteudo.isEmpty()) { throw RuntimeException("Arquivo vazio (0 bytes), selecione outro arquivo.", parent); }

  QNetworkAccessManager manager;
  manager.setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);

  QObject::connect(&manager, &QNetworkAccessManager::authenticationRequired, [](QNetworkReply *, QAuthenticator *authenticator) {
    authenticator->setUser(User::usuario);
    authenticator->setPassword(User::senha);
  });

  // PUT síncrono seguindo redirecionamentos. Em sucesso devolve "" e escreve a URL final em `urlFinal`.
  const auto enviar = [&](QString &urlFinal) -> QString {
    QUrl atual(url);

    for (int tentativa = 0; tentativa < 5; ++tentativa) {
      QNetworkRequest req(atual);
      QEventLoop loop;

      QNetworkReply *reply = manager.put(req, conteudo);
      QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
      loop.exec();

      const QUrl redirect = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
      const int http = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
      const QNetworkReply::NetworkError erro = reply->error();
      const QString erroStr = reply->errorString();
      reply->deleteLater();

      if (redirect.isValid()) {
        atual = redirect.isRelative() ? atual.resolved(redirect) : redirect;
        continue;
      }

      if (erro != QNetworkReply::NoError) { return erroStr + (http > 0 ? " (HTTP " + QString::number(http) + ")" : QString()); }

      urlFinal = atual.toString();
      return QString();
    }

    return "muitos redirecionamentos";
  };

  // HEAD síncrono. Retorna o tamanho do arquivo no servidor; -1 se não há Content-Length p/ verificar; -2 se não encontrado/erro.
  const auto tamanhoRemoto = [&](const QString &urlStr) -> qint64 {
    QUrl atual(urlStr);

    for (int tentativa = 0; tentativa < 5; ++tentativa) {
      QNetworkRequest req(atual);
      QEventLoop loop;

      QNetworkReply *reply = manager.head(req);
      QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
      loop.exec();

      const QUrl redirect = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
      const int http = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
      const QVariant contentLength = reply->header(QNetworkRequest::ContentLengthHeader);
      reply->deleteLater();

      if (redirect.isValid()) {
        atual = redirect.isRelative() ? atual.resolved(redirect) : redirect;
        continue;
      }

      if (http != 200) { return -2; }

      return contentLength.isValid() ? contentLength.toLongLong() : -1;
    }

    return -2;
  };

  QString ultimoErro;

  for (int tentativa = 0; tentativa < 2; ++tentativa) {
    QString urlFinal;

    const QString erro = enviar(urlFinal);

    if (not erro.isEmpty()) {
      ultimoErro = erro;
      continue;
    }

    const qint64 remoto = tamanhoRemoto(urlFinal);

    if (remoto == conteudo.size() or remoto == -1) { return urlFinal; }

    ultimoErro = (remoto == -2) ? "arquivo não encontrado no servidor após o envio"
                                : "servidor gravou " + QString::number(remoto) + " bytes (esperado " + QString::number(conteudo.size()) + ")";
  }

  throw RuntimeException("Falha ao salvar foto no servidor: " + ultimoErro, parent);
}

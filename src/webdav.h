#pragma once

#include <QByteArray>
#include <QString>

class QWidget;

// PUT síncrono de `conteudo` em `url` (WebDAV), seguindo redirecionamentos.
// Verifica via HEAD que o tamanho gravado == conteudo.size() (detecta gravação 0-byte/parcial).
// Re-tenta o envio uma vez se a verificação falhar. Lança RuntimeException em erro.
// Retorna a URL final (após redirecionamentos) em caso de sucesso.
auto uploadWebDav(const QString &url, const QByteArray &conteudo, QWidget *parent = nullptr) -> QString;

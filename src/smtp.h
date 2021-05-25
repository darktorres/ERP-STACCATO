/*
Copyright (c) 2013 Raivis Strogonovs

http://morf.lv

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.*/

#pragma once

#include <QAbstractSocket>
#include <QSslSocket>
#include <QString>

class Smtp final : public QObject {
  Q_OBJECT

public:
  enum class States { Tls, HandShake, Auth, User, Pass, Rcpt, Mail, Data, Init, Body, Quit, Close };
  Q_ENUM(States)

  Smtp(const QString &user, const QString &pass, const QString &host, const quint16 port = 465, const int timeout = 5000);
  ~Smtp() final;

  auto sendMail(const QString &from, const QString &to, const QString &cc, const QString &subject, const QString &body, const QStringList &files = QStringList(), const QString &assinatura = QString())
      -> void;

signals:
  void status(const QString &);

private:
  // attributes
  States state;
  int const timeout;
  quint16 const port;
  QSslSocket *socket;
  QString const host;
  QString const pass;
  QString const user;
  QString from;
  QString message;
  QString response;
  QStringList rcpt;
  QTextStream *t = nullptr;
  // methods
  auto connected() -> void;
  auto disconnected() -> void;
  auto errorReceived(QAbstractSocket::SocketError socketError) -> void;
  auto readyRead() -> void;
  auto setConnections() -> void;
  auto stateChanged(QAbstractSocket::SocketState socketState) -> void;
};

#pragma once

#include "logindialog.h"
#include "sqlquery.h"

#include <QSettings>

class UserSession final {

public:
  UserSession() = delete;

  static auto fromLoja(const QString &parameter, const QString &user = nome) -> QVariant;
  static auto getSetting(const QString &key) -> QVariant;
  static auto login(const QString &user, const QString &password) -> void;
  static auto autorizacao(const QString &user, const QString &password) -> void;
  static auto setSetting(const QString &key, const QVariant &value) -> void;

  inline static int idLoja = -1;
  inline static int idUsuario = -1;
  inline static QString nome = "";
  inline static QString tipoUsuario = "";
  // TODO: save user/password in memory for webdav authentication

private:
  inline static SqlQuery *query = nullptr; // defer creating query until database is set
  inline static QSettings *settings = new QSettings("Staccato", "ERP");
};

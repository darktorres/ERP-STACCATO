#ifndef USERSESSION_H
#define USERSESSION_H

#include <QSettings>
#include <QSqlQuery>
#include <optional>

class UserSession final {

public:
  enum class Tipo { Padrao, Autorizacao };

  UserSession() = delete;
  static auto fromLoja(const QString &parameter, const QString &user = nome()) -> std::optional<QVariant>;
  static auto getSetting(const QString &key) -> std::optional<QVariant>;
  static auto idLoja() -> int;
  static auto idUsuario() -> int;
  static auto login(const QString &user, const QString &password, Tipo tipo = Tipo::Padrao) -> bool;
  static auto nome() -> QString;
  static auto setSetting(const QString &key, const QVariant &value) -> void;
  static auto tipoUsuario() -> QString;

private:
  // attributes
  inline static QSqlQuery *query = nullptr;
  inline static QSettings *settings = new QSettings("Staccato", "ERP");
  // methods
  static auto initializeQuery() -> void;
};

#endif // USERSESSION_H

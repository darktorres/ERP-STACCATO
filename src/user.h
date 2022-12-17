#pragma once

#include "sqlquery.h"

#include <QSettings>

class User final {

public:
  User() = delete;

  static auto autorizacao(const QString &user, const QString &password) -> void;
  static auto fromLoja(const QString &parameter, const QString &user = nome) -> QVariant;
  static auto getSetting(const QString &key) -> QVariant;
  static auto isAdmin() -> bool;              // administrador,  diretor
  static auto isAdministrativo() -> bool;     // administrador, administrativo, diretor
  static auto isEspecial() -> bool;           // vendedor especial
  static auto isGerente() -> bool;            // gerente loja
  static auto isOperacional() -> bool;        // operacional
  static auto isVendedor() -> bool;           // vendedor
  static auto isVendedorOrEspecial() -> bool; // vendedor, vendedor especial
  static auto login(const QString &user, const QString &password) -> void;
  static auto setSetting(const QString &key, const QVariant &value) -> void;
  static auto temPermissao(const QString &permissao) -> bool;

  inline static QString idLoja = "";
  inline static QString idUsuario = "";
  inline static QString nome = "";
  inline static QString tipo = "";
  inline static QString usuario = "";
  inline static QString senha = "";

private:
  inline static SqlQuery *query = nullptr; // defer creating query until database is set
  inline static QSettings *settings = new QSettings("Staccato", "ERP");
};

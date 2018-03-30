/*
 * (C) Copyright 2014 Alex Spataru
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the GNU Lesser General Public License
 * (LGPL) version 2.1 which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/lgpl-2.1.html
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 */

#ifndef Q_SIMPLE_UPDATER_H
#define Q_SIMPLE_UPDATER_H

#if !defined(Q_OS_IOS)
#define SUPPORTS_SSL 1
#endif

#include <QApplication>
#include <QDesktopServices>
#include <QIcon>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPixmap>
#include <QUrl>

#if SUPPORTS_SSL
#include <QSsl>
#include <QSslConfiguration>
#include <QSslError>
#endif

#include "dialogs/download_dialog.h"
#include "dialogs/progress_dialog.h"

class QSimpleUpdater : public QObject {
  Q_OBJECT

public:
  explicit QSimpleUpdater(QObject *parent = nullptr);
  auto changeLog() const -> QString;
  auto checkForUpdates() -> void;
  auto downloadLatestVersion() -> void;
  auto installedVersion() const -> QString;
  auto latestVersion() const -> QString;
  auto newerVersionAvailable() const -> bool;
  auto openDownloadLink() -> void;
  auto setApplicationVersion(const QString &version) -> void;
  auto setChangelogUrl(const QString &url) -> void;
  auto setDownloadUrl(const QString &url) -> void;
  auto setReferenceUrl(const QString &url) -> void;
  auto setShowNewestVersionMessage(bool show) -> void;
  auto setShowUpdateAvailableMessage(bool show) -> void;
  auto setSilent(bool silent) -> void;
  auto silent() const -> bool;

signals:
  void checkingFinished();

private:
  // attributes
  DownloadDialog *m_downloadDialog;
  ProgressDialog *m_progressDialog;
  QNetworkAccessManager *m_manager;
  QString m_changelog;
  QString m_installed_version;
  QString m_latest_version;
  QUrl m_changelog_url;
  QUrl m_download_url;
  QUrl m_reference_url;
  bool m_new_version_available;
  bool m_show_newest_version;
  bool m_show_update_available;
  bool m_silent;
  // methods
  auto cancel() -> void;
  auto checkDownloadedVersion(QNetworkReply *reply) -> void;
  auto ignoreSslErrors(QNetworkReply *reply, const QList<QSslError> &error) -> void;
  auto onCheckingFinished() -> void;
  auto processDownloadedChangelog(QNetworkReply *reply) -> void;
  auto showErrorMessage() -> void;
};

#endif

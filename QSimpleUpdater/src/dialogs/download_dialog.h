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

#ifndef DOWNLOAD_DIALOG_H
#define DOWNLOAD_DIALOG_H

#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QIcon>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include <cmath>

namespace Ui {
class DownloadDialog;
}

class DownloadDialog : public QWidget {
  Q_OBJECT

public:
  explicit DownloadDialog(QWidget *parent = nullptr);
  ~DownloadDialog();
  auto beginDownload(const QUrl &url) -> void;

private:
  // attributes
  Ui::DownloadDialog *ui;
  QString m_path;
  QNetworkReply *m_reply;
  QNetworkAccessManager *m_manager;
  QDateTime m_start_time;
  // methods
  auto cancelDownload() -> void;
  auto downloadFinished() -> void;
  auto ignoreSslErrors(QNetworkReply *reply, const QList<QSslError> &error) -> void;
  auto installUpdate() -> void;
  auto openDownload() -> void;
  auto roundNumber(const double &input)->double;
  auto updateProgress(qint64 received, qint64 total) -> void;
};

#endif

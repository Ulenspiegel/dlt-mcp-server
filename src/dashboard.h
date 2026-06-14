/* Copyright (C) 2026, Mikhail Nikonov <michael.n.nikonov@gmail.com>
 *
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License (MPL), v. 2.0.
 * If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef DASHBOARD_H
#define DASHBOARD_H

#include <QWidget>

#include "config.h"

class DltMcpServer;
class QLabel;
class QPushButton;
class QSettings;
class QTabWidget;
class QTextBrowser;
class QUrl;

class Dashboard : public QWidget {
  Q_OBJECT

 public:
  explicit Dashboard(QSettings* settings, DltMcpServer* server,
                     QWidget* parent = nullptr);
  ~Dashboard() override = default;

 public slots:
  void setReport(const std::string& markdown);
  void clearReport();
  void jumpToMessage(int index);

 signals:
  void openSettings();

 protected:
  void showEvent(QShowEvent* event) override;
  void timerEvent(QTimerEvent* ev) override;

 private:
  void onAnchorClicked(const QUrl& url);
  void updateContextWarning();
  void checkServerStatus();
  void updateFileCount(int count);

  QSettings* settings_ = nullptr;
  DltMcpServer* server_ = nullptr;
  QTabWidget* tabWidget_;
  QTextBrowser* reportBrowser_;
  QLabel* statusLabel_;
  QLabel* portLabel_;
  QLabel* fileCountLabel_;
  QLabel* contextWarningLabel_;
  QPushButton* sseCopyBtn_;
  QPushButton* httpCopyBtn_;
  QPushButton* settingsBtn_;
  int port_ = DefaultPort;
  int restoreTimerId_ = 0;
  QPushButton* pressedBtn_ = nullptr;
  QString originalText_;
};

#endif  // DASHBOARD_H

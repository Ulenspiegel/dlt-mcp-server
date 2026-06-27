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
#include "report-storage.h"

class DltMcpServer;
class QLabel;
class QPushButton;
class QSettings;
class QSplitter;
class QTabWidget;
class QTextBrowser;
class QToolButton;
class QUrl;

class ReportBrowserWidget;
class ReportHeaderWidget;

class Dashboard : public QWidget {
  Q_OBJECT

 public:
  explicit Dashboard(QSettings* settings, DltMcpServer* server,
                     QWidget* parent = nullptr);
  ~Dashboard() override = default;

 public slots:
  void setReport(const std::string& markdown);
  void setReportTitle(const std::string& title);
  void showReport(const ReportStorage::Report& report);
  void clearReport();
  void jumpToMessage(int index);

 signals:
  void openSettings();

 protected:
  void showEvent(QShowEvent* event) override;
  void timerEvent(QTimerEvent* ev) override;

 private:
  void onAnchorClicked(const QUrl& url);
  void toggleReportBrowser();
  void updateContextWarning();
  void checkServerStatus();
  void updateFileCount(int count);
  void updateReportMismatch();

  QSettings* settings_ = nullptr;
  DltMcpServer* server_ = nullptr;
  QTabWidget* tabWidget_;
  QTextBrowser* reportBrowser_;
  QSplitter* reportSplitter_;
  ReportBrowserWidget* reportBrowserWidget_;
  ReportHeaderWidget* reportHeaderWidget_;
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
  std::string currentReportHash_;
};

#endif  // DASHBOARD_H

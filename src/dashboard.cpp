/* Copyright (C) 2026, Mikhail Nikonov <michael.n.nikonov@gmail.com>
 *
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License (MPL), v. 2.0.
 * If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "dashboard.h"

#include <md4c-html.h>

#include <QApplication>
#include <QClipboard>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QTabWidget>
#include <QTextBrowser>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <filesystem>
#include <string>

#include "dlt-mcp-server.h"

static std::string markdownToHtml(const std::string& markdown) {
  std::string html;
  md_html(
      markdown.data(), markdown.size(),
      +[](const MD_CHAR* data, MD_SIZE size, void* userdata) {
        static_cast<std::string*>(userdata)->append(data, size);
      },
      &html, MD_DIALECT_GITHUB, 0);
  return html;
}

Dashboard::Dashboard(QSettings* settings, DltMcpServer* server, QWidget* parent)
    : QWidget(parent),
      settings_(settings),
      server_(server),
      tabWidget_(new QTabWidget(this)),
      reportBrowser_(new QTextBrowser(this)),
      statusLabel_(new QLabel(this)),
      portLabel_(new QLabel(this)),
      fileCountLabel_(new QLabel(this)),
      contextWarningLabel_(new QLabel(this)),
      sseCopyBtn_(new QPushButton("Copy SSE URL", this)),
      httpCopyBtn_(new QPushButton("Copy HTTP URL", this)),
      settingsBtn_(new QPushButton("Settings", this)) {
  port_ = settings_->value(PortKey, DefaultPort).toInt();

  // Report tab
  reportBrowser_->setOpenLinks(false);
  reportBrowser_->setPalette(QApplication::palette());
  reportBrowser_->setPlaceholderText("No report generated");
  connect(reportBrowser_, &QTextBrowser::anchorClicked, this,
          &Dashboard::onAnchorClicked);
  tabWidget_->addTab(reportBrowser_, "Report");

  // Status labels
  statusLabel_->setText("Starting...");
  statusLabel_->setStyleSheet("color: #9e9e9e; font-size: 12px;");

  portLabel_->setText(QString("localhost:%1").arg(port_));
  portLabel_->setStyleSheet("color: #9e9e9e; font-size: 12px;");

  fileCountLabel_->setStyleSheet("color: #9e9e9e; font-size: 12px;");

  contextWarningLabel_->setVisible(false);
  contextWarningLabel_->setStyleSheet("color: #ff9800; font-size: 12px;");

  // Status bar layout - single line
  auto* statusBarLayout = new QHBoxLayout();
  statusBarLayout->addWidget(statusLabel_);
  statusBarLayout->addSpacing(8);
  statusBarLayout->addWidget(portLabel_);
  statusBarLayout->addSpacing(8);
  statusBarLayout->addWidget(fileCountLabel_);
  statusBarLayout->addSpacing(8);
  statusBarLayout->addWidget(contextWarningLabel_);
  statusBarLayout->addStretch();
  statusBarLayout->addWidget(sseCopyBtn_);
  statusBarLayout->addWidget(httpCopyBtn_);
  statusBarLayout->addWidget(settingsBtn_);

  auto* mainLayout = new QVBoxLayout(this);
  mainLayout->addWidget(tabWidget_, 1);
  mainLayout->addLayout(statusBarLayout);

  connect(sseCopyBtn_, &QPushButton::clicked, [this]() {
    QApplication::clipboard()->setText(
        QString("http://localhost:%1/sse").arg(port_));
    pressedBtn_ = sseCopyBtn_;
    originalText_ = sseCopyBtn_->text();
    sseCopyBtn_->setText("SSE URL copied!");
    restoreTimerId_ = startTimer(1500);
  });

  connect(httpCopyBtn_, &QPushButton::clicked, [this]() {
    QApplication::clipboard()->setText(
        QString("http://localhost:%1/mcp").arg(port_));
    pressedBtn_ = httpCopyBtn_;
    originalText_ = httpCopyBtn_->text();
    httpCopyBtn_->setText("HTTP URL copied!");
    restoreTimerId_ = startTimer(1500);
  });

  connect(settingsBtn_, &QPushButton::clicked, this, &Dashboard::openSettings);

  connect(server_, &DltMcpServer::fileCountChanged, this,
          &Dashboard::updateFileCount);

  QTimer::singleShot(500, this, &Dashboard::checkServerStatus);
  updateContextWarning();
  updateFileCount(0);
}

void Dashboard::showEvent(QShowEvent* event) {
  QWidget::showEvent(event);
  updateContextWarning();
}

void Dashboard::updateContextWarning() {
  if (!settings_) {
    contextWarningLabel_->setVisible(false);
    return;
  }

  QString filePath = settings_->value(ContextFileKey).toString();
  if (filePath.isEmpty()) {
    contextWarningLabel_->setVisible(false);
    return;
  }

  std::error_code ec;
  auto path = filePath.toStdString();
  auto stat = std::filesystem::status(path, ec);
  if (ec || stat.type() != std::filesystem::file_type::regular) {
    auto file_name = std::filesystem::path(path).filename().string();
    contextWarningLabel_->setText(QString("Context file missing: %1")
                                      .arg(QString::fromStdString(file_name)));
    contextWarningLabel_->setVisible(true);
  } else {
    contextWarningLabel_->setVisible(false);
  }
}

void Dashboard::checkServerStatus() {
  if (server_ && server_->isServerRunning()) {
    statusLabel_->setText("Running");
    statusLabel_->setStyleSheet("color: #4caf50; font-size: 12px;");
  } else {
    statusLabel_->setText("Failed");
    statusLabel_->setStyleSheet("color: #f44336; font-size: 12px;");
  }
}

void Dashboard::updateFileCount(int count) {
  if (count == 0) {
    fileCountLabel_->setText("No files loaded");
  } else {
    fileCountLabel_->setText(QString("%1 file(s) loaded").arg(count));
  }
}

void Dashboard::timerEvent(QTimerEvent* ev) {
  if (ev->timerId() == restoreTimerId_ && pressedBtn_) {
    pressedBtn_->setText(originalText_);
    pressedBtn_ = nullptr;
    restoreTimerId_ = 0;
  }
  QWidget::timerEvent(ev);
}

void Dashboard::setReport(const std::string& markdown) {
  std::string html = markdownToHtml(markdown);
  reportBrowser_->setHtml(QString::fromUtf8(html.data(), html.size()));
}

void Dashboard::clearReport() { reportBrowser_->clear(); }

void Dashboard::onAnchorClicked(const QUrl& url) {
  if (url.scheme() != "jump" || url.authority() != "msg") {
    return;
  }
  QString indexStr = url.path().mid(1);
  bool ok = false;
  int index = indexStr.toInt(&ok);
  if (!ok || index < 0) {
    return;
  }
  qCDebug(logDltMcpServer) << "jump to message" << index;
  jumpToMessage(index);
}

void Dashboard::jumpToMessage(int index) {
  if (server_) {
    server_->jumpToMessage(index);
  }
}

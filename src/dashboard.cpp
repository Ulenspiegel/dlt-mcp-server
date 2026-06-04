/* Copyright (C) 2026, Mikhail Nikonov <michael.n.nikonov@gmail.com>
 *
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License (MPL), v. 2.0.
 * If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "dashboard.h"
#include "dlt-mcp-server.h"

#include <QApplication>
#include <QClipboard>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QTimer>
#include <QVBoxLayout>

#include <filesystem>

Dashboard::Dashboard(QSettings* settings, DltMcpServer* server, QWidget* parent)
    : QWidget(parent), settings_(settings), server_(server), statusLabel_(new QLabel(this)),
      fileCountLabel_(new QLabel(this)), contextWarningLabel_(new QLabel(this)),
      sseCopyBtn_(new QPushButton("Copy SSE URL", this)),
      httpCopyBtn_(new QPushButton("Copy Streamable HTTP URL", this)),
      settingsBtn_(new QPushButton("Settings...", this)) {

    port_ = settings_->value(PortKey, DefaultPort).toInt();

    statusLabel_->setText(QString("<b>MCP Server: localhost:%1</b><br>Running").arg(port_));
    statusLabel_->setStyleSheet("color: #4caf50; font-size: 14px;");
    statusLabel_->setAlignment(Qt::AlignCenter);

    fileCountLabel_->setStyleSheet("color: #9e9e9e; font-size: 12px;");
    fileCountLabel_->setAlignment(Qt::AlignCenter);

    contextWarningLabel_->setVisible(false);
    contextWarningLabel_->setStyleSheet("color: #ff9800; font-size: 12px;");
    contextWarningLabel_->setAlignment(Qt::AlignCenter);
    contextWarningLabel_->setWordWrap(true);

    auto* btnLayout = new QHBoxLayout();
    btnLayout->addWidget(sseCopyBtn_);
    btnLayout->addWidget(httpCopyBtn_);
    btnLayout->addSpacing(16);
    btnLayout->addWidget(settingsBtn_);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->addStretch();
    mainLayout->addWidget(statusLabel_);
    mainLayout->addWidget(fileCountLabel_);
    mainLayout->addWidget(contextWarningLabel_);
    mainLayout->addSpacing(16);
    mainLayout->addLayout(btnLayout);
    mainLayout->addStretch();

    connect(sseCopyBtn_, &QPushButton::clicked, [this]() {
        QApplication::clipboard()->setText(QString("http://localhost:%1/sse").arg(port_));
        pressedBtn_ = sseCopyBtn_;
        originalText_ = sseCopyBtn_->text();
        sseCopyBtn_->setText("SSE URL copied!");
        restoreTimerId_ = startTimer(1500);
    });

    connect(httpCopyBtn_, &QPushButton::clicked, [this]() {
        QApplication::clipboard()->setText(QString("http://localhost:%1/mcp").arg(port_));
        pressedBtn_ = httpCopyBtn_;
        originalText_ = httpCopyBtn_->text();
        httpCopyBtn_->setText("Streamable HTTP URL copied!");
        restoreTimerId_ = startTimer(1500);
    });

    connect(settingsBtn_, &QPushButton::clicked, this, &Dashboard::openSettings);

    connect(server_, &DltMcpServer::fileCountChanged, this, &Dashboard::updateFileCount);

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
        contextWarningLabel_->setText(QString("<b>Warning:</b> Context file not found:<br>%1")
                                          .arg(QString::fromStdString(file_name)));
        contextWarningLabel_->setVisible(true);
    } else {
        contextWarningLabel_->setVisible(false);
    }
}

void Dashboard::checkServerStatus() {
    if (server_ && !server_->isServerRunning()) {
        statusLabel_->setText("<b>MCP Server failed to start</b><br>Port may be busy");
        statusLabel_->setStyleSheet("color: #f44336; font-size: 14px;");
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

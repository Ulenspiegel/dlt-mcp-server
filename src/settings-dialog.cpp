/* Copyright (C) 2026, Mikhail Nikonov <michael.n.nikonov@gmail.com>
 *
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License (MPL), v. 2.0.
 * If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "settings-dialog.h"

#include <QBoxLayout>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QStandardPaths>

#include "config.h"

SettingsDialog::SettingsDialog(QSettings* settings, QWidget* parent)
    : QDialog(parent), settings_(settings) {
  setWindowTitle("MCP Server Settings");
  setModal(true);
  setMinimumWidth(480);

  portSpin_ = new QSpinBox(this);
  portSpin_->setRange(1, 65535);

  auto* restartHintLabel = new QLabel(this);
  restartHintLabel->setText("\u26a0");
  restartHintLabel->setStyleSheet("color: #fbbf00; font-size: 18px;");
  restartHintLabel->setToolTip("Changing port requires server restart");

  auto* portLayout = new QHBoxLayout();
  portLayout->addWidget(portSpin_);
  portLayout->addSpacing(4);
  portLayout->addWidget(restartHintLabel);
  portLayout->addStretch();

  contextFileEdit_ = new QLineEdit(this);
  contextFileEdit_->setReadOnly(true);
  contextFileEdit_->setPlaceholderText("No context file selected");

  browseBtn_ = new QPushButton("Browse...", this);
  resetBtn_ = new QPushButton("Reset", this);

  auto* browseLayout = new QHBoxLayout();
  browseLayout->addWidget(contextFileEdit_);
  browseLayout->addWidget(browseBtn_);
  browseLayout->addWidget(resetBtn_);

  auto* formLayout = new QFormLayout();
  formLayout->addRow("Server Port:", portLayout);
  formLayout->addRow("Context File:", browseLayout);
  formLayout->setSpacing(8);

  auto* buttonBox = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

  auto* mainLayout = new QVBoxLayout(this);
  mainLayout->addLayout(formLayout);
  mainLayout->addStretch();
  mainLayout->addWidget(buttonBox);

  connect(browseBtn_, &QPushButton::clicked, [this]() {
    QString dir =
        QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    QString filePath =
        QFileDialog::getOpenFileName(this, "Select Context File", dir,
                                     "Text Files (*.txt *.md);;All Files (*)");
    if (!filePath.isEmpty()) {
      contextFileEdit_->setText(filePath);
    }
  });

  connect(resetBtn_, &QPushButton::clicked,
          [this]() { contextFileEdit_->clear(); });

  connect(buttonBox, &QDialogButtonBox::accepted, this, [this]() {
    saveSettings();
    accept();
  });

  connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

  loadSettings();
}

int SettingsDialog::getPort() const { return portSpin_->value(); }

QString SettingsDialog::getContextFilePath() const {
  return contextFileEdit_->text().trimmed();
}

void SettingsDialog::loadSettings() {
  portSpin_->setValue(settings_->value(PortKey, DefaultPort).toInt());
  QString ctxFile = settings_->value(ContextFileKey).toString();
  if (!ctxFile.isEmpty()) {
    contextFileEdit_->setText(ctxFile);
  }
}

void SettingsDialog::saveSettings() {
  settings_->setValue(PortKey, portSpin_->value());
  QString ctxFile = contextFileEdit_->text().trimmed();
  if (ctxFile.isEmpty()) {
    settings_->remove(ContextFileKey);
  } else {
    settings_->setValue(ContextFileKey, ctxFile);
  }
}

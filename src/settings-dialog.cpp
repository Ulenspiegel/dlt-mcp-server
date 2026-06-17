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
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QStandardPaths>
#include <filesystem>

#include "config.h"

namespace {
constexpr const char* kHintStyle =
    "color: #757575; font-size: 11px; font-style: italic;";
constexpr const char* kWarningStyle =
    "color: #f44336; font-size: 11px; font-style: italic;";
}  // namespace

SettingsDialog::SettingsDialog(QSettings* settings, QWidget* parent)
    : QDialog(parent), settings_(settings) {
  setWindowTitle("MCP Server Settings");
  setModal(true);
  setMinimumWidth(520);

  // Port widgets
  portSpin_ = new QSpinBox(this);
  portSpin_->setRange(1, 65535);

  portHintLabel_ = new QLabel(this);
  portHintLabel_->setText("Requires DLT Viewer restart to take effect");
  portHintLabel_->setStyleSheet(kHintStyle);

  auto* portContent = new QVBoxLayout();
  portContent->setSpacing(2);
  {
    auto* hLayout = new QHBoxLayout();
    hLayout->addWidget(portSpin_);
    hLayout->addStretch();
    portContent->addLayout(hLayout);
    portContent->addWidget(portHintLabel_);
  }

  // Context file widgets
  contextFileEdit_ = new QLineEdit(this);
  contextFileEdit_->setReadOnly(true);
  contextFileEdit_->setPlaceholderText("No context file selected");

  browseBtn_ = new QPushButton("Browse...", this);
  resetBtn_ = new QPushButton("Reset", this);

  contextWarningLabel_ = new QLabel(this);
  contextWarningLabel_->setText("\u26a0 File does not exist");
  contextWarningLabel_->setStyleSheet(kWarningStyle);
  contextWarningLabel_->setVisible(false);

  auto* contextContent = new QVBoxLayout();
  contextContent->setSpacing(2);
  {
    auto* hLayout = new QHBoxLayout();
    hLayout->addWidget(contextFileEdit_, 1);
    hLayout->addWidget(browseBtn_);
    hLayout->addWidget(resetBtn_);
    contextContent->addLayout(hLayout);
    contextContent->addWidget(contextWarningLabel_);
  }

  // Server group box
  auto* serverGroup = new QGroupBox("Server", this);
  auto* serverForm = new QFormLayout();
  serverForm->addRow("Port:", portContent);
  serverForm->addRow("Context file:", contextContent);
  serverForm->setSpacing(12);
  serverGroup->setLayout(serverForm);

  // Button box
  auto* buttonBox = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

  // Main layout
  auto* mainLayout = new QVBoxLayout(this);
  mainLayout->addWidget(serverGroup);
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
      validateContextFile();
    }
  });

  connect(resetBtn_, &QPushButton::clicked, [this]() {
    contextFileEdit_->clear();
    validateContextFile();
  });

  connect(buttonBox, &QDialogButtonBox::accepted, this, [this]() {
    saveSettings();
    accept();
  });

  connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

  loadSettings();
  validateContextFile();
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

void SettingsDialog::validateContextFile() {
  QString filePath = contextFileEdit_->text().trimmed();
  if (filePath.isEmpty()) {
    contextWarningLabel_->setVisible(false);
    return;
  }

  std::error_code ec;
  auto path = filePath.toStdString();
  auto stat = std::filesystem::status(path, ec);
  if (ec || stat.type() != std::filesystem::file_type::regular) {
    contextWarningLabel_->setVisible(true);
  } else {
    contextWarningLabel_->setVisible(false);
  }
}

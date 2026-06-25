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
#include <QSlider>
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

  // Index filter size widgets
  filterSizeSlider_ = new QSlider(Qt::Horizontal, this);
  filterSizeSlider_->setRange(0, 3);
  filterSizeSlider_->setSingleStep(1);
  filterSizeSlider_->setPageStep(1);
  filterSizeSlider_->setTickPosition(QSlider::TickPosition::TicksAbove);
  filterSizeSlider_->setTickInterval(1);

  filterSizeDescLabel_ = new QLabel(this);
  filterSizeDescLabel_->setStyleSheet(kHintStyle);
  filterSizeDescLabel_->setWordWrap(true);

  auto* filterHintLabel = new QLabel(this);
  filterHintLabel->setText("Takes effect on next file reload");
  filterHintLabel->setStyleSheet(kHintStyle);

  auto* filterContent = new QVBoxLayout();
  filterContent->setSpacing(4);
  {
    auto* tickLabels = new QHBoxLayout();
    tickLabels->setSpacing(0);
    tickLabels->setContentsMargins(0, 0, 0, 0);
    {
      auto* offLabel = new QLabel("Off", this);
      auto* smallLabel = new QLabel("Small", this);
      auto* normalLabel = new QLabel("Normal", this);
      auto* largeLabel = new QLabel("Large", this);
      tickLabels->addWidget(offLabel);
      tickLabels->addStretch();
      tickLabels->addWidget(smallLabel);
      tickLabels->addStretch();
      tickLabels->addWidget(normalLabel);
      tickLabels->addStretch();
      tickLabels->addWidget(largeLabel);
    }
    filterContent->addLayout(tickLabels);
    filterContent->addWidget(filterSizeSlider_);
    filterContent->addWidget(filterSizeDescLabel_);
    filterContent->addWidget(filterHintLabel);
  }

  // Keyword index group box
  auto* indexGroup = new QGroupBox("Keyword Index", this);
  auto* indexForm = new QFormLayout();
  indexForm->setLabelAlignment(Qt::AlignLeft | Qt::AlignTop);
  indexForm->addRow("Index size:", filterContent);
  indexForm->setSpacing(12);
  indexGroup->setLayout(indexForm);

  // Button box
  auto* buttonBox = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

  // Main layout
  auto* mainLayout = new QVBoxLayout(this);
  mainLayout->addWidget(serverGroup);
  mainLayout->addWidget(indexGroup);
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

  connect(filterSizeSlider_, &QSlider::valueChanged, this,
          &SettingsDialog::updateFilterSizeDescription);

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
  filterSizeSlider_->setValue(
      settings_->value(BloomFilterSizeKey, DefaultBloomFilterSize).toInt());
  updateFilterSizeDescription();
}

void SettingsDialog::saveSettings() {
  settings_->setValue(PortKey, portSpin_->value());
  QString ctxFile = contextFileEdit_->text().trimmed();
  if (ctxFile.isEmpty()) {
    settings_->remove(ContextFileKey);
  } else {
    settings_->setValue(ContextFileKey, ctxFile);
  }
  int filterSize = filterSizeSlider_->value();
  if (filterSize == DefaultBloomFilterSize) {
    settings_->remove(BloomFilterSizeKey);
  } else {
    settings_->setValue(BloomFilterSizeKey, filterSize);
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

void SettingsDialog::updateFilterSizeDescription() {
  const char* descriptions[] = {
      "Full text index disabled. No additional memory usage for indexing.",
      "For memory-constrained systems. Memory usage per 1M lines: 16 MB",
      "Balanced performance and memory. Memory usage per 1M lines: 32 MB",
      "Most accurate indexing. Memory usage per 1M lines: 64 MB",
  };
  int idx = filterSizeSlider_->value();
  if (idx < 0 || idx > 3) idx = 0;
  filterSizeDescLabel_->setText(descriptions[idx]);
}

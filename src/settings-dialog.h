/* Copyright (C) 2026, Mikhail Nikonov <michael.n.nikonov@gmail.com>
 *
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License (MPL), v. 2.0.
 * If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <QDialog>

class QSpinBox;
class QLineEdit;
class QPushButton;
class QSettings;

class SettingsDialog : public QDialog {
  Q_OBJECT

 public:
  explicit SettingsDialog(QSettings* settings, QWidget* parent = nullptr);
  ~SettingsDialog() override = default;

  int getPort() const;
  QString getContextFilePath() const;

 private:
  void loadSettings();
  void saveSettings();

  QSettings* settings_;
  QSpinBox* portSpin_;
  QLineEdit* contextFileEdit_;
  QPushButton* browseBtn_;
  QPushButton* resetBtn_;
};

#endif  // SETTINGS_DIALOG_H

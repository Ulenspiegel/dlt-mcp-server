/* Copyright (C) 2026, Mikhail Nikonov <michael.n.nikonov@gmail.com>
 *
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License (MPL), v. 2.0.
 * If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef DLT_MCP_SERVER_REPORT_HEADER_H_
#define DLT_MCP_SERVER_REPORT_HEADER_H_

#include <QWidget>

class QLabel;
class QToolButton;

class ReportHeaderWidget : public QWidget {
  Q_OBJECT

 public:
  explicit ReportHeaderWidget(QWidget* parent = nullptr);

  void setTitle(const QString& title);
  QString title() const;
  void setToggleArrow(const QString& arrow);
  void setMismatch(bool mismatch);

 signals:
  void toggleClicked();

 protected:
  void resizeEvent(QResizeEvent* event) override;

 private:
  void applyTitleStyle();
  QToolButton* toggleBtn_;
  QLabel* titleLabel_;
  QString originalTitle_;
  bool mismatch_ = false;
};

#endif  // DLT_MCP_SERVER_REPORT_HEADER_H_

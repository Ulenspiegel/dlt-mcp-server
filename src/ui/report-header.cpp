/* Copyright (C) 2026, Mikhail Nikonov <michael.n.nikonov@gmail.com>
 *
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License (MPL), v. 2.0.
 * If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ui/report-header.h"

#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLabel>
#include <QResizeEvent>
#include <QToolButton>

ReportHeaderWidget::ReportHeaderWidget(QWidget* parent)
    : QWidget(parent),
      toggleBtn_(new QToolButton(this)),
      titleLabel_(new QLabel("No report", this)) {
  toggleBtn_->setText("▶");
  toggleBtn_->setFixedSize(28, 24);
  toggleBtn_->setToolTip("Toggle report browser");

  titleLabel_->setObjectName("reportTitleLabel");
  originalTitle_ = titleLabel_->text();

  connect(toggleBtn_, &QToolButton::clicked, this,
          &ReportHeaderWidget::toggleClicked);

  auto* layout = new QHBoxLayout(this);
  layout->setContentsMargins(4, 2, 4, 2);
  layout->setSpacing(6);
  layout->addWidget(toggleBtn_);
  layout->addWidget(titleLabel_, 1);
}

void ReportHeaderWidget::setTitle(const QString& title) {
  originalTitle_ = title;
  applyTitleStyle();
}

void ReportHeaderWidget::setMismatch(bool mismatch) {
  mismatch_ = mismatch;
  applyTitleStyle();
}

void ReportHeaderWidget::applyTitleStyle() {
  QString display = originalTitle_;
  if (mismatch_) {
    display = QString("\u26a0 ") + originalTitle_;
    titleLabel_->setStyleSheet("color: red;");
    titleLabel_->setToolTip(
        "Report from different file(s). Links may not work.");
  } else {
    titleLabel_->setStyleSheet("");
    titleLabel_->setToolTip("");
  }
  titleLabel_->setText(display);
}

QString ReportHeaderWidget::title() const { return originalTitle_; }

void ReportHeaderWidget::setToggleArrow(const QString& arrow) {
  toggleBtn_->setText(arrow);
}

void ReportHeaderWidget::resizeEvent(QResizeEvent* event) {
  QWidget::resizeEvent(event);

  int availableWidth = width() - toggleBtn_->width() - 6;
  if (availableWidth <= 0) {
    titleLabel_->clear();
    return;
  }

  QString display = originalTitle_;
  if (mismatch_) {
    display = QString("\u26a0 ") + originalTitle_;
  }
  QFontMetrics fm(titleLabel_->font());
  QString elided = fm.elidedText(display, Qt::ElideMiddle, availableWidth);
  titleLabel_->setText(elided);
}

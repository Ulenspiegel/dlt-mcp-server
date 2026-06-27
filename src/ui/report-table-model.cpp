/* Copyright (C) 2026, Mikhail Nikonov <michael.n.nikonov@gmail.com>
 *
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License (MPL), v. 2.0.
 * If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ui/report-table-model.h"

#include <QApplication>
#include <QPalette>
#include <QString>
#include <chrono>

#include "report-storage.h"

ReportTableModel::ReportTableModel(QObject* parent)
    : QAbstractTableModel(parent) {}

void ReportTableModel::init(const std::shared_ptr<ReportStorage>& storage) {
  storage_ = storage;
}

void ReportTableModel::refresh(const ReportStorage::Filter& filter,
                               bool filterChecked) {
  if (!storage_) {
    return;
  }
  currentFilterHash_ = ReportStorage::computeFilterHash(filter);
  filterChecked_ = filterChecked;
  beginResetModel();
  if (filterChecked_) {
    reports_ = storage_->list(filter);
  } else {
    reports_ = storage_->list();
  }
  endResetModel();
}

const ReportStorage::Report& ReportTableModel::reportAt(int row) const {
  return reports_[row];
}

int ReportTableModel::rowCount(const QModelIndex& /*parent*/) const {
  return static_cast<int>(reports_.size());
}

int ReportTableModel::columnCount(const QModelIndex& /*parent*/) const {
  return ColumnCount;
}

QVariant ReportTableModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid()) {
    return QVariant();
  }
  const auto& report = reports_[index.row()];

  if (role == Qt::DisplayRole) {
    switch (static_cast<Column>(index.column())) {
      case ColTitle:
        return report.title.empty() ? "Untitled"
                                    : QString::fromStdString(report.title);
      case ColTime:
        return formatRelativeTime(report.timestamp);
      case ColumnCount:
        break;
    }
  } else if (role == Qt::ForegroundRole) {
    if (!filterChecked_ && report.hash != currentFilterHash_) {
      auto palette = QApplication::palette();
      return palette.color(QPalette::Disabled, QPalette::WindowText);
    }
  }

  return QVariant();
}

QVariant ReportTableModel::headerData(int section, Qt::Orientation orientation,
                                      int role) const {
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
    switch (static_cast<Column>(section)) {
      case ColTitle:
        return "Title";
      case ColTime:
        return "Time";
      case ColumnCount:
        break;
    }
  }
  return QVariant();
}

QString ReportTableModel::formatRelativeTime(int64_t timestampUs) {
  auto now = std::chrono::system_clock::now();
  auto then = std::chrono::time_point<std::chrono::system_clock,
                                      std::chrono::microseconds>(
      std::chrono::microseconds(timestampUs));
  auto secs =
      std::chrono::duration_cast<std::chrono::seconds>(now - then).count();
  if (secs < 60) {
    return QString("%1s ago").arg(secs);
  }
  if (secs < 3600) {
    return QString("%1m ago").arg(secs / 60);
  }
  if (secs < 86400) {
    return QString("%1h ago").arg(secs / 3600);
  }
  return QString("%1d ago").arg(secs / 86400);
}

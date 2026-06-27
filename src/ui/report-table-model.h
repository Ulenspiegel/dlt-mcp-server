/* Copyright (C) 2026, Mikhail Nikonov <michael.n.nikonov@gmail.com>
 *
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License (MPL), v. 2.0.
 * If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef DLT_MCP_SERVER_REPORT_TABLE_MODEL_H_
#define DLT_MCP_SERVER_REPORT_TABLE_MODEL_H_

#include <QAbstractTableModel>
#include <memory>
#include <string>
#include <vector>

#include "report-storage.h"

class ReportTableModel : public QAbstractTableModel {
  Q_OBJECT

 public:
  enum Column { ColTitle = 0, ColTime, ColumnCount };

  explicit ReportTableModel(QObject* parent = nullptr);

  void init(const std::shared_ptr<ReportStorage>& storage);
  void refresh(const ReportStorage::Filter& filter, bool filterChecked);
  const ReportStorage::Report& reportAt(int row) const;

  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  int columnCount(const QModelIndex& parent = QModelIndex()) const override;
  QVariant data(const QModelIndex& index,
                int role = Qt::DisplayRole) const override;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const override;

 private:
  static QString formatRelativeTime(int64_t timestampUs);

  std::shared_ptr<ReportStorage> storage_;
  std::vector<ReportStorage::Report> reports_;
  std::string currentFilterHash_;
  bool filterChecked_ = true;
};

#endif  // DLT_MCP_SERVER_REPORT_TABLE_MODEL_H_

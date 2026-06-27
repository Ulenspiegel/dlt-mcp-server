/* Copyright (C) 2026, Mikhail Nikonov <michael.n.nikonov@gmail.com>
 *
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License (MPL), v. 2.0.
 * If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef DLT_MCP_SERVER_REPORT_BROWSER_H_
#define DLT_MCP_SERVER_REPORT_BROWSER_H_

#include <QWidget>

class QCheckBox;
class QItemSelectionModel;
class QTableView;
class QPushButton;

#include "report-storage.h"

class ReportTableModel;

class ReportBrowserWidget : public QWidget {
  Q_OBJECT

 public:
  explicit ReportBrowserWidget(QWidget* parent = nullptr);

  ReportTableModel* model() const;
  int selectedRow() const;
  const ReportStorage::Report& reportAt(int row) const;
  bool isFilterChecked() const;
  void refreshTableModel(const ReportStorage::Filter& filter,
                         bool filterChecked);

 signals:
  void reportSelected(int row);
  void deleteSelected();
  void filterToggled(bool checked);

 public slots:
  void refresh();

 private:
  void onRowClicked(int row);

  QCheckBox* filterCheckbox_;
  QTableView* tableView_;
  QPushButton* deleteButton_;
};

#endif  // DLT_MCP_SERVER_REPORT_BROWSER_H_

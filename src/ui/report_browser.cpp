/* Copyright (C) 2026, Mikhail Nikonov <michael.n.nikonov@gmail.com>
 *
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License (MPL), v. 2.0.
 * If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ui/report_browser.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QTableView>
#include <QVBoxLayout>

#include "report_storage.h"
#include "ui/report_table_model.h"

ReportBrowserWidget::ReportBrowserWidget(QWidget* parent)
    : QWidget(parent),
      filterCheckbox_(new QCheckBox("Filter by loaded file(s)", this)),
      tableView_(new QTableView(this)),
      deleteButton_(new QPushButton("Delete", this)) {
  filterCheckbox_->setChecked(true);

  tableView_->setModel(new ReportTableModel(tableView_));
  tableView_->setSelectionBehavior(QAbstractItemView::SelectRows);
  tableView_->setSelectionMode(QAbstractItemView::SingleSelection);
  tableView_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  tableView_->setShowGrid(false);
  tableView_->verticalHeader()->hide();
  auto* header = tableView_->horizontalHeader();
  header->setSectionResizeMode(ReportTableModel::ColTitle,
                               QHeaderView::Stretch);
  header->setSectionResizeMode(ReportTableModel::ColTime,
                               QHeaderView::ResizeToContents);
  tableView_->setAlternatingRowColors(true);

  connect(filterCheckbox_, &QCheckBox::toggled, this,
          &ReportBrowserWidget::filterToggled);

  connect(tableView_, &QTableView::clicked, this,
          [this](const QModelIndex& idx) { onRowClicked(idx.row()); });

  connect(deleteButton_, &QPushButton::clicked, this,
          &ReportBrowserWidget::deleteSelected);

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(4, 4, 4, 4);
  layout->setSpacing(4);

  layout->addWidget(filterCheckbox_);
  layout->addWidget(tableView_, 1);

  auto* buttonLayout = new QHBoxLayout();
  buttonLayout->setContentsMargins(0, 0, 0, 0);
  buttonLayout->addWidget(deleteButton_);
  buttonLayout->addStretch();
  layout->addLayout(buttonLayout);
}

ReportTableModel* ReportBrowserWidget::model() const {
  return qobject_cast<ReportTableModel*>(tableView_->model());
}

int ReportBrowserWidget::selectedRow() const {
  auto indexes = tableView_->selectionModel()->selectedIndexes();
  if (indexes.isEmpty()) {
    return -1;
  }
  return indexes.first().row();
}

const ReportStorage::Report& ReportBrowserWidget::reportAt(int row) const {
  return model()->reportAt(row);
}

bool ReportBrowserWidget::isFilterChecked() const {
  return filterCheckbox_->isChecked();
}

void ReportBrowserWidget::refreshTableModel(const ReportStorage::Filter& filter,
                                            bool filterChecked) {
  model()->refresh(filter, filterChecked);
}

void ReportBrowserWidget::refresh() {
  refreshTableModel(ReportStorage::Filter{}, isFilterChecked());
}

void ReportBrowserWidget::onRowClicked(int row) { emit reportSelected(row); }

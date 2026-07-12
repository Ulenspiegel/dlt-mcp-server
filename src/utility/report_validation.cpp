/* Copyright (C) 2026, Mikhail Nikonov <michael.n.nikonov@gmail.com>
 *
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License (MPL), v. 2.0.
 * If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "utility/report_validation.h"

#include <md4c.h>

#include <QUrl>
#include <cstring>

namespace {

int noopBlockCallback(MD_BLOCKTYPE /*type*/, void* /*detail*/,
                      void* /*userdata*/) {
  return 0;
}

int noopSpanCallback(MD_SPANTYPE /*type*/, void* /*detail*/,
                     void* /*userdata*/) {
  return 0;
}

int noopTextCallback(MD_TEXTTYPE /*type*/, const MD_CHAR* /*text*/,
                     MD_SIZE /*size*/, void* /*userdata*/) {
  return 0;
}

int validateReport_enterSpan(MD_SPANTYPE type, void* detail, void* userdata) {
  if (type != MD_SPAN_A || !detail) {
    return 0;
  }

  auto* result = static_cast<ReportValidationResult*>(userdata);
  const auto* ad = static_cast<MD_SPAN_A_DETAIL*>(detail);

  const QString hrefStr =
      QString::fromUtf8(ad->href.text, static_cast<int>(ad->href.size));
  const QUrl url(hrefStr);

  if (url.scheme() != "jump") {
    return 0;
  }

  if (url.authority() != "msg") {
    result->malformed_links.emplace_back(hrefStr.toStdString());
    return 0;
  }

  const QString indexStr = url.path().mid(1);
  if (indexStr.isEmpty()) {
    result->malformed_links.emplace_back(hrefStr.toStdString());
    return 0;
  }

  bool ok = false;
  const int id = indexStr.toInt(&ok);

  if (!ok) {
    result->malformed_links.emplace_back(hrefStr.toStdString());
  } else if (id < 0 || id > result->max_msg_idx) {
    result->out_of_range_ids.push_back(id);
  } else {
    result->valid_links++;
  }
  return 0;
}

}  // namespace

ReportValidationResult validateReport(const std::string& report,
                                      int max_msg_idx) {
  ReportValidationResult result;
  result.max_msg_idx = max_msg_idx;

  MD_PARSER parser{};
  parser.flags = MD_DIALECT_GITHUB;
  parser.enter_block = noopBlockCallback;
  parser.leave_block = noopBlockCallback;
  parser.enter_span = validateReport_enterSpan;
  parser.leave_span = noopSpanCallback;
  parser.text = noopTextCallback;

  md_parse(report.c_str(), static_cast<MD_SIZE>(report.size()), &parser,
           &result);

  return result;
}

bool ReportValidationResult::hasErrors() const {
  return !malformed_links.empty() || !out_of_range_ids.empty();
}

std::string ReportValidationResult::formatMalformedLinks() const {
  std::string result;
  for (size_t i = 0; i < malformed_links.size(); ++i) {
    if (i > 0) result += ", ";
    result += malformed_links[i];
  }
  return result;
}

std::string ReportValidationResult::formatOutOfRangeIds() const {
  std::string result;
  for (size_t i = 0; i < out_of_range_ids.size(); ++i) {
    if (i > 0) result += ", ";
    result += std::to_string(out_of_range_ids[i]);
  }
  if (!out_of_range_ids.empty()) {
    result += " (min: 0, max: " + std::to_string(max_msg_idx) + ")";
  }
  return result;
}

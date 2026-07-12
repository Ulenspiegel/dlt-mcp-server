/* Copyright (C) 2026, Mikhail Nikonov <michael.n.nikonov@gmail.com>
 *
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License (MPL), v. 2.0.
 * If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef DLT_MCP_SERVER_UTILITY_REPORT_VALIDATION_H_
#define DLT_MCP_SERVER_UTILITY_REPORT_VALIDATION_H_

#include <string>
#include <vector>

struct ReportValidationResult {
  int valid_links = 0;
  std::vector<std::string> malformed_links;
  std::vector<int> out_of_range_ids;
  int max_msg_idx = 0;

  bool hasErrors() const;
  std::string formatMalformedLinks() const;
  std::string formatOutOfRangeIds() const;
};

ReportValidationResult validateReport(const std::string& report,
                                      int max_msg_idx);

#endif  // DLT_MCP_SERVER_UTILITY_REPORT_VALIDATION_H_

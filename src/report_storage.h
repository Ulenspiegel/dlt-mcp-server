/* Copyright (C) 2026, Mikhail Nikonov <michael.n.nikonov@gmail.com>
 *
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License (MPL), v. 2.0.
 * If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef DLT_MCP_SERVER_REPORT_STORAGE_H_
#define DLT_MCP_SERVER_REPORT_STORAGE_H_

#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace bmi = boost::multi_index;

class ReportStorage {
 public:
  struct Report {
    std::string hash;
    std::vector<std::string> file_paths;
    std::vector<int> per_file_counts;
    std::string title;
    int64_t timestamp;
    std::string content;
  };

  struct Filter {
    std::vector<std::string> file_paths;
    std::vector<int> per_file_counts;
  };

  static std::string computeFilterHash(const Filter& filter);

  explicit ReportStorage(const std::string& dir_path);

  void put(const std::vector<std::string>& file_paths,
           const std::vector<int>& per_file_counts, const std::string& title,
           const std::string& content);

  std::vector<Report> list(
      const std::optional<Filter>& filter = std::nullopt) const;
  void remove(const Report& report);

 private:
  using Reports = bmi::multi_index_container<
      Report, bmi::indexed_by<
                  bmi::ordered_unique<bmi::composite_key<
                      Report, bmi::member<Report, std::string, &Report::hash>,
                      bmi::member<Report, int64_t, &Report::timestamp>>>,
                  bmi::ordered_non_unique<
                      bmi::member<Report, int64_t, &Report::timestamp>>>>;

  void load();
  std::optional<Report> loadReport(const std::string& stem);
  void eraseEntry(const std::string& hash, int64_t timestamp);
  void trim(const std::string& hash_bucket);
  void rotate();

  Reports reports_;
  std::string dir_path_;

  static constexpr int kMaxEntries = 5;
  static constexpr int kMaxTotalEntries = 100;
  static constexpr int64_t kMaxAgeSeconds = 30 * 24 * 3600;
};

#endif  // DLT_MCP_SERVER_REPORT_STORAGE_H_

/* Copyright (C) 2026, Mikhail Nikonov <michael.n.nikonov@gmail.com>
 *
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License (MPL), v. 2.0.
 * If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef REPORT_CACHE_H_
#define REPORT_CACHE_H_

#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>
#include <chrono>
#include <json.hpp>
#include <optional>
#include <string>

namespace bmi = boost::multi_index;
namespace chrono = std::chrono;
namespace fs = std::filesystem;

struct Report {
  std::string key;
  chrono::system_clock::time_point timestamp;
  std::string content;
};

struct by_composite {};
struct by_timestamp {};

inline void to_json(nlohmann::json& j, const Report& r) {
  int64_t epoch = std::chrono::duration_cast<chrono::seconds>(
                      r.timestamp.time_since_epoch())
                      .count();
  j = {{"key", r.key}, {"timestamp", epoch}, {"content", r.content}};
}

inline void from_json(const nlohmann::json& j, Report& r) {
  r.key = j.at("key").get<std::string>();
  int64_t epoch = j.at("timestamp").get<int64_t>();
  r.timestamp = chrono::time_point_cast<chrono::seconds>(
      chrono::system_clock::from_time_t(epoch));
  r.content = j.value("content", std::string{});
}

class ReportCache {
  using Cache = bmi::multi_index_container<
      Report,
      bmi::indexed_by<
          bmi::ordered_unique<
              bmi::tag<by_composite>,
              bmi::composite_key<
                  Report, bmi::member<Report, std::string, &Report::key>,
                  bmi::member<Report, chrono::system_clock::time_point,
                              &Report::timestamp>>>,
          bmi::ordered_non_unique<
              bmi::tag<by_timestamp>,
              bmi::member<Report, chrono::system_clock::time_point,
                          &Report::timestamp>>>>;

 public:
  explicit ReportCache(const std::string& filePath);

  std::string get(const std::string& key);
  void put(const std::string& key, const std::string& markdown);

 private:
  static std::optional<std::string> loadFile(const std::string& path);
  static void saveFile(const std::string& path, const std::string& content);

  void load();
  void save();
  void rotate();
  void trim(const std::string& key);

  Cache cache_;
  std::string filePath_;

  static constexpr int kMaxEntries = 5;
  static constexpr int kMaxTotalEntries = 100;
  static constexpr auto kMaxAge = chrono::hours(30 * 24);
};

#endif  // REPORT_CACHE_H_

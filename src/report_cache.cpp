/* Copyright (C) 2026, Mikhail Nikonov <michael.n.nikonov@gmail.com>
 *
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License (MPL), v. 2.0.
 * If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "report_cache.h"

#include <filesystem>
#include <fstream>
#include <system_error>

using json = nlohmann::json;

ReportCache::ReportCache(const std::string& filePath) : filePath_(filePath) {
  load();
}

std::optional<std::string> ReportCache::loadFile(const std::string& path) {
  std::error_code ec;
  if (!fs::exists(path, ec) || ec) {
    return std::nullopt;
  }

  std::ifstream ifs(path);
  if (!ifs.is_open()) {
    return std::nullopt;
  }

  std::string content((std::istreambuf_iterator<char>(ifs)),
                      std::istreambuf_iterator<char>());
  if (!ifs) {
    return std::nullopt;
  }
  return content;
}

void ReportCache::saveFile(const std::string& path,
                           const std::string& content) {
  std::error_code ec;
  fs::create_directories(fs::path(path).parent_path(), ec);
  if (ec) {
    return;
  }

  std::ofstream ofs(path);
  if (!ofs.is_open()) {
    return;
  }
  ofs << content;
}

void ReportCache::load() {
  cache_.clear();

  auto content = loadFile(filePath_);
  if (!content) {
    return;
  }

  json data;
  try {
    data = json::parse(*content);
  } catch (...) {
    return;
  }

  for (auto& item : data) {
    try {
      cache_.insert(item.get<Report>());
    } catch (...) {
      // Skip malformed entries.
    }
  }
}

void ReportCache::save() {
  json output = json::array();
  for (auto& report : cache_) {
    output.push_back(report);
  }
  saveFile(filePath_, output.dump());
}

std::string ReportCache::get(const std::string& key) {
  auto& idx = cache_.get<by_composite>();
  auto [first, last] = idx.equal_range(std::make_tuple(key));
  if (first == last) {
    return {};
  }
  return std::prev(last)->content;
}

void ReportCache::put(const std::string& key, const std::string& markdown) {
  if (key.empty() || markdown.empty()) {
    return;
  }
  cache_.insert({key, chrono::system_clock::now(), markdown});
  trim(key);
  rotate();
  save();
}

void ReportCache::trim(const std::string& key) {
  auto& compIdx = cache_.get<by_composite>();
  auto [first, last] = compIdx.equal_range(std::make_tuple(key));
  if (const auto count = std::distance(first, last); count > kMaxEntries) {
    compIdx.erase(first, std::next(first, count - kMaxEntries));
  }
}

void ReportCache::rotate() {
  const auto now = chrono::system_clock::now();
  const auto cutoff = now - kMaxAge;
  auto& tsIdx = cache_.get<by_timestamp>();
  tsIdx.erase(tsIdx.begin(), tsIdx.lower_bound(cutoff));
  while (tsIdx.size() > kMaxTotalEntries) {
    tsIdx.erase(tsIdx.begin());
  }
}

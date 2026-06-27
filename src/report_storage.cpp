/* Copyright (C) 2026, Mikhail Nikonov <michael.n.nikonov@gmail.com>
 *
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License (MPL), v. 2.0.
 * If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "report_storage.h"

#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <json.hpp>

namespace fs = std::filesystem;
namespace spd = spdlog;
using json = nlohmann::json;

#include "utility/hash.h"

namespace {

std::string makeHash(const std::vector<std::string>& file_paths,
                     const std::vector<int>& per_file_counts) {
  int total_count = 0;
  std::string hash_input;
  for (size_t i = 0; i < file_paths.size(); ++i) {
    if (i > 0) {
      hash_input += '\0';
    }
    hash_input += fs::path(file_paths[i]).filename().string();
    total_count += per_file_counts[i];
  }
  hash_input += '\0';
  hash_input += std::to_string(total_count);
  return fmt::format("{:016x}", fnv1a64(hash_input));
}

std::string makeFileName(const std::string& hash, int64_t timestamp) {
  return fmt::format("{}_{:020x}", hash, timestamp);
}

struct BaseName {
  std::string hash;
  int64_t timestamp;
};

std::optional<BaseName> parseBaseName(const std::string& stem) {
  if (const auto pos = stem.find('_'); pos != 16) {
    return std::nullopt;
  }
  const std::string hash = stem.substr(0, 16);
  const std::string timestamp = stem.substr(17);
  if (timestamp.size() != 20) {
    return std::nullopt;
  }
  try {
    return BaseName{hash, std::stoll(timestamp, nullptr, 16)};
  } catch (...) {
    return std::nullopt;
  }
}

}  // namespace

std::string ReportStorage::computeFilterHash(const Filter& filter) {
  return makeHash(filter.file_paths, filter.per_file_counts);
}

void to_json(json& j, const ReportStorage::Report& r) {
  j = {{"file_paths", r.file_paths},
       {"per_file_counts", r.per_file_counts},
       {"title", r.title}};
}

void from_json(const json& j, ReportStorage::Report& r) {
  r.file_paths = j.at("file_paths").get<std::vector<std::string>>();
  r.per_file_counts = j.at("per_file_counts").get<std::vector<int>>();
  r.title = j.value("title", std::string{});

  std::vector<std::string> filenames;
  filenames.reserve(r.file_paths.size());
  for (const auto& path : r.file_paths) {
    filenames.emplace_back(fs::path(path).filename().string());
  }

  int64_t total = 0;
  for (const int count : r.per_file_counts) {
    total += count;
  }

  std::string hash_input;
  for (size_t i = 0; i < filenames.size(); ++i) {
    if (i > 0) hash_input += '\0';
    hash_input += filenames[i];
  }
  hash_input += '\0';
  hash_input += std::to_string(total);
  r.hash = fmt::format("{:016x}", fnv1a64(hash_input));
}

ReportStorage::ReportStorage(const std::string& dir_path)
    : dir_path_(dir_path) {
  load();
}

void ReportStorage::load() {
  std::error_code ec;
  if (!fs::exists(dir_path_, ec) && !ec) {
    fs::create_directories(dir_path_, ec);
    if (ec) {
      spd::warn("Failed to create reports storage directory: {}", ec.message());
      return;
    }
  }
  if (!fs::is_directory(dir_path_, ec) || ec) {
    spd::warn("Reports storage path is not a directory: {}", ec.message());
    return;
  }

  try {
    for (const auto& entry : fs::directory_iterator(dir_path_)) {
      if (!entry.is_regular_file() || entry.path().extension() != ".json") {
        continue;
      }
      if (auto report = loadReport(entry.path().stem().string())) {
        reports_.insert(std::move(*report));
      }
    }
  } catch (const fs::filesystem_error& e) {
    spd::warn("Failed to scan reports directory: {}", e.what());
  } catch (...) {
    spd::warn("Unexpected error scanning reports directory");
  }
}

std::optional<ReportStorage::Report> ReportStorage::loadReport(
    const std::string& stem) {
  auto parsed = parseBaseName(stem);
  if (!parsed) {
    return std::nullopt;
  }

  std::error_code ec;
  fs::path json_path = fs::path(dir_path_) / (stem + ".json");
  fs::path md_path = fs::path(dir_path_) / (stem + ".md");

  if (!fs::exists(md_path, ec) || ec) {
    fs::remove(json_path, ec);
    return std::nullopt;
  }

  auto cleanup = [&]() {
    fs::remove(json_path, ec);
    fs::remove(md_path, ec);
  };

  std::ifstream ifs(json_path);
  if (!ifs.is_open()) {
    cleanup();
    return std::nullopt;
  }

  json j;
  try {
    j = json::parse(ifs);
  } catch (...) {
    cleanup();
    return std::nullopt;
  }

  Report report;
  try {
    from_json(j, report);
  } catch (...) {
    cleanup();
    return std::nullopt;
  }

  std::ifstream cifs(md_path);
  if (!cifs.is_open()) {
    cleanup();
    return std::nullopt;
  }

  report.content = std::string((std::istreambuf_iterator<char>(cifs)),
                               std::istreambuf_iterator<char>());
  if (report.content.empty()) {
    cleanup();
    return std::nullopt;
  }

  report.timestamp = parsed->timestamp;
  return report;
}

void ReportStorage::put(const std::vector<std::string>& file_paths,
                        const std::vector<int>& per_file_counts,
                        const std::string& title, const std::string& content) {
  if (content.empty()) {
    return;
  }

  Report report;
  report.file_paths = file_paths;
  report.per_file_counts = per_file_counts;
  report.title = title;
  report.content = content;
  report.hash = makeHash(file_paths, per_file_counts);
  report.timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
                         std::chrono::system_clock::now().time_since_epoch())
                         .count();

  std::string stem = makeFileName(report.hash, report.timestamp);
  fs::path json_path = fs::path(dir_path_) / (stem + ".json");
  fs::path md_path = fs::path(dir_path_) / (stem + ".md");

  {
    std::ofstream ofs(json_path);
    if (!ofs.is_open()) {
      spd::warn("Failed to write report metadata");
      return;
    }
    ofs << json(report).dump();
  }

  {
    std::ofstream ofs(md_path);
    if (!ofs.is_open()) {
      std::error_code ec;
      fs::remove(json_path, ec);
      spd::warn("Failed to write report content");
      return;
    }
    ofs << content;
  }

  // TODO: Very unlikely collision chance here (timestamp is in microsceonds),
  // but for correctness, at some point could add a check.
  std::string saved_hash = report.hash;
  reports_.insert(std::move(report));

  trim(saved_hash);
  rotate();
}

std::vector<ReportStorage::Report> ReportStorage::list(
    const std::optional<Filter>& filter) const {
  std::vector<Report> result;

  if (filter) {
    std::string hash = makeHash(filter->file_paths, filter->per_file_counts);
    auto& idx = reports_.get<0>();
    auto [first, last] = idx.equal_range(hash);

    for (auto report = last; report != first;) {
      result.push_back(*--report);
    }
  } else {
    auto& byTime = reports_.get<1>();
    for (auto report = byTime.rbegin(); report != byTime.rend(); ++report) {
      result.push_back(*report);
    }
  }

  return result;
}

void ReportStorage::eraseEntry(const std::string& hash, int64_t timestamp) {
  std::error_code ec;
  const std::string stem = makeFileName(hash, timestamp);
  fs::remove(fs::path(dir_path_) / (stem + ".json"), ec);
  fs::remove(fs::path(dir_path_) / (stem + ".md"), ec);
  auto& idx = reports_.get<0>();
  if (auto it = idx.find(std::make_tuple(hash, timestamp)); it != idx.end()) {
    idx.erase(it);
  }
}

void ReportStorage::remove(const Report& report) {
  eraseEntry(report.hash, report.timestamp);
}

void ReportStorage::trim(const std::string& hash_bucket) {
  auto& idx = reports_.get<0>();
  auto [first, last] = idx.equal_range(hash_bucket);

  const ptrdiff_t count = std::distance(first, last);
  if (count <= kMaxEntries) {
    return;
  }

  ptrdiff_t excess = count - kMaxEntries;
  std::vector<std::pair<std::string, int64_t>> to_remove;
  for (auto it = first; it != last && excess > 0; ++it, --excess) {
    to_remove.emplace_back(it->hash, it->timestamp);
  }

  for (const auto& [hash, timestamp] : to_remove) {
    eraseEntry(hash, timestamp);
  }
}

void ReportStorage::rotate() {
  const auto now = std::chrono::duration_cast<std::chrono::microseconds>(
                       std::chrono::system_clock::now().time_since_epoch())
                       .count();
  const int64_t cutoff = now - kMaxAgeSeconds * 1'000'000;

  std::vector<std::pair<std::string, int64_t>> to_remove;
  auto& idx = reports_.get<0>();
  for (const auto& report : idx) {
    if (report.timestamp < cutoff) {
      to_remove.emplace_back(report.hash, report.timestamp);
    }
  }

  for (const auto& [hash, timestamp] : to_remove) {
    eraseEntry(hash, timestamp);
  }

  if (idx.empty()) {
    return;
  }

  while (static_cast<int>(idx.size()) > kMaxTotalEntries) {
    const auto it = idx.begin();
    eraseEntry(it->hash, it->timestamp);
  }
}

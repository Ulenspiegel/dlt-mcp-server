/* Copyright (C) 2026, Mikhail Nikonov <michael.n.nikonov@gmail.com>
 *
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License (MPL), v. 2.0.
 * If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef DLT_MCP_SERVER_UTILITY_DLT_ID_H_
#define DLT_MCP_SERVER_UTILITY_DLT_ID_H_

#include <cassert>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

// Maps DLT identifier strings to numeric indices.
class DltID {
 public:
  size_t insert(const std::string& id) {
    if (const auto it = map_.find(id); it != map_.end()) {
      return it->second;
    }
    const size_t index = ids_.size();
    map_[id] = index;
    ids_.push_back(id);
    return index;
  }

  std::optional<size_t> find(const std::string& id) const {
    if (const auto it = map_.find(id); it != map_.end()) {
      return it->second;
    }
    return std::nullopt;
  }

  const std::string& at(const size_t index) const {
    assert(index < ids_.size());
    return ids_[index];
  }

  void clear() {
    map_.clear();
    ids_.clear();
  }

 private:
  std::unordered_map<std::string, size_t> map_;
  std::vector<std::string> ids_;
};

#endif  // DLT_MCP_SERVER_UTILITY_DLT_ID_H_

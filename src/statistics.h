/* Copyright (C) 2026, Mikhail Nikonov <michael.n.nikonov@gmail.com>
 *
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License (MPL), v. 2.0.
 * If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef DLT_MCP_SERVER_STATISTICS_H_
#define DLT_MCP_SERVER_STATISTICS_H_

#include <qdltmsg.h>

#include <map>
#include <set>
#include <string>

class Statistics {
 public:
  struct Context {
    std::set<std::string> apids;
  };

  void reset() {
    contexts_.clear();
    log_levels_.clear();
  }

  void update(const QDltMsg& msg) {
    contexts_[msg.getCtid().toStdString()].apids.insert(
        msg.getApid().toStdString());
    log_levels_[msg.getSubtype()] = msg.getSubtypeString().toStdString();
  }

  const std::map<std::string, Context>& contexts() const { return contexts_; }

  const std::map<int, std::string>& log_levels() const { return log_levels_; }

 private:
  std::map<std::string, Context> contexts_;
  std::map<int, std::string> log_levels_;
};

#endif  // DLT_MCP_SERVER_STATISTICS_H_

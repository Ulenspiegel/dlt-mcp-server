/* Copyright (C) 2026, Mikhail Nikonov <michael.n.nikonov@gmail.com>
 *
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License (MPL), v. 2.0.
 * If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef DLT_MCP_SERVER_INDEX_H_
#define DLT_MCP_SERVER_INDEX_H_

#include <qdltmsg.h>

#include <cstdint>
#include <functional>
#include <optional>
#include <string>

class MessageSource {
 public:
  virtual ~MessageSource() = default;
  virtual std::optional<QDltMsg> get(int index) const = 0;
};

struct Message {
  int index;
  int64_t timestamp;  // ns since epoch
  int64_t ecu_time;   // ticks since boot
  std::string ctid;
  std::string apid;
  std::string ecuid;
  int log_level;
  std::string payload;
};

struct Query {
  std::optional<std::string> ctid;
  std::optional<std::string> apid;
  std::optional<int> min_level;
  std::optional<int> max_level;
  std::optional<int64_t> min_timestamp;
  std::optional<int64_t> max_timestamp;
  std::optional<std::string> keyword;
  bool case_insensitive = false;
};

class Index {
 public:
  virtual ~Index() = default;

  virtual void reset() = 0;

  virtual void add(int index, const QDltMsg& msg) = 0;
  virtual std::optional<Message> get(int index) = 0;

  virtual void search(const Query& params,
                      std::function<bool(const Message&)> callback) = 0;

  virtual int64_t firstTimestamp() const = 0;
  virtual int64_t lastTimestamp() const = 0;
};

#endif  // DLT_MCP_SERVER_INDEX_H_

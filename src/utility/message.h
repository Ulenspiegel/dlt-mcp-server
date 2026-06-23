/* Copyright (C) 2026, Mikhail Nikonov <michael.n.nikonov@gmail.com>
 *
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License (MPL), v. 2.0.
 * If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef DLT_MCP_SERVER_UTILITY_MESSAGE_H_
#define DLT_MCP_SERVER_UTILITY_MESSAGE_H_

#include <qdltmsg.h>

#include <cstdint>

// Extracts wall-clock timestamp in nanoseconds from a DLT message.
inline int64_t getWallClockNs(const QDltMsg& msg) {
  return static_cast<int64_t>(msg.getTime()) * 1'000'000'000LL +
         static_cast<int64_t>(msg.getMicroseconds()) * 1'000LL;
}

// Extracts ECU time in ticks (0.1ms units) from a DLT message.
inline int64_t getEcuTimeTicks(const QDltMsg& msg) {
  return static_cast<int64_t>(msg.getTimestamp());
}

#endif  // DLT_MCP_SERVER_UTILITY_MESSAGE_H_

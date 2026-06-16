/* Copyright (C) 2026, Mikhail Nikonov <michael.n.nikonov@gmail.com>
 *
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License (MPL), v. 2.0.
 * If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef DLT_MCP_SERVER_UTILITY_TIME_H_
#define DLT_MCP_SERVER_UTILITY_TIME_H_

#include <cstdint>
#include <tuple>

// Returns {hours, minutes, seconds, milliseconds} from nanosecond offset.
inline std::tuple<int64_t, int64_t, int64_t, int64_t> splitRelativeTime(
    int64_t offsetNs) {
  int64_t total_ms = offsetNs / 1'000'000;
  return {total_ms / 3600000, (total_ms % 3600000) / 60000,
          (total_ms % 60000) / 1000, total_ms % 1000};
}

// Returns {seconds, microseconds} from nanosecond offset.
inline std::tuple<int64_t, int64_t> splitRelativeTimestamp(int64_t offsetNs) {
  return {offsetNs / 1'000'000'000LL, (offsetNs % 1'000'000'000LL) / 1000LL};
}

// Returns {seconds, milliseconds} from ECU time ticks (0.1ms units).
inline std::tuple<int64_t, int64_t> splitEcuTime(int64_t ecuTimeTicks) {
  return {ecuTimeTicks / 10000, (ecuTimeTicks % 10000) / 10};
}

#endif  // DLT_MCP_SERVER_UTILITY_TIME_H_

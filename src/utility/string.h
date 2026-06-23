/* Copyright (C) 2026, Mikhail Nikonov <michael.n.nikonov@gmail.com>
 *
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License (MPL), v. 2.0.
 * If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef DLT_MCP_SERVER_UTILITY_STRING_H_
#define DLT_MCP_SERVER_UTILITY_STRING_H_

#include <string>

inline std::string cleanPayload(const std::string& payload) {
  std::string result;
  result.reserve(payload.size());
  for (const char c : payload) {
    if (c != '\0' && c != '\n' && c != '\r') {
      result += c;
    }
  }
  return result;
}

#endif  // DLT_MCP_SERVER_UTILITY_STRING_H_

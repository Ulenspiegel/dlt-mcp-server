/* Copyright (C) 2026, Mikhail Nikonov <michael.n.nikonov@gmail.com>
 *
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License (MPL), v. 2.0.
 * If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef DLT_MCP_SERVER_UTILITY_STRING_H_
#define DLT_MCP_SERVER_UTILITY_STRING_H_

#include <QString>
#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>

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

inline std::string payloadPreview(const std::string& payload,
                                  size_t maxLen = 100) {
  if (payload.size() <= maxLen) {
    return payload;
  }
  QString result =
      QString::fromUtf8(payload.data(), static_cast<int>(payload.size()));
  if (result.size() > static_cast<int>(maxLen)) {
    result.truncate(static_cast<int>(maxLen));
    return result.toUtf8().toStdString() + "~";
  }
  return result.toUtf8().toStdString();
}

inline bool isAscii(std::string_view text) {
  return std::all_of(text.begin(), text.end(),
                     [](unsigned char c) { return c < 0x80; });
}

inline std::string toLowerCase(std::string_view text) {
  if (isAscii(text)) {
    std::string result(text);
    for (char& c : result) {
      c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return result;
  }
  return QString::fromUtf8(text.data(), static_cast<int>(text.size()))
      .toLower()
      .toUtf8()
      .toStdString();
}

#endif  // DLT_MCP_SERVER_UTILITY_STRING_H_

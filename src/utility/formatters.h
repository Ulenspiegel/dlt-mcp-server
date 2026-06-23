/* Copyright (C) 2026, Mikhail Nikonov <michael.n.nikonov@gmail.com>
 *
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License (MPL), v. 2.0.
 * If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef DLT_MCP_SERVER_FORMATTERS_H_
#define DLT_MCP_SERVER_FORMATTERS_H_

#include <fmt/format.h>

#include "index.h"

template <>
struct fmt::formatter<Query> {
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  static std::string opt(const std::optional<std::string>& v) {
    return v.value_or("(none)");
  }

  template <typename T>
  static std::string opt(const std::optional<T>& v) {
    return v ? std::to_string(*v) : std::string("(none)");
  }

  auto format(const Query& q, format_context& ctx) const {
    return fmt::format_to(
        ctx.out(),
        "Query{{ctid={}, apid={}, keyword={}, case_insensitive={}, "
        "min_level={}, max_level={}, min_ts={}, max_ts={}}}",
        opt(q.ctid), opt(q.apid), opt(q.keyword), q.case_insensitive,
        opt(q.min_level), opt(q.max_level), opt(q.min_timestamp),
        opt(q.max_timestamp));
  }
};

#endif  // DLT_MCP_SERVER_FORMATTERS_H_

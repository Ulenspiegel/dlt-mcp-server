/* Copyright (C) 2026, Mikhail Nikonov <michael.n.nikonov@gmail.com>
 *
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License (MPL), v. 2.0.
 * If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef DLT_MCP_SERVER_CONFIG_H_
#define DLT_MCP_SERVER_CONFIG_H_

inline constexpr int DefaultPort = 9500;
inline constexpr const char* PortKey = "server_port";
inline constexpr const char* ContextFileKey = "context_file_path";
inline constexpr const char* BloomFilterSizeKey = "bloom_filter_size";
inline constexpr int DefaultBloomFilterSize = 0;

enum class ReportValidationSeverity : int {
  kIgnore = 0,
  kWarning = 1,
  kError = 2,
};

inline constexpr const char* LinksInCodeBlocksKey =
    "links_in_code_blocks_severity";
inline constexpr ReportValidationSeverity DefaultLinksInCodeBlocks =
    ReportValidationSeverity::kError;

inline constexpr const char* NoLinksKey = "no_links_severity";
inline constexpr ReportValidationSeverity DefaultNoLinks =
    ReportValidationSeverity::kWarning;

#endif  // DLT_MCP_SERVER_CONFIG_H_

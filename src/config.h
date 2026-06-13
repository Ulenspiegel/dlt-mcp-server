/* Copyright (C) 2026, Mikhail Nikonov <michael.n.nikonov@gmail.com>
 *
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License (MPL), v. 2.0.
 * If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <QLoggingCategory>

inline constexpr int DefaultPort = 8888;
inline constexpr const char* PortKey = "server_port";
inline constexpr const char* ContextFileKey = "context_file_path";

Q_DECLARE_LOGGING_CATEGORY(logDltMcpServer)

#endif // CONFIG_H

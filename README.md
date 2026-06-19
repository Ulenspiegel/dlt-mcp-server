# DLT MCP Server Plugin

[![Website](https://img.shields.io/badge/Website-dlt--mcp--server-blue)](https://ulenspiegel.github.io/dlt-mcp-server/) [![License: MPL 2.0](https://img.shields.io/badge/License-MPL_2.0-brightgreen.svg)](https://opensource.org/licenses/MPL-2.0)

A plugin for DLT Viewer that exposes DLT log analysis capabilities via the Model Context Protocol (MCP). It allows AI agents and MCP-compatible clients to query and retrieve information from DLT log files.

## Features

- **Log Summary** – Get message count, file size, timestamp range, duration, and available CTID/APID contexts.
- **Search** – Filter log messages by CTID, APID, log level, timestamp range, and keyword, with pagination.
- **Message Retrieval** – Fetch full payload for specific message IDs.
- **Selection** – Get details of the currently selected message in DLT Viewer.
- **Report** – Display Markdown reports in the plugin widget with clickable links that navigate DLT Viewer's log window to specific messages.

## Requirements

- DLT Viewer 2.30.0
- CMake 3.10+
- Boost 1.74+
- Qt 5.14+ / Qt 6 (matching DLT Viewer's build)

## Build

This plugin must be built as part of the DLT Viewer source tree. Clone both repositories, place this plugin inside the DLT Viewer plugin directory, and add it to the parent CMake build:

Clone DLT Viewer:

```bash
git clone https://github.com/COVESA/dlt-viewer.git
cd dlt-viewer
```

Clone this plugin into the plugin directory:

```bash
git clone https://github.com/Ulenspiegel/dlt-mcp-server.git plugin/dlt-mcp-server
```

Add the following line to `plugin/CMakeLists.txt`:

```cmake
add_subdirectory(dlt-mcp-server)
```

Then build:

```bash
cmake -B build -S .
cmake --build build --config Release
```

The plugin binary will be placed in `build/bin/plugins/`.

## License

Mozilla Public License 2.0 – see [LICENSE](LICENSE).

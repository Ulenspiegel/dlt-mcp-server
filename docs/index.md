---
layout: default
theme: jekyll-theme-cayman
title: DLT MCP Server Plugin
description: Bring AI-driven log analysis to the COVESA DLT Viewer via the Model Context Protocol.
---

# DLT MCP Server Plugin

[![License: MPL 2.0](https://img.shields.io/badge/License-MPL_2.0-brightgreen.svg)](https://opensource.org/licenses/MPL-2.0)
[![GitHub Repo](https://img.shields.io/badge/GitHub-Repository-blue.svg)](https://github.com/Ulenspiegel/dlt-mcp-server)

A plugin for the **COVESA DLT Viewer** that exposes Diagnostic Log and Trace (DLT) log data via the **Model Context Protocol (MCP)**, allowing AI agents and MCP-compatible clients to directly query, search, and retrieve information from your DLT log files.

[View Source on GitHub](https://github.com/Ulenspiegel/dlt-mcp-server) � [Report an Issue](https://github.com/Ulenspiegel/dlt-mcp-server/issues)

---

## See it in Action

![Plugin Demo: AI Agent querying DLT logs, opening a Markdown report widget, and clicking links to navigate the log window](assets/demo.gif)
*Workflow example: Requesting an analysis via an AI Agent (e.g., OpenCode), which generates a Markdown report directly inside the DLT Viewer widget. Clicking links in the report instantly navigates to the specific messages in the log window.*

---

## Features

* **Log Summary:** Instantly retrieve message counts, file sizes, timestamp ranges, log duration, and available CTIDs/APIDs.
* **Search:** Filter log messages by CTID, APID, log level, timestamp range, and specific keywords matched against payloads. Includes pagination support, control of case sensitivity and payload previews.
* **Message Retrieval:** Fetch the full, un-truncated payloads for specific message IDs.
* **Selection:** Allow the AI agent to read details of the currently selected message in your DLT Viewer window.
* **Context File:** Provide a context file with any instructions you'd like to provide to your AI agent.
* **Interactive Reporting:** Generate and display formatted Markdown reports in the plugin widget. Includes clickable links that navigate the main DLT Viewer log window to exact messages.
* **Persistence:** Most recently generated reports are persisted and automatically loaded alongside the matching DLT file.

---

## Requirements

To build and run this plugin, your environment must meet the following minimum requirements:

* **DLT Viewer:** 2.30.0+
* **CMake:** 3.10+
* **Boost:** 1.74+
* **Qt:** 5.14+ or Qt 6 *(Same as the version your DLT Viewer is built with.)*

---

## Build Instructions

This plugin is designed to be built as part of the DLT Viewer source tree.

**1. Clone the DLT Viewer repository:**
```bash
git clone https://github.com/COVESA/dlt-viewer.git
cd dlt-viewer
```

**2. Clone this plugin into the viewer's plugin directory:**
```bash
git clone https://github.com/Ulenspiegel/dlt-mcp-server.git plugin/dlt-mcp-server
```

**3. Update the CMake configuration:**

Add the following line to `plugin/CMakeLists.txt`:
```cmake
add_subdirectory(dlt-mcp-server)
```

**4. Build the project:**
```bash
cmake -B build -S .
cmake --build build --config Release
```

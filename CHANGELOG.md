# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- `search` tool warns when keyword contains regex-like characters and the result set is empty.
- Persistent disk cache for reports in JSON format (stored in Qt cache location).
- `set_report` MCP tool to display Markdown reports in the plugin widget.
- Report tab with clickable `[text](jump://msg/<id>)` links that jump DLT Viewer to specific messages.
- Plugin will now detect and display if the server cannot be started (e.g. port is busy).
- Support for multiple opened files and live logging cases.
- Info line on the dashboard indicating how many files are currently loaded.
- Status label reflects actual server state (Starting/Running/Failed).

### Requirements
- Minimum Qt version raised to 5.14 (needed for `QTextBrowser::setMarkdown()`).

### Documentation
- Minor formatting changes in README.md

## [0.1.0] - 2026-06-02

### Added
- Basic set of MCP tools: `get_log_summary`, `search`, `get_messages`, and `get_selection`.
- Dashboard widget with SSE and Streamable HTTP URL copy buttons and configurable port number and context file path.

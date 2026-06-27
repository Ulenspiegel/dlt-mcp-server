# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Block-based trigram index with Bloom filter for fast text search with (filter sizes: 8KB, 16KB, 32KB).
- `search` tool warns when keyword contains regex-like characters and the result set is empty.
- Unit test suite for Bloom filter.
- Persistent disk cache for reports in JSON format (stored in Qt cache location).
- `set_report` MCP tool to display Markdown reports in the plugin widget.
- Report tab with clickable `[text](jump://msg/<id>)` links that jump DLT Viewer to specific messages.
- Plugin will now detect and display if the server cannot be started (e.g. port is busy).
- Support for multiple opened files and live logging cases.
- Info line on the dashboard indicating how many files are currently loaded.
- Status label reflects actual server state (Starting/Running/Failed).
- Added `title` argument to `set_report` MCP tool.
- Introduced new on-disk report storage format, avoiding single file database and separating metadata from content.

### Changed
- Bloom filter block size reduced from 2000 to 500 messages to counter repetitive trigram matches.
- Filter size lineup: Small (8KB), Normal (16KB), Large (32KB) with updated memory estimates.
- Settings dialog redesigned with grouped layout, inline hints, and file existence validation.
- Replaced Qt logging with spdlog for better syntax and output format.
- Replaced dependency on system libfmt with FetchContent.

### Fixed
- NgramIndex constructor bug where filter_ was nullptr and index always reverted to full-text search.
- Report from a previously loaded log file is now cleared when loading a new one.

### Requirements
- Minimum Qt version raised to 5.14 (needed for `QTextBrowser::setMarkdown()`).

### Documentation
- Minor formatting changes in README.md

## [0.1.0] - 2026-06-02

### Added
- Basic set of MCP tools: `get_log_summary`, `search`, `get_messages`, and `get_selection`.
- Dashboard widget with SSE and Streamable HTTP URL copy buttons and configurable port number and context file path.

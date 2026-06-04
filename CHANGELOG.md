# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Plugin will now detect and display if the server cannot be started (e.g. port is busy).
- Support for multiple opened files and live logging cases.
- Info line on the dashboard indicating how many files are currently loaded.

### Documentation
- Minor formatting changes in README.md

## [0.1.0] - 2026-06-02

### Added
- Basic set of MCP tools: `get_log_summary`, `search`, `get_messages`, and `get_selection`.
- Dashboard widget with SSE and Streamable HTTP URL copy buttons and configurable port number and context file path.

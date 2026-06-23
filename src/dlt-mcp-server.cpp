/* Copyright (C) 2026, Mikhail Nikonov <michael.n.nikonov@gmail.com>
 *
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License (MPL), v. 2.0.
 * If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "dlt-mcp-server.h"

#include <mcp_message.h>
#include <spdlog/spdlog.h>

#include <QStandardPaths>
#include <cctype>
#include <climits>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "basic_index.h"
#include "settings-dialog.h"
#include "statistics.h"
#include "utility/message.h"
#include "utility/string.h"
#include "utility/time.h"

namespace spd = spdlog;

DltMcpServer::DltMcpServer() {
  settings_ =
      std::make_unique<QSettings>("MyCompany", "DLTViewer_DltMcpServer");
  auto cacheDir =
      QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
  std::filesystem::path cachePath(cacheDir.toStdString());
  cachePath /= "dlt-mcp-server/reports.json";
  reportCache_ = std::make_unique<ReportCache>(cachePath.string());
  index_ = std::make_unique<BasicIndex>(*this);
  initMcpServer();
  registerMcpTools();
  server_->start(false);
}

DltMcpServer::~DltMcpServer() { server_->stop(); }

QString DltMcpServer::name() { return plugin_name_displayed; }

QString DltMcpServer::pluginVersion() { return DLT_VIEWER_PLUGIN_VERSION; }

QString DltMcpServer::pluginInterfaceVersion() {
  return PLUGIN_INTERFACE_VERSION;
}

QString DltMcpServer::description() {
  return "MCP Server for DLT log analysis";
}

QString DltMcpServer::error() { return {}; }

bool DltMcpServer::loadConfig(QString /*filename*/) { return true; }

bool DltMcpServer::saveConfig(QString /*filename*/) { return true; }

QStringList DltMcpServer::infoConfig() { return {}; }

void DltMcpServer::jumpToMessage(int index) {
  if (control_ && index >= 0) {
    control_->jumpToMsg(index);
  }
}

bool DltMcpServer::initControl(QDltControl* control) {
  control_ = control;
  return true;
}

bool DltMcpServer::initConnections(QStringList /*list*/) { return true; }

bool DltMcpServer::controlMsg(int /*index*/, QDltMsg& /*msg*/) { return true; }

bool DltMcpServer::stateChanged(
    int /*index*/, QDltConnection::QDltConnectionState connectionState,
    QString /*hostname*/) {
  if (connectionState == QDltConnection::QDltConnectionOffline) {
    is_live_ = false;
  }
  return true;
}

bool DltMcpServer::autoscrollStateChanged(bool /*enabled*/) { return true; }

void DltMcpServer::initMessageDecoder(QDltMessageDecoder* /*messageDecoder*/) {}

void DltMcpServer::initMainTableView(QTableView* /*pTableView*/) {}

void DltMcpServer::configurationChanged() {}

QWidget* DltMcpServer::initViewer() {
  dashboard_ = new Dashboard(settings_.get(), this);
  connect(dashboard_, &Dashboard::openSettings, this, [this]() {
    SettingsDialog dialog(settings_.get(), nullptr);
    dialog.exec();
  });
  return dashboard_;
}

void DltMcpServer::selectedIdxMsgDecoded(int index, QDltMsg& /*msg*/) {
  selected_index_ = index;
}

void DltMcpServer::selectedIdxMsg(int /*index*/, QDltMsg& /*msg*/) {}

void DltMcpServer::initFileStart(QDltFile* file) {
  reset();
  dlt_file_ = file;
  is_live_ = false;
  if (dashboard_) {
    QMetaObject::invokeMethod(dashboard_, &Dashboard::clearReport,
                              Qt::QueuedConnection);
  }
}

void DltMcpServer::initMsg(int /*index*/, QDltMsg& /*msg*/) {}

void DltMcpServer::initMsgDecoded(int index, QDltMsg& msg) {
  onMessageReceived(index, msg);
}

void DltMcpServer::initFileFinish() {
  computeFileRanges();
  if (dlt_file_ && dlt_file_->size() > 0) {
    emit fileCountChanged(file_ranges_.size());
    auto key = buildReportKey();
    if (!key.empty() && dashboard_) {
      std::string cached = reportCache_->get(key);
      if (!cached.empty()) {
        QMetaObject::invokeMethod(
            dashboard_,
            [dash = dashboard_, cached]() { dash->setReport(cached); },
            Qt::QueuedConnection);
      }
    }
  } else {
    emit fileCountChanged(0);
  }
}

void DltMcpServer::updateFileStart() { is_live_ = true; }

void DltMcpServer::updateMsg(int /*index*/, QDltMsg& /*msg*/) {}

void DltMcpServer::updateMsgDecoded(int index, QDltMsg& msg) {
  onMessageReceived(index, msg);
}

void DltMcpServer::updateFileFinish() { liveUpdateLastFileMessageCount(); }

bool DltMcpServer::isMsg(QDltMsg& /*msg*/, int /*triggeredByUser*/) {
  return false;
}

bool DltMcpServer::decodeMsg(QDltMsg& /*msg*/, int /*triggeredByUser*/) {
  return false;
}

void DltMcpServer::reset() {
  selected_index_ = -1;
  index_->reset();
  statistics_.reset();
  file_ranges_.clear();
}

std::string DltMcpServer::buildReportKey() {
  if (file_ranges_.empty()) {
    return {};
  }

  std::string names;
  int totalCount = 0;
  for (const auto& info : file_ranges_) {
    if (!names.empty()) {
      names += "+";
    }
    names += std::filesystem::path(info.name).filename().string();
    totalCount += info.message_count;
  }
  names += ":" + std::to_string(totalCount);
  return names;
}

void DltMcpServer::onMessageReceived(int index, const QDltMsg& msg) {
  if (msg.getType() != QDltMsg::DltTypeLog) {
    return;
  }
  index_->add(index, msg);
  statistics_.update(msg);
}

std::optional<QDltMsg> DltMcpServer::get(int index) const {
  if (!dlt_file_) {
    return std::nullopt;
  }
  QDltMsg msg;
  if (dlt_file_->getMsg(index, msg)) {
    return msg;
  }
  return std::nullopt;
}

char DltMcpServer::getLevelChar(int level) {
  auto it = statistics_.log_levels().find(level);
  if (it != statistics_.log_levels().end()) {
    return static_cast<char>(
        std::toupper(static_cast<unsigned char>(it->second[0])));
  }
  return '?';
}

void DltMcpServer::computeFileRanges() {
  file_ranges_.clear();
  if (!dlt_file_) {
    return;
  }
  int start = 0;
  for (int i = 0; i < dlt_file_->getNumberOfFiles(); i++) {
    const int count = dlt_file_->getFileMsgNumber(i);
    if (count > 0) {
      file_ranges_.push_back(
          {dlt_file_->getFileName(i).toStdString(), count, start});
    }
    start += count;
  }
}

void DltMcpServer::liveUpdateLastFileMessageCount() {
  if (!dlt_file_ || file_ranges_.empty()) {
    return;
  }
  int num_files = dlt_file_->getNumberOfFiles();
  if (num_files > 0) {
    file_ranges_.back().message_count =
        dlt_file_->getFileMsgNumber(num_files - 1);
  }
}

std::string DltMcpServer::formatMessageLine(
    int64_t hours, int64_t minutes, int64_t seconds, int64_t millis,
    char levelChar, const std::string& ctid, const std::string& apid,
    int msgIndex, int64_t relSec, int64_t relUsec, int64_t ecuSec,
    int64_t ecuMmm) {
  return fmt::format(
      "+{:02}:{:02}:{:02}.{:03} {} {}/{} #{} @{}.{:06} T{}.{:03} ", hours,
      minutes, seconds, millis, levelChar, ctid, apid, msgIndex, relSec,
      relUsec, ecuSec, ecuMmm);
}

bool DltMcpServer::looksLikeRegex(const std::string& s) {
  static constexpr char kMetaChars[] = "*+?^$[]";
  for (const char c : s) {
    if (strchr(kMetaChars, c)) return true;
  }
  for (size_t i = 0; i + 1 < s.size(); ++i) {
    if (s[i] == '\\' && strchr("dwWsSb", s[i + 1])) return true;
  }
  return false;
}

mcp::json DltMcpServer::makeTextResult(const std::string& text,
                                       const std::string& warning) {
  if (warning.empty()) {
    return {{{"type", "text"}, {"text", text}}};
  }
  return {{{"type", "text"}, {"text", text}},
          {{"type", "text"}, {"text", "[WARNING] " + warning}}};
}

void DltMcpServer::initMcpServer() {
  mcp::server::configuration config{};
  config.host = "localhost";
  config.port = settings_->value(PortKey, DefaultPort).toInt();
  server_ = std::make_unique<mcp::server>(config);
  server_->set_server_info("DLT MCP Server", DLT_VIEWER_PLUGIN_VERSION);
  mcp::json capabilities = {{"tools", mcp::json::object()}};
  server_->set_capabilities(capabilities);
}

void DltMcpServer::registerMcpTools() {
  mcp::tool summary_tool =
      mcp::tool_builder("get_log_summary")
          .with_description(
              "Get statistics and summary info of the loaded DLT log, such as "
              "range of timestamps and list of available CTID/APID pairs")
          .build();
  server_->register_tool(summary_tool,
                         [this](const auto& params, const auto& session_id) {
                           return get_log_summary(params, session_id);
                         });
  mcp::tool search_tool =
      mcp::tool_builder("search")
          .with_description(
              "Retrieve previews of log lines, optionally filtered by CTID, "
              "APID, log level(s), timestamp range, keyword and paginated "
              "by limit/offset. Log lines include relative time offset "
              "(+HH:MM:SS.mmm), log level, CTID/APID, message ID (#), "
              "relative timestamp in seconds from log start (@), and ECU "
              "uptime in seconds (T). The value after @ can be reused "
              "directly as min_timestamp or max_timestamp parameter "
              "(float64, seconds from start of log). If payload exceeds "
              "100 characters, the log line is a preview and ends with "
              "symbol ~. Use count=true to get only the number of "
              "matching lines. Use case_insensitive=true for "
              "case-insensitive keyword matching.")
          .with_string_param("ctid", "CTID to restrict the output to", false)
          .with_string_param("apid", "APID to restrict the output to", false)
          .with_number_param("min_log_level",
                             "Minimal log level to include into output", false)
          .with_number_param("max_log_level",
                             "Maximal log level to include into output", false)
          .with_number_param("min_timestamp",
                             "Starting timestamp in seconds from log start",
                             false)
          .with_number_param("max_timestamp",
                             "Ending timestamp in seconds from log start",
                             false)
          .with_string_param("keyword",
                             "Substring to search in the message payload "
                             "(regular expressions or "
                             "wildcards are not supported)",
                             false)
          .with_boolean_param("case_insensitive",
                              "When true, perform case-insensitive keyword "
                              "matching. Default: false.",
                              false)
          .with_boolean_param("count",
                              "When true, return only the total number of "
                              "matching lines instead "
                              "of the lines themselves. Ignores limit/offset.",
                              false)
          .with_number_param(
              "limit", "Limit output to this many log lines, hard limit is 100",
              false)
          .with_number_param(
              "offset",
              "Offset of the result from the start of selected log range",
              false)
          .build();
  server_->register_tool(search_tool,
                         [this](const auto& params, const auto& session_id) {
                           return search(params, session_id);
                         });
  mcp::tool retrieve_tool =
      mcp::tool_builder("get_messages")
          .with_description(
              "Retrieve a list of log messages with given IDs (#), including "
              "full payload. The limit is 20 messages per tool call.")
          .with_array_param("message_ids",
                            "IDs of the log messages to retrieve", "number")
          .build();
  server_->register_tool(retrieve_tool,
                         [this](const auto& params, const auto& session_id) {
                           return get_messages(params, session_id);
                         });
  mcp::tool selection_tool = mcp::tool_builder("get_selection")
                                 .with_description(
                                     "Get details of the currently selected "
                                     "message in the DLT Viewer. "
                                     "Returns metadata including index, "
                                     "CTID/APID, log level, timestamps, "
                                     "and a payload preview. Returns an error "
                                     "if no message is selected.")
                                 .build();
  server_->register_tool(selection_tool,
                         [this](const auto& params, const auto& session_id) {
                           return get_selection(params, session_id);
                         });
  // TODO: Report history — store multiple reports, navigation UI.
  //   Decision needed: labeling (agent-provided heading vs timestamp),
  //   storage (memory vs disk), capacity, navigation controls.
  mcp::tool report_tool =
      mcp::tool_builder("set_report")
          .with_description(
              "Set the Markdown report content in the plugin widget. "
              "Use [text](jump://msg/<id>) links where <id> is the message ID "
              "(#) from other tools, to create clickable message references. "
              "Pass empty string to clear the report. "
              "Only generate reports when you have a conclusive analysis "
              "result — do not overwrite existing reports during "
              "intermediate exploration. "
              "If unsure whether the analysis is complete, ask the user first.")
          .with_string_param(
              "markdown", "Markdown-formatted report content to display", true)
          .build();
  server_->register_tool(report_tool,
                         [this](const auto& params, const auto& session_id) {
                           return set_report(params, session_id);
                         });
}

mcp::json DltMcpServer::get_log_summary(const mcp::json& /*params*/,
                                        const std::string& /*session_id*/) {
  if (!dlt_file_) {
    throw mcp::mcp_exception(mcp::error_code::internal_error,
                             "DLT file is not loaded");
  }
  if (dlt_file_->size() == 0) {
    throw mcp::mcp_exception(mcp::error_code::internal_error,
                             "DLT file is empty");
  }
  int64_t first_ts = index_->firstTimestamp();
  int64_t last_ts = index_->lastTimestamp();
  double duration_sec = (last_ts - first_ts) / 1'000'000'000.0;

  // Wall-clock start time for context.
  QDltMsg first_msg;
  dlt_file_->getMsg(0, first_msg);
  std::string log_start = fmt::format(
      "{}.{:06}", first_msg.getGmTimeWithOffsetString(0, false).toStdString(),
      first_msg.getMicroseconds());

  mcp::json log_levels = mcp::json::array();
  for (const auto& level : statistics_.log_levels()) {
    log_levels.push_back({{level.second, level.first}});
  }
  mcp::json summary = {{"message_count", dlt_file_->size()},
                       {"total_file_size", dlt_file_->fileSize()},
                       {"log_start", log_start},
                       {"duration_seconds", duration_sec},
                       {"min_timestamp", 0.0},
                       {"max_timestamp", duration_sec},
                       {"log_level_names", log_levels},
                       {"contexts", mcp::json::object()}};
  auto& ctx_object = summary["contexts"];
  for (const auto& [ctid, ctx] : statistics_.contexts()) {
    auto apid_array = mcp::json::array();
    for (const auto& apid : ctx.apids) {
      apid_array.push_back(apid);
    }
    ctx_object[ctid] = apid_array;
  }
  std::string context = loadContextFile();
  if (!context.empty()) {
    summary["context_info"] = context;
  }

  mcp::json files_array = mcp::json::array();
  for (const auto& info : file_ranges_) {
    files_array.push_back({{"name", info.name},
                           {"message_count", info.message_count},
                           {"start_index", info.start_index}});
  }
  summary["files"] = files_array;

  return makeTextResult(summary.dump());
}

std::string DltMcpServer::loadContextFile() {
  QString filePath = settings_->value(ContextFileKey).toString();
  if (filePath.isEmpty()) {
    return {};
  }

  auto path = filePath.toStdString();
  if (path != context_file_path_) {
    context_file_path_ = path;
    context_file_content_.clear();
  }

  if (!context_file_content_.empty()) {
    return context_file_content_;
  }

  std::error_code ec;
  auto stat = std::filesystem::status(path, ec);
  if (ec || stat.type() != std::filesystem::file_type::regular) {
    return {};
  }

  std::ifstream file(path);
  if (!file.is_open()) {
    return {};
  }

  context_file_content_ = std::string(std::istreambuf_iterator<char>(file),
                                      std::istreambuf_iterator<char>());
  return context_file_content_;
}

mcp::json DltMcpServer::search(const mcp::json& params,
                               const std::string& /*session_id*/) {
  if (!dlt_file_) {
    throw mcp::mcp_exception(mcp::error_code::internal_error,
                             "DLT file is not loaded");
  }
  if (dlt_file_->size() == 0) {
    return makeTextResult("");
  }

  // Extract tool parameters.
  std::string ctid;
  bool has_ctid = false;
  if (params.contains("ctid")) {
    ctid = params["ctid"].get<std::string>();
    has_ctid = true;
  }

  std::string apid;
  bool has_apid = false;
  if (params.contains("apid")) {
    apid = params["apid"].get<std::string>();
    has_apid = true;
  }

  bool has_min_level = params.contains("min_log_level");
  bool has_max_level = params.contains("max_log_level");
  int min_log_level = has_min_level ? params["min_log_level"].get<int>() : 0;
  int max_log_level = has_max_level ? params["max_log_level"].get<int>() : 0;

  // Validate log level range: must be in DLT range (0-15) and min <= max.
  if (has_min_level && (min_log_level < 0 || min_log_level > 15)) {
    throw mcp::mcp_exception(
        mcp::error_code::invalid_params,
        "min_log_level must be 0-15, got: " + std::to_string(min_log_level));
  }
  if (has_max_level && (max_log_level < 0 || max_log_level > 15)) {
    throw mcp::mcp_exception(
        mcp::error_code::invalid_params,
        "max_log_level must be 0-15, got: " + std::to_string(max_log_level));
  }
  if (has_min_level && has_max_level && min_log_level > max_log_level) {
    throw mcp::mcp_exception(
        mcp::error_code::invalid_params,
        fmt::format("min_log_level ({}) > max_log_level ({})", min_log_level,
                    max_log_level));
  }

  bool has_min_ts = params.contains("min_timestamp");
  bool has_max_ts = params.contains("max_timestamp");
  double min_timestamp_sec =
      has_min_ts ? params["min_timestamp"].get<double>() : 0;
  double max_timestamp_sec =
      has_max_ts ? params["max_timestamp"].get<double>() : 0;

  if (has_min_ts && has_max_ts && min_timestamp_sec > max_timestamp_sec) {
    throw mcp::mcp_exception(
        mcp::error_code::invalid_params,
        fmt::format("min_timestamp ({}) > max_timestamp ({})",
                    min_timestamp_sec, max_timestamp_sec));
  }

  bool has_keyword = params.contains("keyword");
  std::string keyword =
      has_keyword ? params["keyword"].get<std::string>() : std::string();

  bool case_insensitive = params.contains("case_insensitive")
                              ? params["case_insensitive"].get<bool>()
                              : false;

  bool count_only =
      params.contains("count") ? params["count"].get<bool>() : false;

  int limit = params.contains("limit") ? params["limit"].get<int>() : 20;
  if (limit > 100) {
    limit = 100;
  }
  if (limit < 1) {
    limit = 1;
  }

  int offset = params.contains("offset") ? params["offset"].get<int>() : 0;
  if (offset < 0) {
    offset = 0;
  }

  Query query;
  if (has_ctid) {
    query.ctid = ctid;
  }
  if (has_apid) {
    query.apid = apid;
  }
  if (has_min_level) {
    query.min_level = min_log_level;
  }
  if (has_max_level) {
    query.max_level = max_log_level;
  }
  if (has_min_ts) {
    int64_t first_ts = index_->firstTimestamp();
    query.min_timestamp =
        first_ts + static_cast<int64_t>(min_timestamp_sec * 1'000'000'000.0);
  }
  if (has_max_ts) {
    int64_t first_ts = index_->firstTimestamp();
    query.max_timestamp =
        first_ts + static_cast<int64_t>(max_timestamp_sec * 1'000'000'000.0);
  }
  if (has_keyword) {
    query.keyword = keyword;
    query.case_insensitive = case_insensitive;
  }

  // Search and format/count in a single pass.
  int matches_count = 0;
  int matches_skipped = 0;
  int matches_collected = 0;
  std::string regex_warning;
  std::ostringstream oss;
  int64_t first_ts = index_->firstTimestamp();

  index_->search(query, [&](const Message& m) {
    if (matches_skipped < offset) {
      matches_skipped++;
      return true;
    }
    if (!count_only && matches_collected >= limit) {
      return false;  // stop iteration
    }

    if (count_only) {
      matches_count++;
    } else {
      if (matches_collected > 0) {
        oss << "\n";
      }
      int64_t offset_ns = m.timestamp - first_ts;
      auto [hours, minutes, seconds, millis] = splitRelativeTime(offset_ns);
      auto [rel_sec, rel_usec] = splitRelativeTimestamp(offset_ns);
      auto [ecu_sec, ecu_mmm] = splitEcuTime(m.ecu_time);

      oss << formatMessageLine(hours, minutes, seconds, millis,
                               getLevelChar(m.log_level), m.ctid, m.apid,
                               m.index, rel_sec, rel_usec, ecu_sec, ecu_mmm);

      std::string cleaned = cleanPayload(m.payload);
      if (static_cast<int>(cleaned.size()) > 100) {
        oss << cleaned.substr(0, 100) << "~";
      } else {
        oss << cleaned;
      }
      matches_collected++;
    }
    return true;
  });

  int total_count =
      count_only ? matches_count : matches_collected + matches_skipped;
  if (has_keyword && total_count == 0 && looksLikeRegex(keyword)) {
    regex_warning =
        "Keyword contains regex-like characters but only literal "
        "substring matching is supported";
  }

  if (count_only) {
    return makeTextResult(std::to_string(matches_count), regex_warning);
  }

  return makeTextResult(oss.str(), regex_warning);
}

mcp::json DltMcpServer::get_messages(const mcp::json& params,
                                     const std::string& /*session_id*/) {
  if (!dlt_file_) {
    throw mcp::mcp_exception(mcp::error_code::internal_error,
                             "DLT file is not loaded");
  }

  if (!params.contains("message_ids") || !params["message_ids"].is_array()) {
    throw mcp::mcp_exception(
        mcp::error_code::invalid_params,
        "message_ids parameter is required and must be an array");
  }

  auto ids = params["message_ids"];
  if (ids.empty()) {
    return makeTextResult("");
  }

  // Enforce hard limit.
  if (ids.size() > 20) {
    ids.erase(ids.begin() + 20, ids.end());
  }

  // Get base timestamp for relative time.
  int64_t base_ts = index_->firstTimestamp();

  std::ostringstream oss;
  size_t count = 0;

  for (const auto& id_json : ids) {
    int msg_index = id_json.get<int>();

    QDltMsg msg;
    if (!dlt_file_->getMsg(msg_index, msg)) {
      continue;
    }

    // Extract metadata from message.
    std::string ctid_str = msg.getCtid().toStdString();
    std::string apid_str = msg.getApid().toStdString();
    int level = msg.getSubtype();
    int64_t timestamp = getWallClockNs(msg);
    int64_t ecu_time = getEcuTimeTicks(msg);

    // Payload with cleanup.
    std::string payload = msg.toStringPayload().toStdString();

    int64_t offset_ns = timestamp - base_ts;
    auto [hours, minutes, seconds, millis] = splitRelativeTime(offset_ns);
    auto [rel_sec, rel_usec] = splitRelativeTimestamp(offset_ns);
    auto [ecu_sec, ecu_mmm] = splitEcuTime(ecu_time);

    oss << formatMessageLine(hours, minutes, seconds, millis,
                             getLevelChar(level), ctid_str, apid_str, msg_index,
                             rel_sec, rel_usec, ecu_sec, ecu_mmm);

    // Full payload (no truncation).
    oss << cleanPayload(payload);

    count++;
    if (count < ids.size()) {
      oss << "\n";
    }
  }

  return makeTextResult(oss.str());
}

mcp::json DltMcpServer::get_selection(const mcp::json& /*params*/,
                                      const std::string& /*session_id*/) {
  if (selected_index_ < 0) {
    throw mcp::mcp_exception(
        mcp::error_code::invalid_params,
        "No message is currently selected in the DLT Viewer");
  }

  if (!dlt_file_) {
    throw mcp::mcp_exception(mcp::error_code::internal_error,
                             "DLT file is not loaded");
  }

  QDltMsg msg;
  if (!dlt_file_->getMsg(selected_index_, msg)) {
    throw mcp::mcp_exception(mcp::error_code::internal_error,
                             "Failed to retrieve selected message");
  }

  int64_t timestamp = getWallClockNs(msg);
  int64_t ecu_time = getEcuTimeTicks(msg);

  int64_t base_ts = index_->firstTimestamp();
  int64_t offset_ns = timestamp - base_ts;
  auto [hours, minutes, seconds, millis] = splitRelativeTime(offset_ns);
  auto [rel_sec, rel_usec] = splitRelativeTimestamp(offset_ns);
  auto [ecu_sec, ecu_mmm] = splitEcuTime(ecu_time);

  // Payload with cleanup, no truncation.
  std::string payload = msg.toStringPayload().toStdString();

  std::ostringstream oss;
  oss << formatMessageLine(
      hours, minutes, seconds, millis, getLevelChar(msg.getSubtype()),
      msg.getCtid().toStdString(), msg.getApid().toStdString(), selected_index_,
      rel_sec, rel_usec, ecu_sec, ecu_mmm);

  // Full payload (no truncation).
  oss << cleanPayload(payload);

  return makeTextResult(oss.str());
}

mcp::json DltMcpServer::set_report(const mcp::json& params,
                                   const std::string& /*session_id*/) {
  if (!dashboard_) {
    throw mcp::mcp_exception(mcp::error_code::internal_error,
                             "Dashboard not initialized");
  }
  if (!params.contains("markdown") || !params["markdown"].is_string()) {
    throw mcp::mcp_exception(mcp::error_code::invalid_params,
                             "markdown parameter is required");
  }
  std::string markdown = params["markdown"].get<std::string>();
  if (!markdown.empty() && !is_live_) {
    auto key = buildReportKey();
    if (!key.empty()) {
      reportCache_->put(key, markdown);
    }
  }
  QMetaObject::invokeMethod(
      dashboard_,
      [dash = dashboard_, markdown]() {
        if (markdown.empty()) {
          dash->clearReport();
        } else {
          dash->setReport(markdown);
        }
      },
      Qt::QueuedConnection);
  return makeTextResult("");
}

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
// clang-format off
Q_EXPORT_PLUGIN2(dlt-mcp-server, DltMcpServerPlugin);
// clang-format on
#endif

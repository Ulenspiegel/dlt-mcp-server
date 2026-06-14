/* Copyright (C) 2026, Mikhail Nikonov <michael.n.nikonov@gmail.com>
 *
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License (MPL), v. 2.0.
 * If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "dlt-mcp-server.h"

#include <mcp_message.h>

#include "settings-dialog.h"

Q_LOGGING_CATEGORY(logDltMcpServer, "dlt.mcp.server", QtDebugMsg)

#include <fmt/format.h>

#include <cctype>
#include <climits>
#include <filesystem>
#include <fstream>
#include <sstream>

DltMcpServer::DltMcpServer() {
  settings_ =
      std::make_unique<QSettings>("MyCompany", "DLTViewer_DltMcpServer");
  dashboard_ = nullptr;
  dlt_file_ = nullptr;
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
    int /*index*/, QDltConnection::QDltConnectionState /*connectionState*/,
    QString /*hostname*/) {
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
}

void DltMcpServer::initMsg(int /*index*/, QDltMsg& /*msg*/) {}

void DltMcpServer::initMsgDecoded(int index, QDltMsg& msg) {
  onMessageReceived(index, msg);
}

void DltMcpServer::initFileFinish() {
  computeFileRanges();
  if (dlt_file_ && dlt_file_->size() > 0) {
    emit fileCountChanged(file_ranges_.size());
  } else {
    emit fileCountChanged(0);
  }
}

void DltMcpServer::updateFileStart() {}

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
  apids_.clear();
  ctids_.clear();
  ctid_map_.clear();
  apid_map_.clear();
  ecuids_.clear();
  ecuid_map_.clear();
  index_.clear();
  distribution_.clear();
  log_levels_.clear();
  file_ranges_.clear();
}

void DltMcpServer::onMessageReceived(int index, const QDltMsg& msg) {
  if (msg.getType() != QDltMsg::DltTypeLog) {
    return;
  }
  indexMessage(index, msg);
  updateStats(msg);
}

void DltMcpServer::indexMessage(int index, const QDltMsg& msg) {
  const auto timestamp = getWallClockNs(msg);
  const auto ecu_time = getEcuTimeTicks(msg);
  const auto ctid = msg.getCtid().toStdString();
  const auto apid = msg.getApid().toStdString();
  const auto ecuid = msg.getEcuid().toStdString();
  const auto level = msg.getSubtype();
  index_.insert({index, timestamp, ecu_time, getCtidIndex(ctid),
                 getApidIndex(apid), getEcuidIndex(ecuid), level});
}

void DltMcpServer::updateStats(const QDltMsg& msg) {
  const auto level = msg.getSubtype();
  const auto ctid = getCtidIndex(msg.getCtid().toStdString());
  const auto apid = getApidIndex(msg.getApid().toStdString());
  log_levels_[level] = msg.getSubtypeString().toStdString();
  auto& index = distribution_.get<0>();
  auto [it, inserted] = index.insert({ctid, apid, 0, {}});
  index.modify(it, [level](DistributionEntry& entry) {
    entry.total++;
    if (entry.levels.find(level) != entry.levels.end()) {
      entry.levels[level]++;
    } else {
      entry.levels[level] = 1;
    }
  });
}

size_t DltMcpServer::getCtidIndex(const std::string& ctid) {
  if (const auto it = ctid_map_.find(ctid); it != ctid_map_.end()) {
    return it->second;
  }
  ctid_map_[ctid] = ctids_.size();
  ctids_.push_back(ctid);
  return ctids_.size() - 1;
}

size_t DltMcpServer::getApidIndex(const std::string& apid) {
  if (const auto it = apid_map_.find(apid); it != apid_map_.end()) {
    return it->second;
  }
  apid_map_[apid] = apids_.size();
  apids_.push_back(apid);
  return apids_.size() - 1;
}

size_t DltMcpServer::getEcuidIndex(const std::string& ecuid) {
  if (const auto it = ecuid_map_.find(ecuid); it != ecuid_map_.end()) {
    return it->second;
  }
  ecuid_map_[ecuid] = ecuids_.size();
  ecuids_.push_back(ecuid);
  return ecuids_.size() - 1;
}

int64_t DltMcpServer::getWallClockNs(const QDltMsg& msg) {
  return static_cast<int64_t>(msg.getTime()) * 1'000'000'000LL +
         static_cast<int64_t>(msg.getMicroseconds()) * 1'000LL;
}

int64_t DltMcpServer::getEcuTimeTicks(const QDltMsg& msg) {
  return static_cast<int64_t>(msg.getTimestamp());
}

char DltMcpServer::getLevelChar(int level) {
  auto it = log_levels_.find(level);
  if (it != log_levels_.end()) {
    return static_cast<char>(
        std::toupper(static_cast<unsigned char>(it->second[0])));
  }
  return '?';
}

std::string DltMcpServer::cleanPayload(const QString& payload) {
  std::string result;
  result.reserve(payload.size());
  for (QChar c : payload) {
    if (c != '\0' && c != '\n' && c != '\r') {
      result += static_cast<char>(c.unicode());
    }
  }
  return result;
}

std::tuple<int64_t, int64_t, int64_t, int64_t> DltMcpServer::splitRelativeTime(
    int64_t offsetNs) {
  int64_t total_ms = offsetNs / 1'000'000;
  return {total_ms / 3600000, (total_ms % 3600000) / 60000,
          (total_ms % 60000) / 1000, total_ms % 1000};
}

std::tuple<int64_t, int64_t> DltMcpServer::splitRelativeTimestamp(
    int64_t offsetNs) {
  return {offsetNs / 1'000'000'000LL, (offsetNs % 1'000'000'000LL) / 1000LL};
}

std::tuple<int64_t, int64_t> DltMcpServer::splitEcuTime(int64_t ecuTimeTicks) {
  return {ecuTimeTicks / 10000, (ecuTimeTicks % 10000) / 10};
}

int64_t DltMcpServer::getBaseTimestamp() {
  return index_.get<0>().begin()->timestamp;
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

mcp::json DltMcpServer::makeTextResult(const std::string& text) {
  return {{{"type", "text"}, {"text", text}}};
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
              "APID, log level(s), "
              "timestamp range, keyword and paginated by limit/offset. Log "
              "lines include "
              "relative time offset (+HH:MM:SS.mmm), log level, CTID/APID, "
              "message ID (#), "
              "relative timestamp in seconds from log start (@), and ECU "
              "uptime in seconds (T). "
              "The value after @ can be reused directly as min_timestamp or "
              "max_timestamp "
              "parameter (float64, seconds from start of log). If payload "
              "exceeds 100 "
              "characters, the log line is a preview and ends with symbol ~. "
              "Use count=true to "
              "get only the number of matching lines. Use "
              "case_insensitive=true for "
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
              "full "
              "payload. The limit is 20 messages per tool call.")
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
              "(#) "
              "from other tools, to create clickable message references. "
              "Pass empty string to clear the report. "
              "Only generate reports when you have a conclusive analysis "
              "result — "
              "do not overwrite existing reports during intermediate "
              "exploration. "
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
  int64_t base_ts = getBaseTimestamp();
  auto& index = index_.get<0>();
  int64_t last_ts = std::prev(index.end())->timestamp;
  double duration_sec = (last_ts - base_ts) / 1'000'000'000.0;

  // Wall-clock start time for context.
  QDltMsg first_msg;
  dlt_file_->getMsg(index.begin()->index, first_msg);
  std::string log_start = fmt::format(
      "{}.{:06}", first_msg.getGmTimeWithOffsetString(0, false).toStdString(),
      first_msg.getMicroseconds());

  mcp::json log_levels = mcp::json::array();
  for (const auto& level : log_levels_) {
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
  auto& distribution_index = distribution_.get<0>();
  std::map<size_t, std::set<size_t>> contexts;
  for (const auto& distribution : distribution_index) {
    if (contexts.find(distribution.ctid) == contexts.end()) {
      contexts[distribution.ctid] = {};
    }
    contexts[distribution.ctid].insert(distribution.apid);
  }
  auto& ctx_object = summary["contexts"];
  for (const auto& [ctid, apid_list] : contexts) {
    auto apid_array = mcp::json::array();
    for (const auto& apid : apid_list) {
      apid_array.push_back(apids_[apid]);
    }
    ctx_object[ctids_[ctid]] = apid_array;
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
  std::string ctid_str;
  bool has_ctid = false;
  if (params.contains("ctid")) {
    ctid_str = params["ctid"].get<std::string>();
    has_ctid = true;
  }

  std::string apid_str;
  bool has_apid = false;
  if (params.contains("apid")) {
    apid_str = params["apid"].get<std::string>();
    has_apid = true;
  }

  bool has_min_level = params.contains("min_log_level");
  bool has_max_level = params.contains("max_log_level");
  int min_log_level = has_min_level ? params["min_log_level"].get<int>() : 0;
  int max_log_level = has_max_level ? params["max_log_level"].get<int>() : 0;

  if (has_min_level && log_levels_.find(min_log_level) == log_levels_.end()) {
    throw mcp::mcp_exception(
        mcp::error_code::invalid_params,
        "Invalid min_log_level: " + std::to_string(min_log_level));
  }
  if (has_max_level && log_levels_.find(max_log_level) == log_levels_.end()) {
    throw mcp::mcp_exception(
        mcp::error_code::invalid_params,
        "Invalid max_log_level: " + std::to_string(max_log_level));
  }

  bool has_min_ts = params.contains("min_timestamp");
  bool has_max_ts = params.contains("max_timestamp");
  double min_timestamp_sec =
      has_min_ts ? params["min_timestamp"].get<double>() : 0;
  double max_timestamp_sec =
      has_max_ts ? params["max_timestamp"].get<double>() : 0;

  QString keyword;
  bool has_keyword = false;
  if (params.contains("keyword")) {
    keyword = QString::fromStdString(params["keyword"].get<std::string>());
    has_keyword = true;
  }

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

  // Resolve CTID/APID string filters to numeric indices.
  size_t ctid_idx = 0;
  size_t apid_idx = 0;
  if (has_ctid) {
    auto it = ctid_map_.find(ctid_str);
    if (it == ctid_map_.end()) {
      // Unknown CTID - return an empty string.
      return makeTextResult("");
    }
    ctid_idx = it->second;
  }
  if (has_apid) {
    auto it = apid_map_.find(apid_str);
    if (it == apid_map_.end()) {
      // Unknown APID - return an empty string.
      return makeTextResult("");
    }
    apid_idx = it->second;
  }

  // Determine timestamp range in index.
  int64_t base_ts = getBaseTimestamp();
  auto& index = index_.get<0>();
  int64_t min_ts_ns =
      base_ts + static_cast<int64_t>(min_timestamp_sec * 1'000'000'000.0);
  int64_t max_ts_ns =
      base_ts + static_cast<int64_t>(max_timestamp_sec * 1'000'000'000.0);
  auto it_lower = has_min_ts ? index.lower_bound(min_ts_ns) : index.begin();
  auto it_upper = has_max_ts ? index.upper_bound(max_ts_ns) : index.end();

  // Collect results.
  struct SearchResult {
    int msg_index;
    int64_t timestamp;
    int64_t ecu_time;
    size_t ctid;
    size_t apid;
    int level;
  };

  std::vector<SearchResult> matches;

  int max_collect = count_only ? INT_MAX : limit + offset;
  for (auto it = it_lower;
       it != it_upper && static_cast<int>(matches.size()) < max_collect; ++it) {
    const auto& entry = *it;

    // CTID filter
    if (has_ctid && entry.ctid != ctid_idx) {
      continue;
    }

    // APID filter
    if (has_apid && entry.apid != apid_idx) {
      continue;
    }

    // Log level range filter (numeric bounds on DLT level value)
    if (has_min_level && entry.level < min_log_level) {
      continue;
    }
    if (has_max_level && entry.level > max_log_level) {
      continue;
    }

    // Keyword filter - requires fetching the message, consider caching in
    // memory.
    if (has_keyword) {
      QDltMsg msg;
      if (!dlt_file_->getMsg(entry.index, msg)) continue;
      QString payload = msg.toStringPayload();
      Qt::CaseSensitivity cs =
          case_insensitive ? Qt::CaseInsensitive : Qt::CaseSensitive;
      if (payload.indexOf(keyword, cs) == -1) continue;
    }

    matches.push_back({entry.index, entry.timestamp, entry.ecu_time, entry.ctid,
                       entry.apid, entry.level});
  }

  if (count_only) {
    return makeTextResult(std::to_string(matches.size()));
  }

  // Format output.
  std::ostringstream oss;

  for (int i = offset; i < static_cast<int>(matches.size()); i++) {
    const auto& m = matches[i];

    // Fetch full payload.
    QDltMsg msg;
    if (!dlt_file_->getMsg(m.msg_index, msg)) continue;
    QString payload = msg.toStringPayload();

    int64_t offset_ns = m.timestamp - base_ts;
    auto [hours, minutes, seconds, millis] = splitRelativeTime(offset_ns);
    auto [rel_sec, rel_usec] = splitRelativeTimestamp(offset_ns);
    auto [ecu_sec, ecu_mmm] = splitEcuTime(m.ecu_time);

    oss << formatMessageLine(
        hours, minutes, seconds, millis, getLevelChar(m.level), ctids_[m.ctid],
        apids_[m.apid], m.msg_index, rel_sec, rel_usec, ecu_sec, ecu_mmm);

    // Payload with truncation at 100 chars.
    std::string cleaned = cleanPayload(payload);
    if (static_cast<int>(cleaned.size()) > 100) {
      oss << cleaned.substr(0, 100) << "~";
    } else {
      oss << cleaned;
    }

    if (i < static_cast<int>(matches.size()) - 1) {
      oss << "\n";
    }
  }

  return makeTextResult(oss.str());
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
  int64_t base_ts = getBaseTimestamp();

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
    QString payload = msg.toStringPayload();

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

  int64_t base_ts = getBaseTimestamp();
  int64_t offset_ns = timestamp - base_ts;
  auto [hours, minutes, seconds, millis] = splitRelativeTime(offset_ns);
  auto [rel_sec, rel_usec] = splitRelativeTimestamp(offset_ns);
  auto [ecu_sec, ecu_mmm] = splitEcuTime(ecu_time);

  // Payload with cleanup, no truncation.
  QString payload = msg.toStringPayload();

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

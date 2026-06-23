/* Copyright (C) 2026, Mikhail Nikonov <michael.n.nikonov@gmail.com>
 *
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License (MPL), v. 2.0.
 * If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef DLT_MCP_SERVER_H_
#define DLT_MCP_SERVER_H_

#include <mcp_resource.h>
#include <mcp_server.h>

#include <QObject>
#include <QSettings>
#include <memory>

#include "dashboard.h"
#include "index.h"
#include "plugininterface.h"
#include "report-cache.h"
#include "statistics.h"

#ifndef DLT_VIEWER_PLUGIN_VERSION
#define DLT_VIEWER_PLUGIN_VERSION "0.0.0"
#endif

class DltMcpServer : public QObject,
                     QDLTPluginInterface,
                     QDltPluginViewerInterface,
                     QDLTPluginDecoderInterface,
                     QDltPluginControlInterface,
                     public MessageSource {
  Q_OBJECT
  Q_INTERFACES(QDLTPluginInterface)
  Q_INTERFACES(QDltPluginViewerInterface)
  Q_INTERFACES(QDLTPluginDecoderInterface)
  Q_INTERFACES(QDltPluginControlInterface)
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
  Q_PLUGIN_METADATA(IID "org.genivi.DLT.DltMcpServerPlugin")
#endif

 signals:
  void fileCountChanged(int count);

 public:
  DltMcpServer();
  ~DltMcpServer() override;

  /* QDLTPluginInterface interface */
  QString name() override;
  QString pluginVersion() override;
  QString pluginInterfaceVersion() override;
  QString description() override;
  QString error() override;
  bool loadConfig(QString filename) override;
  bool saveConfig(QString filename) override;
  QStringList infoConfig() override;

  QSettings* settings() const { return settings_.get(); }
  bool isServerRunning() const { return server_->is_running(); }
  void jumpToMessage(int index);

  /* QDltPluginViewerInterface */
  QWidget* initViewer() override;
  void initFileStart(QDltFile* file) override;
  void initFileFinish() override;
  void initMsg(int index, QDltMsg& msg) override;
  void initMsgDecoded(int index, QDltMsg& msg) override;
  void updateFileStart() override;
  void updateMsg(int index, QDltMsg& msg) override;
  void updateMsgDecoded(int index, QDltMsg& msg) override;
  void updateFileFinish() override;
  void selectedIdxMsg(int index, QDltMsg& msg) override;
  void selectedIdxMsgDecoded(int index, QDltMsg& msg) override;

  /* QDltPluginDecoderInterface */
  bool isMsg(QDltMsg& msg, int triggeredByUser) override;
  bool decodeMsg(QDltMsg& msg, int triggeredByUser) override;

  /* QDltPluginControlInterface */
  bool initControl(QDltControl* control) override;
  bool initConnections(QStringList list) override;
  bool controlMsg(int index, QDltMsg& msg) override;
  bool stateChanged(int index,
                    QDltConnection::QDltConnectionState connectionState,
                    QString hostname) override;
  bool autoscrollStateChanged(bool enabled) override;
  void initMessageDecoder(QDltMessageDecoder* messageDecoder) override;
  void initMainTableView(QTableView* pTableView) override;
  void configurationChanged() override;

  /* internal variables */
  Dashboard* dashboard_;

 private:
  struct LogFileInfo {
    std::string name;
    int message_count;
    int start_index;
  };

  void reset();
  void onMessageReceived(int index, const QDltMsg& msg);

  std::optional<QDltMsg> get(int index) const override;

  char getLevelChar(int level);

  void computeFileRanges();
  void liveUpdateLastFileMessageCount();
  std::string loadContextFile();

  static std::string formatMessageLine(int64_t hours, int64_t minutes,
                                       int64_t seconds, int64_t millis,
                                       char levelChar, const std::string& ctid,
                                       const std::string& apid, int msgIndex,
                                       int64_t relSec, int64_t relUsec,
                                       int64_t ecuSec, int64_t ecuMmm);

  static mcp::json makeTextResult(const std::string& text,
                                  const std::string& warning = "");

  static bool looksLikeRegex(const std::string& s);

  void initMcpServer();
  void registerMcpTools();

  mcp::json get_log_summary(const mcp::json& params,
                            const std::string& session_id);
  mcp::json search(const mcp::json& params, const std::string& session_id);
  mcp::json get_messages(const mcp::json& params,
                         const std::string& session_id);
  mcp::json get_selection(const mcp::json& params,
                          const std::string& session_id);
  mcp::json set_report(const mcp::json& params, const std::string& session_id);

  QString plugin_name_displayed = QString("DLT MCP Server");

  QDltFile* dlt_file_{nullptr};
  QDltControl* control_{nullptr};
  int selected_index_{-1};

  std::unique_ptr<Index> index_;
  Statistics statistics_;

  std::unique_ptr<mcp::server> server_;
  std::unique_ptr<QSettings> settings_;

  std::string context_file_path_;
  std::string context_file_content_;

  std::vector<LogFileInfo> file_ranges_;

  std::unique_ptr<ReportCache> reportCache_;

  bool is_live_{false};

  std::string buildReportKey();
};

#endif  // DLT_MCP_SERVER_H_

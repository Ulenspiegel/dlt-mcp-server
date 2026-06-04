/* Copyright (C) 2026, Mikhail Nikonov <michael.n.nikonov@gmail.com>
 *
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License (MPL), v. 2.0.
 * If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef DLT_MCP_SERVER_H_
#define DLT_MCP_SERVER_H_

#include "dashboard.h"
#include "plugininterface.h"
#include <QObject>
#include <QSettings>

#include <mcp_resource.h>
#include <mcp_server.h>

#include <tuple>
#include <unordered_map>

#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>

#ifndef DLT_VIEWER_PLUGIN_VERSION
#define DLT_VIEWER_PLUGIN_VERSION "0.0.0"
#endif

namespace bmi = boost::multi_index;

class DltMcpServer : public QObject,
                     QDLTPluginInterface,
                     QDltPluginViewerInterface,
                     QDLTPluginDecoderInterface {
    Q_OBJECT
    Q_INTERFACES(QDLTPluginInterface)
    Q_INTERFACES(QDltPluginViewerInterface)
    Q_INTERFACES(QDLTPluginDecoderInterface)
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

    /* internal variables */
    Dashboard* dashboard_;

private:
    struct IndexEntry {
        int index;
        int64_t timestamp;
        int64_t ecu_time;
        size_t ctid;
        size_t apid;
        size_t ecuid;
        int level;
    };

    struct by_timestamp {};

    using Index = bmi::multi_index_container<
        IndexEntry,
        bmi::indexed_by<bmi::ordered_non_unique<
            bmi::tag<by_timestamp>, bmi::member<IndexEntry, int64_t, &IndexEntry::timestamp>>>>;

    struct DistributionEntry {
        size_t ctid{};
        size_t apid{};
        size_t total{};
        std::map<int, size_t> levels; // <Log level, number of messages.>
    };

    struct LogFileInfo {
        std::string name;
        int message_count;
        int start_index;
    };

    using Distribution = bmi::multi_index_container<
        DistributionEntry,
        bmi::indexed_by<bmi::ordered_unique<bmi::composite_key<
            DistributionEntry, bmi::member<DistributionEntry, size_t, &DistributionEntry::ctid>,
            bmi::member<DistributionEntry, size_t, &DistributionEntry::apid>>>>>;

    void reset();
    void onMessageReceived(int index, const QDltMsg& msg);
    void indexMessage(int index, const QDltMsg& msg);
    void updateStats(const QDltMsg& msg);

    size_t getCtidIndex(const std::string& ctid);
    size_t getApidIndex(const std::string& apid);
    size_t getEcuidIndex(const std::string& ecuid);

    static int64_t getWallClockNs(const QDltMsg& msg);
    static int64_t getEcuTimeTicks(const QDltMsg& msg);

    char getLevelChar(int level);
    static std::string cleanPayload(const QString& payload);

    // Returns {hours, minutes, seconds, milliseconds} from nanosecond offset.
    static std::tuple<int64_t, int64_t, int64_t, int64_t> splitRelativeTime(int64_t offsetNs);

    // Returns {seconds, microseconds} from nanosecond offset.
    static std::tuple<int64_t, int64_t> splitRelativeTimestamp(int64_t offsetNs);

    // Returns {seconds, milliseconds} from ECU time ticks (0.1ms units).
    static std::tuple<int64_t, int64_t> splitEcuTime(int64_t ecuTimeTicks);

    int64_t getBaseTimestamp();

    void computeFileRanges();
    void liveUpdateLastFileMessageCount();
    std::string loadContextFile();

    static std::string formatMessageLine(int64_t hours, int64_t minutes, int64_t seconds,
                                         int64_t millis, char levelChar, const std::string& ctid,
                                         const std::string& apid, int msgIndex, int64_t relSec,
                                         int64_t relUsec, int64_t ecuSec, int64_t ecuMmm);

    static mcp::json makeTextResult(const std::string& text);

    void initMcpServer();
    void registerMcpTools();

    mcp::json get_log_summary(const mcp::json& params, const std::string& session_id);
    mcp::json search(const mcp::json& params, const std::string& session_id);
    mcp::json get_messages(const mcp::json& params, const std::string& session_id);
    mcp::json get_selection(const mcp::json& params, const std::string& session_id);

    QString plugin_name_displayed = QString("DLT MCP Server");

    QDltFile* dlt_file_{nullptr};
    int selected_index_{-1};

    std::unordered_map<std::string, size_t> apid_map_;
    std::unordered_map<std::string, size_t> ctid_map_;
    std::unordered_map<std::string, size_t> ecuid_map_;
    std::vector<std::string> apids_;
    std::vector<std::string> ctids_;
    std::vector<std::string> ecuids_;

    std::map<int, std::string> log_levels_;

    Index index_;
    Distribution distribution_;

    std::unique_ptr<mcp::server> server_;
    std::unique_ptr<QSettings> settings_;

    std::string context_file_path_;
    std::string context_file_content_;

    std::vector<LogFileInfo> file_ranges_;
};

#endif // DLT_MCP_SERVER_H_

#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVector>

class QTimer;

namespace speed::client::legacy {
class LegacyClient;
}
namespace speed::client::rest {
class RestClient;
}

class ClientBackend : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString legacyHost READ legacyHost WRITE setLegacyHost NOTIFY legacyHostChanged)
    Q_PROPERTY(QString restHost READ restHost WRITE setRestHost NOTIFY restHostChanged)
    Q_PROPERTY(QString legacyEndpoint READ legacyEndpoint NOTIFY legacyEndpointChanged)
    Q_PROPERTY(QString restEndpoint READ restEndpoint NOTIFY restEndpointChanged)
    Q_PROPERTY(int legacyPort READ legacyPort WRITE setLegacyPort NOTIFY legacyPortChanged)
    Q_PROPERTY(int restPort READ restPort WRITE setRestPort NOTIFY restPortChanged)
    Q_PROPERTY(bool restUseHttps READ restUseHttps WRITE setRestUseHttps NOTIFY restUseHttpsChanged)
    Q_PROPERTY(bool autoConnectOnStartup READ autoConnectOnStartup WRITE setAutoConnectOnStartup
                   NOTIFY autoConnectOnStartupChanged)
    Q_PROPERTY(QString subscribeTopic READ subscribeTopic WRITE setSubscribeTopic NOTIFY
                   subscribeTopicChanged)
    Q_PROPERTY(QString echoText READ echoText WRITE setEchoText NOTIFY echoTextChanged)
    Q_PROPERTY(bool legacyConnected READ legacyConnected NOTIFY legacyConnectedChanged)
    Q_PROPERTY(bool legacyBusy READ legacyBusy NOTIFY legacyBusyChanged)
    Q_PROPERTY(bool restBusy READ restBusy NOTIFY restBusyChanged)
    Q_PROPERTY(bool pushPollEnabled READ pushPollEnabled WRITE setPushPollEnabled NOTIFY
                   pushPollEnabledChanged)
    Q_PROPERTY(bool legacySubscriptionActive READ legacySubscriptionActive NOTIFY
                   legacySubscriptionActiveChanged)
    Q_PROPERTY(QString lastRestHealth READ lastRestHealth NOTIFY lastRestHealthChanged)
    Q_PROPERTY(QString lastRestEcho READ lastRestEcho NOTIFY lastRestEchoChanged)
    Q_PROPERTY(QString lastLegacyLine READ lastLegacyLine NOTIFY lastLegacyLineChanged)
    Q_PROPERTY(QStringList eventLog READ eventLog NOTIFY eventLogChanged)
    /** Step 图：时间类目 + 多路 0/1 序列（供 ShadcnChart series / categories） */
    Q_PROPERTY(QStringList statusChartCategories READ statusChartCategories NOTIFY
                   statusChartChanged)
    Q_PROPERTY(QStringList statusChartAxisLabels READ statusChartAxisLabels NOTIFY
                   statusChartChanged)
    Q_PROPERTY(QStringList statusChartHoverLabels READ statusChartHoverLabels NOTIFY
                   statusChartChanged)
    Q_PROPERTY(QVariantList statusChartSeries READ statusChartSeries NOTIFY statusChartChanged)
    Q_PROPERTY(int statusHistorySeconds READ statusHistorySeconds CONSTANT)
    Q_PROPERTY(bool serverSnapshotBusy READ serverSnapshotBusy NOTIFY serverSnapshotBusyChanged)
    Q_PROPERTY(bool serverSnapshotReady READ serverSnapshotReady NOTIFY serverSnapshotChanged)
    Q_PROPERTY(QString serverSnapshotError READ serverSnapshotError NOTIFY serverSnapshotChanged)
    Q_PROPERTY(QString serverSnapshotUpdatedLabel READ serverSnapshotUpdatedLabel NOTIFY
                   serverSnapshotChanged)
    Q_PROPERTY(QString serverAppName READ serverAppName NOTIFY serverSnapshotChanged)
    Q_PROPERTY(QString serverPlatformLabel READ serverPlatformLabel NOTIFY serverSnapshotChanged)
    Q_PROPERTY(QVariantList serverOpsTiles READ serverOpsTiles NOTIFY serverSnapshotChanged)
    Q_PROPERTY(double serverMemUsedPercent READ serverMemUsedPercent NOTIFY serverSnapshotChanged)
    Q_PROPERTY(double serverCpuPercent READ serverCpuPercent NOTIFY serverSnapshotChanged)
    Q_PROPERTY(QString serverHostMemDetail READ serverHostMemDetail NOTIFY serverSnapshotChanged)
    Q_PROPERTY(QString serverProcessRssDetail READ serverProcessRssDetail NOTIFY serverSnapshotChanged)
    Q_PROPERTY(double serverRootDiskUsedPercent READ serverRootDiskUsedPercent NOTIFY serverSnapshotChanged)
    Q_PROPERTY(QString serverRootDiskDetail READ serverRootDiskDetail NOTIFY serverSnapshotChanged)
    Q_PROPERTY(QString serverRootDiskMount READ serverRootDiskMount NOTIFY serverSnapshotChanged)
    Q_PROPERTY(bool serverHasRootDisk READ serverHasRootDisk NOTIFY serverSnapshotChanged)
    Q_PROPERTY(double serverDataDiskUsedPercent READ serverDataDiskUsedPercent NOTIFY serverSnapshotChanged)
    Q_PROPERTY(QString serverDataDiskDetail READ serverDataDiskDetail NOTIFY serverSnapshotChanged)
    Q_PROPERTY(QString serverDataDiskMount READ serverDataDiskMount NOTIFY serverSnapshotChanged)
    Q_PROPERTY(bool serverHasDataDisk READ serverHasDataDisk NOTIFY serverSnapshotChanged)
    Q_PROPERTY(bool serverOpsStreamActive READ serverOpsStreamActive NOTIFY serverOpsStreamChanged)
    Q_PROPERTY(int legacyPingRttMs READ legacyPingRttMs NOTIFY legacyPingRttChanged)
    Q_PROPERTY(QString legacyPingLabel READ legacyPingLabel NOTIFY legacyPingRttChanged)
    Q_PROPERTY(QString legacyPingTone READ legacyPingTone NOTIFY legacyPingRttChanged)
    Q_PROPERTY(QString dashboardRequestsTotal READ dashboardRequestsTotal NOTIFY opsDashboardChanged)
    Q_PROPERTY(QString dashboardEventsTotal READ dashboardEventsTotal NOTIFY opsDashboardChanged)
    /** 本地日历日增量（Snapshot 内 framework.*.today，需服务端 ops.history） */
    Q_PROPERTY(QString dashboardRequestsToday READ dashboardRequestsToday NOTIFY opsDashboardChanged)
    Q_PROPERTY(QString dashboardEventsToday READ dashboardEventsToday NOTIFY opsDashboardChanged)
    Q_PROPERTY(QVariantList opsHeatmapDays READ opsHeatmapDays NOTIFY opsDashboardChanged)
    /** 近 7 日趋势：Daily 归档 + 当日 Snapshot framework.*.today */
    Q_PROPERTY(QVariantList opsTrendDays READ opsTrendDays NOTIFY opsDashboardChanged)
    Q_PROPERTY(bool opsDashboardBusy READ opsDashboardBusy NOTIFY opsDashboardChanged)
    Q_PROPERTY(QString opsDashboardError READ opsDashboardError NOTIFY opsDashboardChanged)
    Q_PROPERTY(bool opsHeatmapReady READ opsHeatmapReady NOTIFY opsDashboardChanged)
    Q_PROPERTY(bool opsTrendReady READ opsTrendReady NOTIFY opsDashboardChanged)

public:
    explicit ClientBackend(QObject *parent = nullptr);
    ~ClientBackend() override;

    QString legacyHost() const;
    QString restHost() const;
    QString legacyEndpoint() const;
    QString restEndpoint() const;
    int legacyPort() const;
    int restPort() const;
    bool restUseHttps() const;
    bool autoConnectOnStartup() const;
    QString subscribeTopic() const;
    QString echoText() const;
    bool legacyConnected() const;
    bool legacyBusy() const;
    bool restBusy() const;
    bool pushPollEnabled() const;
    bool legacySubscriptionActive() const;
    QString lastRestHealth() const;
    QString lastRestEcho() const;
    QString lastLegacyLine() const;
    QStringList eventLog() const;
    QStringList statusChartCategories() const;
    QStringList statusChartAxisLabels() const;
    QStringList statusChartHoverLabels() const;
    QVariantList statusChartSeries() const;
    int statusHistorySeconds() const;
    bool serverSnapshotBusy() const;
    bool serverSnapshotReady() const;
    QString serverSnapshotError() const;
    QString serverSnapshotUpdatedLabel() const;
    QString serverAppName() const;
    QString serverPlatformLabel() const;
    QVariantList serverOpsTiles() const;
    double serverMemUsedPercent() const;
    double serverCpuPercent() const;
    QString serverHostMemDetail() const;
    QString serverProcessRssDetail() const;
    double serverRootDiskUsedPercent() const;
    QString serverRootDiskDetail() const;
    QString serverRootDiskMount() const;
    bool serverHasRootDisk() const;
    double serverDataDiskUsedPercent() const;
    QString serverDataDiskDetail() const;
    QString serverDataDiskMount() const;
    bool serverHasDataDisk() const;
    bool serverOpsStreamActive() const;
    int legacyPingRttMs() const;
    QString legacyPingLabel() const;
    QString legacyPingTone() const;
    QString dashboardRequestsTotal() const;
    QString dashboardEventsTotal() const;
    QString dashboardRequestsToday() const;
    QString dashboardEventsToday() const;
    QVariantList opsHeatmapDays() const;
    QVariantList opsTrendDays() const;
    bool opsDashboardBusy() const;
    QString opsDashboardError() const;
    bool opsHeatmapReady() const;
    bool opsTrendReady() const;

    void setLegacyHost(const QString &value);
    void setRestHost(const QString &value);
    void setLegacyPort(int value);
    void setRestPort(int value);
    void setRestUseHttps(bool value);
    void setAutoConnectOnStartup(bool value);
    void setSubscribeTopic(const QString &value);
    void setEchoText(const QString &value);
    void setPushPollEnabled(bool value);

    Q_INVOKABLE void saveSettings();
    Q_INVOKABLE void connectLegacy();
    Q_INVOKABLE void disconnectLegacy();
    Q_INVOKABLE void legacyPing();
    Q_INVOKABLE void legacyEcho();
    Q_INVOKABLE void legacySubscribe();
    Q_INVOKABLE void legacyUnsubscribe();
    Q_INVOKABLE void restHealth();
    Q_INVOKABLE void restEcho();
    Q_INVOKABLE void clearLog();
    /** Legacy 连接 + REST /health（启动与手动「立即连接」共用） */
    Q_INVOKABLE void connectOnStartup();
    /** 恢复生产环境默认（api.qfot.com / 9001 / REST 443 HTTPS）并保存 */
    Q_INVOKABLE void applyCpolarDefaults();
    Q_INVOKABLE void clearStatusHistory();
    Q_INVOKABLE void fetchServerSnapshot();
    Q_INVOKABLE void refreshOpsDashboard();

Q_SIGNALS:
    void legacyHostChanged();
    void restHostChanged();
    void legacyEndpointChanged();
    void restEndpointChanged();
    void legacyPortChanged();
    void restPortChanged();
    void restUseHttpsChanged();
    void autoConnectOnStartupChanged();
    void subscribeTopicChanged();
    void echoTextChanged();
    void legacyConnectedChanged();
    void legacyBusyChanged();
    void restBusyChanged();
    void pushPollEnabledChanged();
    void legacySubscriptionActiveChanged();
    void lastRestHealthChanged();
    void lastRestEchoChanged();
    void lastLegacyLineChanged();
    void eventLogChanged();
    void statusChartChanged();
    void serverSnapshotBusyChanged();
    void serverSnapshotChanged();
    void serverOpsStreamChanged();
    void legacyPingRttChanged();
    void toastRequested(const QString &message, const QString &level);
    void opsDashboardChanged();

private:
    void parseOpsSnapshotPayload(const QString &body, int httpStatus);
    void applyOpsSnapshotJson(const QString &body, const QString &source);
    void parseOpsDashboardPayload(const QString &body, int httpStatus);
    void parseOpsHeatmapPayload(const QString &body, int httpStatus);
    void parseOpsHistoryDailyPayload(const QString &body, int httpStatus);
    bool applyOpsDailyListObject(const QJsonObject &root);
    bool applyOpsHeatmapObject(const QJsonObject &root);
    void updateDashboardTotalsFromMetrics(const QJsonArray &metrics);
    void rebuildOpsTrendDays();
    void onOpsDashboardHistoryFetchFinished();
    static quint64 metricCountU64(const QJsonArray &metrics, const QString &id);
    void subscribeOpsMetrics();
    void restoreOpsMetricsSubscription();
    void resetLegacyPingRtt();
    static QVariant metricValue(const QJsonArray &metrics, const QString &id);
    static QString formatBytes(qint64 bytes);
    static QString formatUsedTotalBytes(qint64 usedBytes, qint64 totalBytes);
    static QString formatDurationSec(double seconds);
    static double usedPercentFromBytes(qint64 availBytes, qint64 totalBytes);

    static constexpr double kInvalidMetric = -1.0;
    void sampleStatusPoint();
    void rebuildStatusChartSeries();
    bool restHealthOk() const;
    void appendLog(const QString &line);
    void applyEndpoints();
    /** delayMs < 0 时使用 reconnect_backoff_ms_（连接失败退避） */
    void scheduleAutoReconnect(int delayMs = -1);
    void runAutoConnect(const QString &reason); // Legacy TCP + REST /health（启动/手动）
    void runLegacyReconnect(const QString &reason);
    void restoreLegacySessionAfterConnect();

    static constexpr int kReconnectDelaySessionLostMs = 250;
    static constexpr int kReconnectBackoffInitialMs = 500;
    static constexpr int kReconnectBackoffMaxMs = 8000;
    /** 仅保留最近 5 个 1s 采样，绘图与内存均为 5s 滑动窗 */
    static constexpr int kStatusHistoryMax = 5;
    static constexpr int kStatusSampleIntervalMs = 1000;
    static constexpr int kStatusPlotMaxPoints = kStatusHistoryMax;
    static constexpr int kOpsDashboardTrendDays = 7;
    speed::client::legacy::LegacyClient *legacy_{nullptr};
    speed::client::rest::RestClient *rest_{nullptr};
    QTimer *reconnect_timer_{nullptr};
    QTimer *status_sample_timer_{nullptr};

    QString legacy_host_;
    QString rest_host_;
    int legacy_port_{9001};
    int rest_port_{443};
    bool rest_use_https_{true};
    bool auto_connect_on_startup_{true};
    bool manual_legacy_disconnect_{false};
    bool startup_connect_done_{false};
    QString subscribe_topic_;
    QString echo_text_;
    bool push_poll_enabled_{false};
    /** 用户曾 SUB 且未 UNSUB 时，重连后自动恢复订阅 */
    bool legacy_subscription_active_{false};
    int reconnect_backoff_ms_{kReconnectBackoffInitialMs};
    QString last_rest_health_;
    QString last_rest_echo_;
    QString last_legacy_line_;
    QStringList event_log_;
    QStringList status_time_labels_;
    QVector<double> legacy_conn_history_;
    QVector<double> legacy_busy_history_;
    QVector<double> rest_ok_history_;
    QVector<double> push_poll_history_;
    QVector<double> subscription_history_;
    QStringList plot_time_labels_;
    QVector<double> plot_legacy_conn_;
    QVector<double> plot_legacy_busy_;
    QVector<double> plot_rest_ok_;
    QVector<double> plot_push_poll_;
    QVector<double> plot_subscription_;
    QVariantList status_chart_series_;
    QStringList status_slot_categories_;

    bool server_snapshot_ready_{false};
    QString server_snapshot_error_;
    QString server_snapshot_updated_label_;
    QString server_app_name_;
    QString server_platform_label_;
    QVariantList server_ops_tiles_;
    double server_mem_used_percent_{kInvalidMetric};
    double server_cpu_percent_{kInvalidMetric};
    QString server_host_mem_detail_;
    QString server_process_rss_detail_;
    double server_root_disk_used_percent_{kInvalidMetric};
    QString server_root_disk_detail_;
    QString server_root_disk_mount_;
    bool server_has_root_disk_{false};
    double server_data_disk_used_percent_{kInvalidMetric};
    QString server_data_disk_detail_;
    QString server_data_disk_mount_;
    bool server_has_data_disk_{false};
    qint64 server_snapshot_ts_ms_{0};
    bool ops_metrics_subscribe_pending_{true};
    bool ops_metrics_stream_active_{false};
    int legacy_ping_rtt_ms_{-1};

    QString dashboard_requests_total_{QStringLiteral("—")};
    QString dashboard_requests_today_{QStringLiteral("—")};
    QString dashboard_events_total_{QStringLiteral("—")};
    QString dashboard_events_today_{QStringLiteral("—")};
    QVariantList ops_heatmap_days_;
    QVariantList ops_daily_days_;
    QVariantList ops_trend_days_;
    bool ops_heatmap_ready_{false};
    bool ops_trend_ready_{false};
    QString ops_dashboard_error_;
    bool ops_dashboard_refresh_pending_{false};
    int ops_dashboard_history_pending_{0};
    bool dashboard_requests_today_valid_{false};
    bool dashboard_events_today_valid_{false};
    quint64 dashboard_requests_today_count_{0};
    quint64 dashboard_events_today_count_{0};
};

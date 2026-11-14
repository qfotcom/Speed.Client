#include "ClientBackend.h"

#include <QtMath>

#include <QDate>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTimer>
#include <QVariantMap>

#include "config/client_settings.hpp"
#include "legacy/legacy_client.hpp"
#include "rest/rest_client.hpp"

#include <QSet>

#include <algorithm>

namespace {

const QString kOpsMetricsTopic = QStringLiteral("ops.host.metrics");

QString shortTimeLabel(const QString &hhmmss)
{
    if (hhmmss.size() >= 5) {
        return hhmmss.left(5);
    }
    return hhmmss;
}

bool channelChanged(const QVector<double> &values, int index)
{
    if (index <= 0 || index >= values.size()) {
        return false;
    }
    return values.at(index) != values.at(index - 1);
}

QList<int> buildPlotIndices(int count, int maxPoints, const QVector<QVector<double> > &channels)
{
    if (count <= 0) {
        return {};
    }
    if (count <= maxPoints) {
        QList<int> all;
        all.reserve(count);
        for (int i = 0; i < count; ++i) {
            all.append(i);
        }
        return all;
    }

    QSet<int> keep;
    keep.insert(0);
    keep.insert(count - 1);
    for (int i = 1; i < count; ++i) {
        for (const QVector<double> &channel : channels) {
            if (channelChanged(channel, i)) {
                keep.insert(i);
                keep.insert(i - 1);
                break;
            }
        }
    }

    QList<int> indices = keep.values();
    std::sort(indices.begin(), indices.end());

    if (indices.size() > maxPoints) {
        QList<int> thinned;
        thinned.reserve(maxPoints);
        const int last = indices.size() - 1;
        for (int k = 0; k < maxPoints; ++k) {
            const int pick = (k * last) / (maxPoints - 1);
            thinned.append(indices.at(pick));
        }
        indices = thinned;
    }

    return indices;
}

QString formatCountMetric(double n)
{
    if (!qIsFinite(n)) {
        return QStringLiteral("—");
    }
    const qint64 whole = static_cast<qint64>(qRound64(n));
    if (whole >= 1'000'000) {
        return QStringLiteral("%1M").arg(n / 1'000'000.0, 0, 'f', 1);
    }
    if (whole >= 10'000) {
        return QStringLiteral("%1K").arg(n / 1'000.0, 0, 'f', 1);
    }
    return QString::number(whole);
}

} // namespace

ClientBackend::ClientBackend(QObject *parent)
    : QObject(parent)
    , legacy_(new speed::client::legacy::LegacyClient(this))
    , rest_(new speed::client::rest::RestClient(this))
{
    const speed::client::ClientSettings cfg = speed::client::ClientSettings::load();
    legacy_host_ = cfg.legacy_host;
    rest_host_ = cfg.rest_host;
    legacy_port_ = cfg.legacy_port;
    rest_port_ = cfg.rest_port;
    subscribe_topic_ = cfg.subscribe_topic;
    echo_text_ = cfg.echo_text;
    auto_connect_on_startup_ = cfg.auto_connect_on_startup;
    applyEndpoints();

    reconnect_timer_ = new QTimer(this);
    reconnect_timer_->setSingleShot(true);

    connect(reconnect_timer_, &QTimer::timeout, this, [this]() {
        if (!auto_connect_on_startup_ || manual_legacy_disconnect_ || legacyConnected()) {
            return;
        }
        runLegacyReconnect(QStringLiteral("自动重连"));
    });

    connect(legacy_, &speed::client::legacy::LegacyClient::connectedChanged, this, [this]() {
        Q_EMIT legacyConnectedChanged();
        sampleStatusPoint();
        if (legacyConnected()) {
            reconnect_timer_->stop();
            reconnect_backoff_ms_ = kReconnectBackoffInitialMs;
            ops_metrics_subscribe_pending_ = true;
            ops_metrics_stream_active_ = false;
            Q_EMIT serverOpsStreamChanged();
            restoreLegacySessionAfterConnect();
            subscribeOpsMetrics();
            fetchServerSnapshot();
            startLegacyPingProbe();
            return;
        }
        stopLegacyPingProbe();
        legacy_ping_rtt_ms_ = -1;
        Q_EMIT legacyPingRttChanged();
        ops_metrics_stream_active_ = false;
        ops_metrics_subscribe_pending_ = auto_connect_on_startup_;
        Q_EMIT serverOpsStreamChanged();
        if (auto_connect_on_startup_ && !manual_legacy_disconnect_) {
            scheduleAutoReconnect(kReconnectDelaySessionLostMs);
        }
    });
    connect(legacy_, &speed::client::legacy::LegacyClient::busyChanged, this,
            &ClientBackend::legacyBusyChanged);
    connect(legacy_, &speed::client::legacy::LegacyClient::errorOccurred, this,
            [this](const QString &msg) {
                appendLog(QStringLiteral("[Legacy ERR] %1").arg(msg));
                Q_EMIT toastRequested(msg, QStringLiteral("error"));
                if (auto_connect_on_startup_ && !manual_legacy_disconnect_ && !legacyConnected()) {
                    scheduleAutoReconnect(reconnect_backoff_ms_);
                    reconnect_backoff_ms_ =
                        qMin(reconnect_backoff_ms_ * 2, kReconnectBackoffMaxMs);
                }
            });
    connect(legacy_, &speed::client::legacy::LegacyClient::responseReceived, this,
            [this](const QString &primary, const QString &raw) {
                last_legacy_line_ = primary;
                Q_EMIT lastLegacyLineChanged();
                if (primary != QStringLiteral("PONG")) {
                    appendLog(QStringLiteral("[Legacy] %1").arg(raw));
                }
            });
    connect(legacy_, &speed::client::legacy::LegacyClient::pingRttMeasured, this,
            [this](int rttMs) {
                legacy_ping_rtt_ms_ = rttMs;
                Q_EMIT legacyPingRttChanged();
            });
    connect(legacy_, &speed::client::legacy::LegacyClient::pushReceived, this,
            [this](const QString &topic, const QString &payload) {
                if (topic == kOpsMetricsTopic) {
                    applyOpsSnapshotJson(payload, QStringLiteral("PUSH"));
                    return;
                }
                appendLog(QStringLiteral("[PUSH] %1 %2").arg(topic, payload));
            });

    connect(rest_, &speed::client::rest::RestClient::busyChanged, this,
            &ClientBackend::restBusyChanged);
    connect(rest_, &speed::client::rest::RestClient::errorOccurred, this,
            [this](const QString &msg) {
                appendLog(QStringLiteral("[REST ERR] %1").arg(msg));
                if (msg.endsWith(QStringLiteral("请求进行中"))) {
                    return;
                }
                Q_EMIT toastRequested(msg, QStringLiteral("error"));
            });
    connect(rest_, &speed::client::rest::RestClient::healthReceived, this,
            [this](const QString &body, int status) {
                last_rest_health_ = QStringLiteral("HTTP %1: %2").arg(status).arg(body);
                Q_EMIT lastRestHealthChanged();
                sampleStatusPoint();
                appendLog(QStringLiteral("[REST /health] %1").arg(last_rest_health_));
            });
    connect(rest_, &speed::client::rest::RestClient::echoReceived, this,
            [this](const QString &body, int status) {
                last_rest_echo_ = QStringLiteral("HTTP %1: %2").arg(status).arg(body);
                Q_EMIT lastRestEchoChanged();
                appendLog(QStringLiteral("[REST echo] %1").arg(last_rest_echo_));
            });
    connect(rest_, &speed::client::rest::RestClient::snapshotBusyChanged, this,
            &ClientBackend::serverSnapshotBusyChanged);
    connect(rest_, &speed::client::rest::RestClient::opsSnapshotReceived, this,
            [this](const QString &body, int status) {
                parseOpsSnapshotPayload(body, status);
            });
    connect(rest_, &speed::client::rest::RestClient::opsHistoryHeatmapReceived, this,
            [this](const QString &body, int status) {
                parseOpsHeatmapPayload(body, status);
            });

    status_sample_timer_ = new QTimer(this);
    status_sample_timer_->setInterval(kStatusSampleIntervalMs);
    connect(status_sample_timer_, &QTimer::timeout, this, &ClientBackend::sampleStatusPoint);
    status_sample_timer_->start();
    sampleStatusPoint();

    legacy_ping_timer_ = new QTimer(this);
    legacy_ping_timer_->setInterval(2000);
    connect(legacy_ping_timer_, &QTimer::timeout, this, [this]() {
        if (legacyConnected()) {
            legacy_->sendLine(QStringLiteral("PING"));
        }
    });

    // 下一事件循环即连接（Main.qml callLater 会再调一次，由 startup_connect_done_ 去重）
    QTimer::singleShot(0, this, &ClientBackend::connectOnStartup);
}

ClientBackend::~ClientBackend() = default;

QString ClientBackend::legacyHost() const
{
    return legacy_host_;
}

QString ClientBackend::restHost() const
{
    return rest_host_;
}

QString ClientBackend::legacyEndpoint() const
{
    return QStringLiteral("%1:%2").arg(legacy_host_).arg(legacy_port_);
}

QString ClientBackend::restEndpoint() const
{
    return QStringLiteral("%1:%2").arg(rest_host_).arg(rest_port_);
}

int ClientBackend::legacyPort() const
{
    return legacy_port_;
}

int ClientBackend::restPort() const
{
    return rest_port_;
}

bool ClientBackend::autoConnectOnStartup() const
{
    return auto_connect_on_startup_;
}

QString ClientBackend::subscribeTopic() const
{
    return subscribe_topic_;
}

QString ClientBackend::echoText() const
{
    return echo_text_;
}

bool ClientBackend::legacyConnected() const
{
    return legacy_->isConnected();
}

bool ClientBackend::legacyBusy() const
{
    return legacy_->isBusy();
}

bool ClientBackend::restBusy() const
{
    return rest_->isBusy();
}

bool ClientBackend::pushPollEnabled() const
{
    return push_poll_enabled_;
}

bool ClientBackend::legacySubscriptionActive() const
{
    return legacy_subscription_active_;
}

QString ClientBackend::lastRestHealth() const
{
    return last_rest_health_;
}

QString ClientBackend::lastRestEcho() const
{
    return last_rest_echo_;
}

QString ClientBackend::lastLegacyLine() const
{
    return last_legacy_line_;
}

QStringList ClientBackend::eventLog() const
{
    return event_log_;
}

void ClientBackend::setLegacyHost(const QString &value)
{
    const QString trimmed = value.trimmed();
    if (legacy_host_ == trimmed) {
        return;
    }
    legacy_host_ = trimmed;
    applyEndpoints();
    Q_EMIT legacyHostChanged();
    Q_EMIT legacyEndpointChanged();
}

void ClientBackend::setRestHost(const QString &value)
{
    const QString trimmed = value.trimmed();
    if (rest_host_ == trimmed) {
        return;
    }
    rest_host_ = trimmed;
    applyEndpoints();
    Q_EMIT restHostChanged();
    Q_EMIT restEndpointChanged();
}

void ClientBackend::setLegacyPort(int value)
{
    if (legacy_port_ == value) {
        return;
    }
    legacy_port_ = value;
    applyEndpoints();
    Q_EMIT legacyPortChanged();
    Q_EMIT legacyEndpointChanged();
}

void ClientBackend::setRestPort(int value)
{
    if (rest_port_ == value) {
        return;
    }
    rest_port_ = value;
    applyEndpoints();
    Q_EMIT restPortChanged();
    Q_EMIT restEndpointChanged();
}

void ClientBackend::setSubscribeTopic(const QString &value)
{
    if (subscribe_topic_ == value) {
        return;
    }
    subscribe_topic_ = value;
    Q_EMIT subscribeTopicChanged();
}

void ClientBackend::setEchoText(const QString &value)
{
    if (echo_text_ == value) {
        return;
    }
    echo_text_ = value;
    Q_EMIT echoTextChanged();
}

void ClientBackend::setPushPollEnabled(bool value)
{
    if (push_poll_enabled_ == value) {
        return;
    }
    push_poll_enabled_ = value;
    legacy_->setPushPollEnabled(value);
    Q_EMIT pushPollEnabledChanged();
    sampleStatusPoint();
}

bool ClientBackend::serverOpsStreamActive() const
{
    return ops_metrics_stream_active_;
}

int ClientBackend::legacyPingRttMs() const
{
    return legacy_ping_rtt_ms_;
}

QString ClientBackend::legacyPingLabel() const
{
    if (legacy_ping_rtt_ms_ < 0) {
        return legacyConnected() ? QStringLiteral("测速中…")
                                 : QStringLiteral("Legacy 未连接");
    }
    return QStringLiteral("%1 ms → %2").arg(legacy_ping_rtt_ms_).arg(legacyEndpoint());
}

QString ClientBackend::legacyPingTone() const
{
    if (!legacyConnected() || legacy_ping_rtt_ms_ < 0) {
        return QStringLiteral("pending");
    }
    if (legacy_ping_rtt_ms_ < 80) {
        return QStringLiteral("success");
    }
    if (legacy_ping_rtt_ms_ < 200) {
        return QStringLiteral("neutral");
    }
    return QStringLiteral("danger");
}

void ClientBackend::startLegacyPingProbe()
{
    if (!legacy_ping_timer_) {
        return;
    }
    legacy_ping_rtt_ms_ = -1;
    Q_EMIT legacyPingRttChanged();
    if (!legacy_ping_timer_->isActive()) {
        legacy_ping_timer_->start();
    }
    QTimer::singleShot(200, this, [this]() {
        if (legacyConnected()) {
            legacy_->sendLine(QStringLiteral("PING"));
        }
    });
}

void ClientBackend::stopLegacyPingProbe()
{
    if (legacy_ping_timer_) {
        legacy_ping_timer_->stop();
    }
}

void ClientBackend::setAutoConnectOnStartup(bool value)
{
    if (auto_connect_on_startup_ == value) {
        return;
    }
    auto_connect_on_startup_ = value;
    if (!value) {
        reconnect_timer_->stop();
        ops_metrics_stream_active_ = false;
        Q_EMIT serverOpsStreamChanged();
    } else {
        ops_metrics_subscribe_pending_ = true;
        if (legacyConnected()) {
            subscribeOpsMetrics();
        } else if (!manual_legacy_disconnect_) {
            scheduleAutoReconnect();
        }
    }
    Q_EMIT autoConnectOnStartupChanged();
}

void ClientBackend::saveSettings()
{
    speed::client::ClientSettings cfg;
    cfg.legacy_host = legacy_host_;
    cfg.rest_host = rest_host_;
    cfg.legacy_port = static_cast<quint16>(legacy_port_);
    cfg.rest_port = static_cast<quint16>(rest_port_);
    cfg.subscribe_topic = subscribe_topic_;
    cfg.echo_text = echo_text_;
    cfg.auto_connect_on_startup = auto_connect_on_startup_;
    cfg.save();
    appendLog(QStringLiteral("[配置] 已保存"));
    Q_EMIT toastRequested(QStringLiteral("配置已保存"), QStringLiteral("success"));
}

void ClientBackend::connectLegacy()
{
    manual_legacy_disconnect_ = false;
    legacy_->connectToServer();
    appendLog(QStringLiteral("[Legacy] 连接 %1").arg(legacyEndpoint()));
}

void ClientBackend::disconnectLegacy()
{
    manual_legacy_disconnect_ = true;
    reconnect_timer_->stop();
    stopLegacyPingProbe();
    legacy_->disconnectFromServer();
    appendLog(QStringLiteral("[Legacy] 已断开（已暂停自动重连，直至再次手动连接）"));
}

void ClientBackend::legacyPing()
{
    legacy_->sendLine(QStringLiteral("PING"));
}

void ClientBackend::legacyEcho()
{
    legacy_->sendLine(
        QStringLiteral("REQ echo echo %1").arg(echo_text_.trimmed().isEmpty() ? QStringLiteral("world")
                                                                              : echo_text_));
}

void ClientBackend::legacySubscribe()
{
    legacy_subscription_active_ = true;
    Q_EMIT legacySubscriptionActiveChanged();
    sampleStatusPoint();
    legacy_->sendLine(QStringLiteral("SUB %1").arg(subscribe_topic_));
    if (push_poll_enabled_) {
        legacy_->setPushPollEnabled(true);
    }
}

void ClientBackend::legacyUnsubscribe()
{
    legacy_subscription_active_ = false;
    Q_EMIT legacySubscriptionActiveChanged();
    sampleStatusPoint();
    legacy_->sendLine(QStringLiteral("UNSUB %1").arg(subscribe_topic_));
}

void ClientBackend::restHealth()
{
    rest_->fetchHealth();
}

void ClientBackend::restEcho()
{
    rest_->fetchEcho(echo_text_);
}

void ClientBackend::clearLog()
{
    event_log_.clear();
    Q_EMIT eventLogChanged();
}

void ClientBackend::connectOnStartup()
{
    if (!auto_connect_on_startup_) {
        if (!startup_connect_done_) {
            startup_connect_done_ = true;
            appendLog(QStringLiteral("[启动] 自动连接已关闭（可在连接页开启）"));
        }
        return;
    }
    if (startup_connect_done_) {
        return;
    }
    startup_connect_done_ = true;
    runAutoConnect(QStringLiteral("启动"));
}

void ClientBackend::runAutoConnect(const QString &reason)
{
    appendLog(QStringLiteral("[%1] Legacy → %2 · REST → %3")
                  .arg(reason, legacyEndpoint(), restEndpoint()));
    manual_legacy_disconnect_ = false;
    ops_metrics_subscribe_pending_ = auto_connect_on_startup_;
    // 首屏快照走 REST，与 Legacy TCP 并行；先发出 HTTP 避免与握手抢同一帧
    fetchServerSnapshot();
    restHealth();
    legacy_->connectToServer();
}

void ClientBackend::scheduleAutoReconnect(int delayMs)
{
    if (!auto_connect_on_startup_ || manual_legacy_disconnect_ || legacyConnected()) {
        return;
    }
    const int delay = delayMs >= 0 ? delayMs : reconnect_backoff_ms_;
    reconnect_timer_->setInterval(qMax(0, delay));
    if (!reconnect_timer_->isActive()) {
        reconnect_timer_->start();
    }
}

void ClientBackend::runLegacyReconnect(const QString &reason)
{
    appendLog(QStringLiteral("[%1] Legacy → %2").arg(reason, legacyEndpoint()));
    manual_legacy_disconnect_ = false;
    legacy_->connectToServer();
}

void ClientBackend::restoreLegacySessionAfterConnect()
{
    restoreOpsMetricsSubscription();
    if (!legacy_subscription_active_) {
        return;
    }
    legacy_->sendLine(QStringLiteral("SUB %1").arg(subscribe_topic_));
    if (push_poll_enabled_) {
        legacy_->setPushPollEnabled(true);
    }
    appendLog(QStringLiteral("[Legacy] 重连后恢复订阅 %1").arg(subscribe_topic_));
}

void ClientBackend::subscribeOpsMetrics()
{
    if (!auto_connect_on_startup_ || !legacyConnected() || !ops_metrics_subscribe_pending_) {
        return;
    }
    legacy_->sendLine(QStringLiteral("SUB %1").arg(kOpsMetricsTopic));
    ops_metrics_subscribe_pending_ = false;
    ops_metrics_stream_active_ = true;
    appendLog(QStringLiteral("[Legacy] 已订阅 %1（流式直推）").arg(kOpsMetricsTopic));
    Q_EMIT serverOpsStreamChanged();
}

void ClientBackend::restoreOpsMetricsSubscription()
{
    if (auto_connect_on_startup_) {
        ops_metrics_subscribe_pending_ = true;
        subscribeOpsMetrics();
    }
}

void ClientBackend::applyCpolarDefaults()
{
    const speed::client::ClientSettings cfg = speed::client::ClientSettings::productionDefaults();
    setLegacyHost(cfg.legacy_host);
    setRestHost(cfg.rest_host);
    setLegacyPort(cfg.legacy_port);
    setRestPort(cfg.rest_port);
    saveSettings();
    appendLog(QStringLiteral("[配置] 已恢复生产默认端点（%1）").arg(cfg.legacy_host));
    Q_EMIT toastRequested(
        QStringLiteral("已恢复默认：%1（Legacy %2 / REST %3）")
            .arg(cfg.legacy_host)
            .arg(cfg.legacy_port)
            .arg(cfg.rest_port),
        QStringLiteral("success"));
    runAutoConnect(QStringLiteral("生产默认"));
}

void ClientBackend::appendLog(const QString &line)
{
    const QString stamped =
        QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss")) + QStringLiteral("  ") + line;
    event_log_.prepend(stamped);
    while (event_log_.size() > 200) {
        event_log_.removeLast();
    }
    Q_EMIT eventLogChanged();
}

void ClientBackend::applyEndpoints()
{
    legacy_->setEndpoint(legacy_host_, static_cast<quint16>(legacy_port_));
    rest_->setEndpoint(rest_host_, static_cast<quint16>(rest_port_));
}

QStringList ClientBackend::statusChartCategories() const
{
    const int n = plot_time_labels_.size();
    QStringList out;
    if (n <= 0) {
        return out;
    }
    out.reserve(n);
    const int labelSlots = qMax(2, kStatusAxisLabelCount);
    const int stride = qMax(1, (n - 1) / (labelSlots - 1));
    for (int i = 0; i < n; ++i) {
        if (i == 0 || i == n - 1 || (i % stride) == 0) {
            out.append(shortTimeLabel(plot_time_labels_.at(i)));
        } else {
            out.append(QStringLiteral("\u2009"));
        }
    }
    return out;
}

QVariantList ClientBackend::statusChartSeries() const
{
    return status_chart_series_;
}

int ClientBackend::statusHistorySeconds() const
{
    return (kStatusHistoryMax * kStatusSampleIntervalMs) / 1000;
}

void ClientBackend::clearStatusHistory()
{
    status_time_labels_.clear();
    legacy_conn_history_.clear();
    legacy_busy_history_.clear();
    rest_ok_history_.clear();
    push_poll_history_.clear();
    subscription_history_.clear();
    plot_time_labels_.clear();
    plot_legacy_conn_.clear();
    plot_legacy_busy_.clear();
    plot_rest_ok_.clear();
    plot_push_poll_.clear();
    plot_subscription_.clear();
    rebuildStatusChartSeries();
    Q_EMIT statusChartChanged();
    sampleStatusPoint();
}

bool ClientBackend::restHealthOk() const
{
    static const QRegularExpression re(QStringLiteral("^HTTP\\s+(\\d+)"));
    const QRegularExpressionMatch match = re.match(last_rest_health_);
    if (!match.hasMatch()) {
        return false;
    }
    const int code = match.captured(1).toInt();
    return code >= 200 && code < 300;
}

void ClientBackend::sampleStatusPoint()
{
    const QString label = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss"));
    status_time_labels_.append(label);
    legacy_conn_history_.append(legacyConnected() ? 1.0 : 0.0);
    legacy_busy_history_.append(legacyConnected() && legacyBusy() ? 1.0 : 0.0);
    rest_ok_history_.append(restHealthOk() ? 1.0 : 0.0);
    push_poll_history_.append(push_poll_enabled_ ? 1.0 : 0.0);
    subscription_history_.append(legacy_subscription_active_ ? 1.0 : 0.0);

    while (status_time_labels_.size() > kStatusHistoryMax) {
        status_time_labels_.removeFirst();
        legacy_conn_history_.removeFirst();
        legacy_busy_history_.removeFirst();
        rest_ok_history_.removeFirst();
        push_poll_history_.removeFirst();
        subscription_history_.removeFirst();
    }

    rebuildStatusChartSeries();
    Q_EMIT statusChartChanged();
}

void ClientBackend::rebuildStatusChartSeries()
{
    const int count = status_time_labels_.size();
    const QVector<QVector<double> > channels = {
        legacy_conn_history_,
        legacy_busy_history_,
        rest_ok_history_,
        subscription_history_,
        push_poll_history_,
    };
    const QList<int> pick = buildPlotIndices(count, kStatusPlotMaxPoints, channels);

    plot_time_labels_.clear();
    plot_legacy_conn_.clear();
    plot_legacy_busy_.clear();
    plot_rest_ok_.clear();
    plot_push_poll_.clear();
    plot_subscription_.clear();
    plot_time_labels_.reserve(pick.size());
    plot_legacy_conn_.reserve(pick.size());
    plot_legacy_busy_.reserve(pick.size());
    plot_rest_ok_.reserve(pick.size());
    plot_push_poll_.reserve(pick.size());
    plot_subscription_.reserve(pick.size());

    for (int index : pick) {
        plot_time_labels_.append(status_time_labels_.at(index));
        plot_legacy_conn_.append(legacy_conn_history_.at(index));
        plot_legacy_busy_.append(legacy_busy_history_.at(index));
        plot_rest_ok_.append(rest_ok_history_.at(index));
        plot_push_poll_.append(push_poll_history_.at(index));
        plot_subscription_.append(subscription_history_.at(index));
    }

    auto toVariantList = [](const QVector<double> &values) {
        QVariantList list;
        list.reserve(values.size());
        for (double v : values) {
            list.append(v * 100.0);
        }
        return list;
    };

    status_chart_series_ = QVariantList{
        QVariantMap{
            {QStringLiteral("label"), QStringLiteral("Legacy TCP")},
            {QStringLiteral("values"), toVariantList(plot_legacy_conn_)},
            {QStringLiteral("color"), QStringLiteral("chart-1")},
        },
        QVariantMap{
            {QStringLiteral("label"), QStringLiteral("Legacy 传输")},
            {QStringLiteral("values"), toVariantList(plot_legacy_busy_)},
            {QStringLiteral("color"), QStringLiteral("chart-2")},
        },
        QVariantMap{
            {QStringLiteral("label"), QStringLiteral("REST /health")},
            {QStringLiteral("values"), toVariantList(plot_rest_ok_)},
            {QStringLiteral("color"), QStringLiteral("chart-3")},
        },
        QVariantMap{
            {QStringLiteral("label"), QStringLiteral("订阅意图")},
            {QStringLiteral("values"), toVariantList(plot_subscription_)},
            {QStringLiteral("color"), QStringLiteral("chart-4")},
        },
        QVariantMap{
            {QStringLiteral("label"), QStringLiteral("轮询 PING")},
            {QStringLiteral("values"), toVariantList(plot_push_poll_)},
            {QStringLiteral("color"), QStringLiteral("chart-5")},
        },
    };
}

bool ClientBackend::serverSnapshotBusy() const
{
    return rest_->isSnapshotBusy();
}

bool ClientBackend::serverSnapshotReady() const
{
    return server_snapshot_ready_;
}

QString ClientBackend::serverSnapshotError() const
{
    return server_snapshot_error_;
}

QString ClientBackend::serverSnapshotUpdatedLabel() const
{
    return server_snapshot_updated_label_;
}

QString ClientBackend::serverAppName() const
{
    return server_app_name_;
}

QString ClientBackend::serverPlatformLabel() const
{
    return server_platform_label_;
}

QVariantList ClientBackend::serverOpsTiles() const
{
    return server_ops_tiles_;
}

double ClientBackend::serverMemUsedPercent() const
{
    return server_mem_used_percent_;
}

double ClientBackend::serverCpuPercent() const
{
    return server_cpu_percent_;
}

QString ClientBackend::serverHostMemDetail() const
{
    return server_host_mem_detail_;
}

QString ClientBackend::serverProcessRssDetail() const
{
    return server_process_rss_detail_;
}

double ClientBackend::serverRootDiskUsedPercent() const
{
    return server_root_disk_used_percent_;
}

QString ClientBackend::serverRootDiskDetail() const
{
    return server_root_disk_detail_;
}

QString ClientBackend::serverRootDiskMount() const
{
    return server_root_disk_mount_;
}

bool ClientBackend::serverHasRootDisk() const
{
    return server_has_root_disk_;
}

double ClientBackend::serverDataDiskUsedPercent() const
{
    return server_data_disk_used_percent_;
}

QString ClientBackend::serverDataDiskDetail() const
{
    return server_data_disk_detail_;
}

QString ClientBackend::serverDataDiskMount() const
{
    return server_data_disk_mount_;
}

bool ClientBackend::serverHasDataDisk() const
{
    return server_has_data_disk_;
}

void ClientBackend::fetchServerSnapshot()
{
    if (rest_->isSnapshotBusy()) {
        return;
    }
    rest_->fetchOpsSnapshot();
}

QVariant ClientBackend::metricValue(const QJsonArray &metrics, const QString &id)
{
    for (const QJsonValue &item : metrics) {
        const QJsonObject obj = item.toObject();
        if (obj.value(QStringLiteral("id")).toString() == id) {
            return obj.value(QStringLiteral("v")).toVariant();
        }
    }
    return QVariant();
}

QString ClientBackend::formatBytes(qint64 bytes)
{
    if (bytes < 0) {
        return QStringLiteral("—");
    }
    const double b = static_cast<double>(bytes);
    if (b >= 1e9) {
        return QStringLiteral("%1 GB").arg(b / 1e9, 0, 'f', 2);
    }
    if (b >= 1e6) {
        return QStringLiteral("%1 MB").arg(b / 1e6, 0, 'f', 1);
    }
    if (b >= 1e3) {
        return QStringLiteral("%1 KB").arg(b / 1e3, 0, 'f', 0);
    }
    return QStringLiteral("%1 B").arg(bytes);
}

QString ClientBackend::formatUsedTotalBytes(qint64 usedBytes, qint64 totalBytes)
{
    if (usedBytes < 0 || totalBytes <= 0) {
        return QStringLiteral("—");
    }
    return QStringLiteral("%1 / %2").arg(formatBytes(usedBytes), formatBytes(totalBytes));
}

double ClientBackend::usedPercentFromBytes(qint64 availBytes, qint64 totalBytes)
{
    if (totalBytes <= 0 || availBytes < 0) {
        return kInvalidMetric;
    }
    const qint64 used = qMax<qint64>(0, totalBytes - availBytes);
    return qBound(0.0, static_cast<double>(used) * 100.0 / static_cast<double>(totalBytes), 100.0);
}

QString ClientBackend::formatDurationSec(double seconds)
{
    if (seconds < 0 || qIsNaN(seconds)) {
        return QStringLiteral("—");
    }
    const qint64 total = static_cast<qint64>(seconds);
    const qint64 days = total / 86400;
    const qint64 hours = (total % 86400) / 3600;
    const qint64 mins = (total % 3600) / 60;
    if (days > 0) {
        return QStringLiteral("%1天 %2小时").arg(days).arg(hours);
    }
    if (hours > 0) {
        return QStringLiteral("%1小时 %2分").arg(hours).arg(mins);
    }
    if (mins > 0) {
        return QStringLiteral("%1分 %2秒").arg(mins).arg(total % 60);
    }
    return QStringLiteral("%1 秒").arg(total);
}

void ClientBackend::parseOpsSnapshotPayload(const QString &body, int httpStatus)
{
    if (httpStatus < 200 || httpStatus >= 300 || body.isEmpty()) {
        server_snapshot_ready_ = false;
        server_snapshot_error_ = httpStatus > 0
                                     ? QStringLiteral("HTTP %1").arg(httpStatus)
                                     : QStringLiteral("无法获取 ops snapshot");
        appendLog(QStringLiteral("[REST /api/v1/ops/snapshot] %1").arg(server_snapshot_error_));
        Q_EMIT serverSnapshotChanged();
        return;
    }
    applyOpsSnapshotJson(body, QStringLiteral("REST"));
}

void ClientBackend::applyOpsSnapshotJson(const QString &body, const QString &source)
{
    if (body.isEmpty()) {
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(body.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (source != QStringLiteral("PUSH")) {
            server_snapshot_ready_ = false;
            server_snapshot_error_ = QStringLiteral("JSON 解析失败");
            Q_EMIT serverSnapshotChanged();
        }
        return;
    }

    const QJsonObject root = doc.object();
    const QJsonArray metrics = root.value(QStringLiteral("metrics")).toArray();
    server_snapshot_ts_ms_ = root.value(QStringLiteral("ts_ms")).toVariant().toLongLong();

    server_app_name_ = metricValue(metrics, QStringLiteral("meta.app_name")).toString();
    if (server_app_name_.isEmpty()) {
        server_app_name_ = QStringLiteral("speed_server");
    }
    const QString os = metricValue(metrics, QStringLiteral("meta.platform.os")).toString();
    const QString arch = metricValue(metrics, QStringLiteral("meta.platform.arch")).toString();
    server_platform_label_ = os.isEmpty() ? arch : QStringLiteral("%1 · %2").arg(os, arch);

    const QVariant cpuVar = metricValue(metrics, QStringLiteral("host.cpu.utilization_pct"));
    if (cpuVar.isNull() || !cpuVar.isValid()) {
        server_cpu_percent_ = kInvalidMetric;
    } else {
        server_cpu_percent_ = qBound(0.0, cpuVar.toDouble(), 100.0);
    }

    const qint64 memTotal =
        metricValue(metrics, QStringLiteral("host.mem.total_bytes")).toLongLong();
    const qint64 memAvail =
        metricValue(metrics, QStringLiteral("host.mem.available_bytes")).toLongLong();
    if (memTotal > 0 && memAvail >= 0) {
        server_mem_used_percent_ = usedPercentFromBytes(memAvail, memTotal);
        const qint64 memUsed = qMax<qint64>(0, memTotal - memAvail);
        server_host_mem_detail_ =
            QStringLiteral("已用 %1 / 总计 %2 · 可用 %3")
                .arg(formatBytes(memUsed), formatBytes(memTotal), formatBytes(memAvail));
    } else {
        server_mem_used_percent_ = kInvalidMetric;
        server_host_mem_detail_.clear();
    }

    const qint64 procRss =
        metricValue(metrics, QStringLiteral("process.rss_bytes")).toLongLong();
    server_process_rss_detail_ = procRss > 0 ? formatBytes(procRss) : QString();

    auto applyDisk = [&](const QString &prefix, double &pct, QString &detail, QString &mount,
                         bool &has) {
        const qint64 total =
            metricValue(metrics, prefix + QStringLiteral(".total_bytes")).toLongLong();
        const qint64 avail =
            metricValue(metrics, prefix + QStringLiteral(".available_bytes")).toLongLong();
        const qint64 used =
            metricValue(metrics, prefix + QStringLiteral(".used_bytes")).toLongLong();
        mount = metricValue(metrics, prefix + QStringLiteral(".mount_path")).toString();
        const QVariant pctVar = metricValue(metrics, prefix + QStringLiteral(".utilization_pct"));
        if (total <= 0) {
            has = false;
            pct = kInvalidMetric;
            detail.clear();
            return;
        }
        has = true;
        if (pctVar.isValid() && !pctVar.isNull()) {
            pct = pctVar.toDouble();
        } else if (avail >= 0) {
            pct = usedPercentFromBytes(avail, total);
        } else {
            pct = kInvalidMetric;
        }
        const qint64 usedBytes = used >= 0 ? used : qMax<qint64>(0, total - qMax(avail, qint64(0)));
        detail = QStringLiteral("%1 · 可用 %2")
                     .arg(formatUsedTotalBytes(usedBytes, total),
                          avail >= 0 ? formatBytes(avail) : QStringLiteral("—"));
    };
    applyDisk(QStringLiteral("host.disk.system"), server_root_disk_used_percent_,
              server_root_disk_detail_, server_root_disk_mount_, server_has_root_disk_);
    applyDisk(QStringLiteral("host.disk.data"), server_data_disk_used_percent_,
              server_data_disk_detail_, server_data_disk_mount_, server_has_data_disk_);

    auto metricNumber = [&](const QString &id) -> QString {
        const QVariant v = metricValue(metrics, id);
        if (!v.isValid() || v.isNull()) {
            return QStringLiteral("—");
        }
        const double n = v.toDouble();
        const bool countLike = id.startsWith(QStringLiteral("framework."))
                               && (id.endsWith(QStringLiteral("_total"))
                                   || id == QStringLiteral("framework.connections.active"));
        if (countLike) {
            return formatCountMetric(n);
        }
        return QString::number(n, 'f', n >= 10 ? 0 : 1);
    };

    server_ops_tiles_.clear();
    auto pushTile = [this](const QString &icon, const QString &title, const QString &value,
                           const QString &hint) {
        server_ops_tiles_.append(QVariantMap{
            {QStringLiteral("icon"), icon},
            {QStringLiteral("title"), title},
            {QStringLiteral("value"), value},
            {QStringLiteral("hint"), hint},
        });
    };

    pushTile(QStringLiteral("users"), QStringLiteral("活跃连接"),
             metricNumber(QStringLiteral("framework.connections.active")),
             QStringLiteral("实时 TCP / HTTP 会话"));
    pushTile(QStringLiteral("list-checks"), QStringLiteral("累计请求"),
             metricNumber(QStringLiteral("framework.requests.total")),
             QStringLiteral("自进程启动"));
    pushTile(QStringLiteral("radio-tower"), QStringLiteral("事件发布"),
             metricNumber(QStringLiteral("framework.events.published_total")),
             QStringLiteral("含 ops.host.metrics 推送"));

    updateDashboardTotalsFromMetrics(metrics);
    pushTile(QStringLiteral("refresh-cw"), QStringLiteral("配置热加载"),
             metricNumber(QStringLiteral("framework.config.reloads_total")),
             QStringLiteral("reload 次数"));

    pushTile(QStringLiteral("timer"), QStringLiteral("进程运行"),
             formatDurationSec(metricValue(metrics, QStringLiteral("process.uptime_sec")).toDouble()),
             QStringLiteral("Ops 服务 uptime"));
    const QVariant loadVar = metricValue(metrics, QStringLiteral("host.loadavg.1m"));
    if (loadVar.isValid() && !loadVar.isNull()) {
        pushTile(QStringLiteral("gauge"), QStringLiteral("负载 1m"),
                 QString::number(loadVar.toDouble(), 'f', 2), QStringLiteral("仅 Linux"));
    }

    if (server_snapshot_ts_ms_ > 0) {
        const QDateTime when = QDateTime::fromMSecsSinceEpoch(server_snapshot_ts_ms_, Qt::UTC).toLocalTime();
        server_snapshot_updated_label_ =
            when.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    } else {
        server_snapshot_updated_label_ = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss"));
    }

    server_snapshot_ready_ = true;
    server_snapshot_error_.clear();
    if (source == QStringLiteral("PUSH")) {
        if (!ops_metrics_stream_active_) {
            ops_metrics_stream_active_ = true;
            Q_EMIT serverOpsStreamChanged();
        }
    } else {
        appendLog(QStringLiteral("[%1 snapshot] %2 指标").arg(source).arg(server_ops_tiles_.size()));
    }
    Q_EMIT serverSnapshotChanged();
}

QString ClientBackend::dashboardRequestsTotal() const
{
    return dashboard_requests_total_;
}

QString ClientBackend::dashboardEventsTotal() const
{
    return dashboard_events_total_;
}

QVariantList ClientBackend::opsHeatmapDays() const
{
    return ops_heatmap_days_;
}

bool ClientBackend::opsDashboardBusy() const
{
    return ops_dashboard_refresh_pending_ || rest_->isHistoryBusy();
}

QString ClientBackend::opsDashboardError() const
{
    return ops_dashboard_error_;
}

bool ClientBackend::opsHeatmapReady() const
{
    return ops_heatmap_ready_;
}

void ClientBackend::refreshOpsDashboard()
{
    ops_dashboard_error_.clear();
    ops_dashboard_refresh_pending_ = true;
    Q_EMIT opsDashboardChanged();

    if (!server_snapshot_ready_ && !serverSnapshotBusy()) {
        fetchServerSnapshot();
    }
    rest_->fetchOpsHistoryHeatmap(QDate::currentDate().year());
}

void ClientBackend::updateDashboardTotalsFromMetrics(const QJsonArray &metrics)
{
    auto countFromMetric = [&](const QString &id) -> QString {
        const QVariant v = metricValue(metrics, id);
        if (!v.isValid() || v.isNull()) {
            return QStringLiteral("—");
        }
        return formatCountMetric(v.toDouble());
    };

    dashboard_requests_total_ = countFromMetric(QStringLiteral("framework.requests.total"));
    dashboard_events_total_ =
        countFromMetric(QStringLiteral("framework.events.published_total"));
    Q_EMIT opsDashboardChanged();
}

void ClientBackend::parseOpsHeatmapPayload(const QString &body, int httpStatus)
{
    ops_dashboard_refresh_pending_ = false;

    if (httpStatus < 200 || httpStatus >= 300 || body.isEmpty()) {
        ops_heatmap_ready_ = false;
        ops_dashboard_error_ = httpStatus == 503
                                   ? QStringLiteral("服务端未开启 ops.history")
                                   : (httpStatus > 0 ? QStringLiteral("HTTP %1").arg(httpStatus)
                                                     : QStringLiteral("无法读取热力图数据"));
        appendLog(QStringLiteral("[REST /api/v1/ops/history/heatmap] %1").arg(ops_dashboard_error_));
        Q_EMIT opsDashboardChanged();
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(body.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        ops_heatmap_ready_ = false;
        ops_dashboard_error_ = QStringLiteral("热力图 JSON 解析失败");
        Q_EMIT opsDashboardChanged();
        return;
    }

    const QJsonArray days = doc.object().value(QStringLiteral("days")).toArray();
    QVariantList parsed;
    parsed.reserve(days.size());
    for (const QJsonValue &item : days) {
        const QJsonObject row = item.toObject();
        const QString date = row.value(QStringLiteral("date")).toString();
        if (date.isEmpty()) {
            continue;
        }
        parsed.append(QVariantMap{
            {QStringLiteral("date"), date},
            {QStringLiteral("requests"),
             row.value(QStringLiteral("requests")).toVariant().toULongLong()},
            {QStringLiteral("events_published"),
             row.value(QStringLiteral("events_published")).toVariant().toULongLong()},
        });
    }

    std::sort(parsed.begin(), parsed.end(), [](const QVariant &a, const QVariant &b) {
        return a.toMap().value(QStringLiteral("date")).toString()
               < b.toMap().value(QStringLiteral("date")).toString();
    });

    ops_heatmap_days_ = parsed;
    ops_heatmap_ready_ = true;
    ops_dashboard_error_.clear();
    appendLog(QStringLiteral("[REST heatmap] %1 天").arg(ops_heatmap_days_.size()));
    Q_EMIT opsDashboardChanged();
}

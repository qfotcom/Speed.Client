#include "ClientBackend.h"

#include <QDateTime>
#include <QRegularExpression>
#include <QTimer>
#include <QVariantMap>

#include "config/client_settings.hpp"
#include "legacy/legacy_client.hpp"
#include "rest/rest_client.hpp"

#include <QSet>

#include <algorithm>

namespace {

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
            restoreLegacySessionAfterConnect();
            return;
        }
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
                appendLog(QStringLiteral("[Legacy] %1").arg(raw));
            });
    connect(legacy_, &speed::client::legacy::LegacyClient::pushReceived, this,
            [this](const QString &topic, const QString &payload) {
                appendLog(QStringLiteral("[PUSH] %1 %2").arg(topic, payload));
            });

    connect(rest_, &speed::client::rest::RestClient::busyChanged, this,
            &ClientBackend::restBusyChanged);
    connect(rest_, &speed::client::rest::RestClient::errorOccurred, this,
            [this](const QString &msg) {
                appendLog(QStringLiteral("[REST ERR] %1").arg(msg));
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

    status_sample_timer_ = new QTimer(this);
    status_sample_timer_->setInterval(kStatusSampleIntervalMs);
    connect(status_sample_timer_, &QTimer::timeout, this, &ClientBackend::sampleStatusPoint);
    status_sample_timer_->start();
    sampleStatusPoint();

    // QML 就绪后再连（Main.qml 也会调一次 connectOnStartup）
    QTimer::singleShot(600, this, &ClientBackend::connectOnStartup);
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

void ClientBackend::setAutoConnectOnStartup(bool value)
{
    if (auto_connect_on_startup_ == value) {
        return;
    }
    auto_connect_on_startup_ = value;
    if (!value) {
        reconnect_timer_->stop();
    } else if (!legacyConnected() && !manual_legacy_disconnect_) {
        scheduleAutoReconnect();
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
    legacy_->connectToServer();
    restHealth();
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
    if (!legacy_subscription_active_) {
        return;
    }
    legacy_->sendLine(QStringLiteral("SUB %1").arg(subscribe_topic_));
    if (push_poll_enabled_) {
        legacy_->setPushPollEnabled(true);
    }
    appendLog(QStringLiteral("[Legacy] 重连后恢复订阅 %1").arg(subscribe_topic_));
}

void ClientBackend::applyCpolarDefaults()
{
    const speed::client::ClientSettings cfg = speed::client::ClientSettings::cpolarDefaults();
    setLegacyHost(cfg.legacy_host);
    setRestHost(cfg.rest_host);
    setLegacyPort(cfg.legacy_port);
    setRestPort(cfg.rest_port);
    saveSettings();
    appendLog(QStringLiteral("[配置] 已恢复 cpolar 默认双通道"));
    Q_EMIT toastRequested(QStringLiteral("已恢复 cpolar 默认（Legacy / REST 独立主机）"),
                          QStringLiteral("success"));
    runAutoConnect(QStringLiteral("cpolar 默认"));
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

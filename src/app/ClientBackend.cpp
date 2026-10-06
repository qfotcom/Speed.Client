#include "ClientBackend.h"

#include <QDateTime>
#include <QTimer>

#include "config/client_settings.hpp"
#include "legacy/legacy_client.hpp"
#include "rest/rest_client.hpp"

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
                appendLog(QStringLiteral("[REST /health] %1").arg(last_rest_health_));
            });
    connect(rest_, &speed::client::rest::RestClient::echoReceived, this,
            [this](const QString &body, int status) {
                last_rest_echo_ = QStringLiteral("HTTP %1: %2").arg(status).arg(body);
                Q_EMIT lastRestEchoChanged();
                appendLog(QStringLiteral("[REST echo] %1").arg(last_rest_echo_));
            });

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
    legacy_->sendLine(QStringLiteral("SUB %1").arg(subscribe_topic_));
    if (push_poll_enabled_) {
        legacy_->setPushPollEnabled(true);
    }
}

void ClientBackend::legacyUnsubscribe()
{
    legacy_subscription_active_ = false;
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

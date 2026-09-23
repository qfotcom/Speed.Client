#include "ClientBackend.h"

#include <QDateTime>

#include "config/client_settings.hpp"
#include "legacy/legacy_client.hpp"
#include "rest/rest_client.hpp"

ClientBackend::ClientBackend(QObject *parent)
    : QObject(parent)
    , legacy_(new speed::client::legacy::LegacyClient(this))
    , rest_(new speed::client::rest::RestClient(this))
{
    const speed::client::ClientSettings cfg = speed::client::ClientSettings::load();
    host_ = cfg.host;
    legacy_port_ = cfg.legacy_port;
    rest_port_ = cfg.rest_port;
    subscribe_topic_ = cfg.subscribe_topic;
    echo_text_ = cfg.echo_text;
    applyEndpoints();

    connect(legacy_, &speed::client::legacy::LegacyClient::connectedChanged, this,
            &ClientBackend::legacyConnectedChanged);
    connect(legacy_, &speed::client::legacy::LegacyClient::busyChanged, this,
            &ClientBackend::legacyBusyChanged);
    connect(legacy_, &speed::client::legacy::LegacyClient::errorOccurred, this,
            [this](const QString &msg) {
                appendLog(QStringLiteral("[Legacy ERR] %1").arg(msg));
                Q_EMIT toastRequested(msg, QStringLiteral("error"));
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
}

ClientBackend::~ClientBackend() = default;

QString ClientBackend::host() const
{
    return host_;
}

int ClientBackend::legacyPort() const
{
    return legacy_port_;
}

int ClientBackend::restPort() const
{
    return rest_port_;
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

void ClientBackend::setHost(const QString &value)
{
    if (host_ == value) {
        return;
    }
    host_ = value;
    applyEndpoints();
    Q_EMIT hostChanged();
}

void ClientBackend::setLegacyPort(int value)
{
    if (legacy_port_ == value) {
        return;
    }
    legacy_port_ = value;
    applyEndpoints();
    Q_EMIT legacyPortChanged();
}

void ClientBackend::setRestPort(int value)
{
    if (rest_port_ == value) {
        return;
    }
    rest_port_ = value;
    applyEndpoints();
    Q_EMIT restPortChanged();
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

void ClientBackend::saveSettings()
{
    speed::client::ClientSettings cfg;
    cfg.host = host_;
    cfg.legacy_port = static_cast<quint16>(legacy_port_);
    cfg.rest_port = static_cast<quint16>(rest_port_);
    cfg.subscribe_topic = subscribe_topic_;
    cfg.echo_text = echo_text_;
    cfg.save();
    appendLog(QStringLiteral("[配置] 已保存"));
    Q_EMIT toastRequested(QStringLiteral("配置已保存"), QStringLiteral("success"));
}

void ClientBackend::connectLegacy()
{
    legacy_->connectToServer();
    appendLog(QStringLiteral("[Legacy] 连接 %1:%2").arg(host_).arg(legacy_port_));
}

void ClientBackend::disconnectLegacy()
{
    legacy_->disconnectFromServer();
    appendLog(QStringLiteral("[Legacy] 已断开"));
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
    legacy_->sendLine(QStringLiteral("SUB %1").arg(subscribe_topic_));
    if (push_poll_enabled_) {
        legacy_->setPushPollEnabled(true);
    }
}

void ClientBackend::legacyUnsubscribe()
{
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
    legacy_->setEndpoint(host_, static_cast<quint16>(legacy_port_));
    rest_->setEndpoint(host_, static_cast<quint16>(rest_port_));
}

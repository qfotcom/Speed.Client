#include "legacy/legacy_client.hpp"
#include "legacy/legacy_protocol.hpp"

#include <QElapsedTimer>
#include <QNetworkProxy>
#include <QTcpSocket>
#include <QTimer>
#include <QtEndian>

#include <cstring>

namespace speed::client::legacy {

LegacyClient::LegacyClient(QObject *parent)
    : QObject(parent)
    , socket_(new QTcpSocket(this))
    , poll_timer_(new QTimer(this))
    , ping_rtt_timer_(new QElapsedTimer())
{
    poll_timer_->setInterval(1500);
    connect(poll_timer_, &QTimer::timeout, this, &LegacyClient::onPollTick);

    connect(socket_, &QTcpSocket::connected, this, &LegacyClient::onConnected);
    connect(socket_, &QTcpSocket::disconnected, this, &LegacyClient::onDisconnected);
    connect(socket_, &QTcpSocket::readyRead, this, &LegacyClient::onReadyRead);
    connect(socket_, &QAbstractSocket::errorOccurred, this, &LegacyClient::onError);
}

LegacyClient::~LegacyClient()
{
    disconnectFromServer();
    delete ping_rtt_timer_;
    ping_rtt_timer_ = nullptr;
}

QString LegacyClient::host() const
{
    return host_;
}

quint16 LegacyClient::port() const
{
    return port_;
}

bool LegacyClient::isConnected() const
{
    return connected_;
}

bool LegacyClient::isBusy() const
{
    return busy_;
}

void LegacyClient::setEndpoint(const QString &host, quint16 port)
{
    if (host_ != host) {
        host_ = host;
        Q_EMIT hostChanged();
    }
    if (port_ != port) {
        port_ = port;
        Q_EMIT portChanged();
    }
}

void LegacyClient::connectToServer()
{
    if (connected_ || socket_->state() != QAbstractSocket::UnconnectedState) {
        socket_->abort();
    }
    setConnected(false);
    setBusy(false);
    outbound_lines_.clear();
    active_line_.clear();
    read_phase_ = ReadPhase::Idle;
    read_buffer_.clear();
    socket_->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    socket_->setProxy(QNetworkProxy::applicationProxy());
    socket_->connectToHost(host_, port_);
}

void LegacyClient::disconnectFromServer()
{
    poll_timer_->stop();
    if (socket_->state() != QAbstractSocket::UnconnectedState) {
        socket_->disconnectFromHost();
        if (socket_->state() != QAbstractSocket::UnconnectedState) {
            socket_->waitForDisconnected(500);
        }
    }
    socket_->abort();
    setConnected(false);
    setBusy(false);
    outbound_lines_.clear();
    active_line_.clear();
    read_phase_ = ReadPhase::Idle;
    read_buffer_.clear();
}

void LegacyClient::sendLine(const QString &line)
{
    if (!connected_) {
        Q_EMIT errorOccurred(QStringLiteral("Legacy 未连接"));
        return;
    }
    outbound_lines_.enqueue(line);
    flushWriteQueue();
}

void LegacyClient::setPushPollEnabled(bool enabled)
{
    user_push_poll_ = enabled;
    updatePollTimer();
}

void LegacyClient::setOpsPushPollEnabled(bool enabled)
{
    ops_push_poll_ = enabled;
    updatePollTimer();
}

void LegacyClient::updatePollTimer()
{
    if (!connected_) {
        poll_timer_->stop();
        return;
    }
    int intervalMs = 2000;
    if (ops_push_poll_) {
        intervalMs = 1000;
    } else if (user_push_poll_) {
        intervalMs = 1500;
    }
    poll_timer_->setInterval(intervalMs);
    if (!poll_timer_->isActive()) {
        poll_timer_->start();
    }
}

void LegacyClient::onConnected()
{
    socket_->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    setConnected(true);
    updatePollTimer();
    flushWriteQueue();
    QTimer::singleShot(200, this, [this]() {
        if (connected_ && !busy_) {
            sendLine(QStringLiteral("PING"));
        }
    });
}

void LegacyClient::onDisconnected()
{
    setConnected(false);
    setBusy(false);
    ping_rtt_waiting_ = false;
    poll_timer_->stop();
    outbound_lines_.clear();
    active_line_.clear();
    read_phase_ = ReadPhase::Idle;
    read_buffer_.clear();
}

void LegacyClient::onReadyRead()
{
    while (socket_->bytesAvailable() > 0) {
        if (read_phase_ == ReadPhase::Idle) {
            beginReadFrame();
        }
        if (read_phase_ == ReadPhase::Length) {
            if (read_buffer_.size() < 4) {
                read_buffer_.append(socket_->read(4 - read_buffer_.size()));
            }
            if (read_buffer_.size() < 4) {
                return;
            }
            quint32 be = 0;
            std::memcpy(&be, read_buffer_.constData(), 4);
            pending_body_length_ = qFromBigEndian(be);
            read_buffer_.clear();
            read_phase_ = ReadPhase::Body;
        }
        if (read_phase_ == ReadPhase::Body) {
            const int need = static_cast<int>(pending_body_length_) - read_buffer_.size();
            if (need > 0) {
                read_buffer_.append(socket_->read(need));
            }
            if (read_buffer_.size() < static_cast<int>(pending_body_length_)) {
                return;
            }
            const QByteArray bodyBytes = read_buffer_.left(static_cast<int>(pending_body_length_));
            finishFrame(bodyBytes);
            read_buffer_.clear();
            read_phase_ = ReadPhase::Idle;
        }
    }
}

void LegacyClient::onError()
{
    if (socket_->error() == QAbstractSocket::RemoteHostClosedError) {
        return;
    }
    Q_EMIT errorOccurred(socket_->errorString());
}

void LegacyClient::onPollTick()
{
    if (!connected_ || busy_) {
        return;
    }
    sendLine(QStringLiteral("PING"));
}

void LegacyClient::beginReadFrame()
{
    read_phase_ = ReadPhase::Length;
    read_buffer_.clear();
    pending_body_length_ = 0;
}

void LegacyClient::finishFrame(const QByteArray &body)
{
    const QString bodyText = QString::fromUtf8(body);
    const FrameParseResult parsed = parseFrameBody(bodyText);

    for (const QString &pushLine : parsed.push_lines) {
        if (!pushLine.startsWith(QStringLiteral("PUSH "))) {
            continue;
        }
        const int firstSpace = pushLine.indexOf(QLatin1Char(' '));
        const int secondSpace = pushLine.indexOf(QLatin1Char(' '), firstSpace + 1);
        if (secondSpace < 0) {
            continue;
        }
        const QString topic = pushLine.mid(firstSpace + 1, secondSpace - firstSpace - 1);
        const QString payload = pushLine.mid(secondSpace + 1);
        Q_EMIT pushReceived(topic, payload);
    }

    if (!parsed.primary_line.isEmpty()) {
        if (ping_rtt_waiting_ && parsed.primary_line == QStringLiteral("PONG")
            && ping_rtt_timer_->isValid()) {
            ping_rtt_waiting_ = false;
            Q_EMIT pingRttMeasured(static_cast<int>(ping_rtt_timer_->elapsed()));
        }
        Q_EMIT responseReceived(parsed.primary_line, bodyText);
        setBusy(false);
        active_line_.clear();
        flushWriteQueue();
    }
}

void LegacyClient::flushWriteQueue()
{
    if (!connected_ || busy_ || outbound_lines_.isEmpty()) {
        return;
    }
    active_line_ = outbound_lines_.dequeue();
    if (active_line_.trimmed() == QStringLiteral("PING")) {
        ping_rtt_waiting_ = true;
        ping_rtt_timer_->restart();
    }
    setBusy(true);
    const QByteArray frame = encodeFrame(active_line_);
    const qint64 written = socket_->write(frame);
    if (written != frame.size()) {
        setBusy(false);
        Q_EMIT errorOccurred(QStringLiteral("Legacy 写入失败"));
        return;
    }
    socket_->flush();
}

void LegacyClient::setConnected(bool value)
{
    if (connected_ == value) {
        return;
    }
    connected_ = value;
    Q_EMIT connectedChanged();
}

void LegacyClient::setBusy(bool value)
{
    if (busy_ == value) {
        return;
    }
    busy_ = value;
    Q_EMIT busyChanged();
}

} // namespace speed::client::legacy

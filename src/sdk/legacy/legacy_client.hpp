#pragma once

#include <QObject>
#include <QQueue>
#include <QString>

class QTcpSocket;
class QTimer;

namespace speed::client::legacy {

class LegacyClient final : public QObject
{
    Q_OBJECT

public:
    explicit LegacyClient(QObject *parent = nullptr);
    ~LegacyClient() override;

    QString host() const;
    quint16 port() const;
    bool isConnected() const;
    bool isBusy() const;

public Q_SLOTS:
    void setEndpoint(const QString &host, quint16 port);
    void connectToServer();
    void disconnectFromServer();
    void sendLine(const QString &line);
    void setPushPollEnabled(bool enabled);

Q_SIGNALS:
    void hostChanged();
    void portChanged();
    void connectedChanged();
    void busyChanged();
    void errorOccurred(const QString &message);
    void pushReceived(const QString &topic, const QString &payload);
    void responseReceived(const QString &primaryLine, const QString &rawBody);

private Q_SLOTS:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onError();
    void onPollTick();

private:
    void beginReadFrame();
    void finishFrame(const QByteArray &body);
    void flushWriteQueue();
    void setConnected(bool value);
    void setBusy(bool value);

    QTcpSocket *socket_{nullptr};
    QTimer *poll_timer_{nullptr};
    QString host_{QStringLiteral("127.0.0.1")};
    quint16 port_{9001};
    bool connected_{false};
    bool busy_{false};
    enum class ReadPhase { Idle, Length, Body };
    ReadPhase read_phase_{ReadPhase::Idle};
    QByteArray read_buffer_;
    quint32 pending_body_length_{0};
    QQueue<QString> outbound_lines_;
    QString active_line_;
};

} // namespace speed::client::legacy

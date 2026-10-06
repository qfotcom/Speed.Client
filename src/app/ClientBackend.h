#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

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
    Q_PROPERTY(QString lastRestHealth READ lastRestHealth NOTIFY lastRestHealthChanged)
    Q_PROPERTY(QString lastRestEcho READ lastRestEcho NOTIFY lastRestEchoChanged)
    Q_PROPERTY(QString lastLegacyLine READ lastLegacyLine NOTIFY lastLegacyLineChanged)
    Q_PROPERTY(QStringList eventLog READ eventLog NOTIFY eventLogChanged)

public:
    explicit ClientBackend(QObject *parent = nullptr);
    ~ClientBackend() override;

    QString legacyHost() const;
    QString restHost() const;
    QString legacyEndpoint() const;
    QString restEndpoint() const;
    int legacyPort() const;
    int restPort() const;
    bool autoConnectOnStartup() const;
    QString subscribeTopic() const;
    QString echoText() const;
    bool legacyConnected() const;
    bool legacyBusy() const;
    bool restBusy() const;
    bool pushPollEnabled() const;
    QString lastRestHealth() const;
    QString lastRestEcho() const;
    QString lastLegacyLine() const;
    QStringList eventLog() const;

    void setLegacyHost(const QString &value);
    void setRestHost(const QString &value);
    void setLegacyPort(int value);
    void setRestPort(int value);
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
    /** 恢复 cpolar 默认双 host + 双端口并保存 */
    Q_INVOKABLE void applyCpolarDefaults();

Q_SIGNALS:
    void legacyHostChanged();
    void restHostChanged();
    void legacyEndpointChanged();
    void restEndpointChanged();
    void legacyPortChanged();
    void restPortChanged();
    void autoConnectOnStartupChanged();
    void subscribeTopicChanged();
    void echoTextChanged();
    void legacyConnectedChanged();
    void legacyBusyChanged();
    void restBusyChanged();
    void pushPollEnabledChanged();
    void lastRestHealthChanged();
    void lastRestEchoChanged();
    void lastLegacyLineChanged();
    void eventLogChanged();
    void toastRequested(const QString &message, const QString &level);

private:
    void appendLog(const QString &line);
    void applyEndpoints();
    void scheduleAutoReconnect();
    void runAutoConnect(const QString &reason); // Legacy TCP + REST /health

    speed::client::legacy::LegacyClient *legacy_{nullptr};
    speed::client::rest::RestClient *rest_{nullptr};
    QTimer *reconnect_timer_{nullptr};

    QString legacy_host_;
    QString rest_host_;
    int legacy_port_{20771};
    int rest_port_{10513};
    bool auto_connect_on_startup_{true};
    bool manual_legacy_disconnect_{false};
    bool startup_connect_done_{false};
    QString subscribe_topic_;
    QString echo_text_;
    bool push_poll_enabled_{false};
    QString last_rest_health_;
    QString last_rest_echo_;
    QString last_legacy_line_;
    QStringList event_log_;
};

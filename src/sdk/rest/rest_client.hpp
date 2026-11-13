#pragma once

#include <QObject>
#include <QString>
#include <QUrl>

class QNetworkAccessManager;

namespace speed::client::rest {

class RestClient final : public QObject
{
    Q_OBJECT

public:
    explicit RestClient(QObject *parent = nullptr);

    QString host() const;
    quint16 port() const;
    bool isBusy() const;
    bool isSnapshotBusy() const;

public Q_SLOTS:
    void setEndpoint(const QString &host, quint16 port);
    void fetchHealth();
    void fetchEcho(const QString &text);
    void fetchOpsSnapshot();

Q_SIGNALS:
    void hostChanged();
    void portChanged();
    void busyChanged();
    void snapshotBusyChanged();
    void errorOccurred(const QString &message);
    void healthReceived(const QString &body, int httpStatus);
    void echoReceived(const QString &body, int httpStatus);
    void opsSnapshotReceived(const QString &body, int httpStatus);

private:
    QUrl buildUrl(const QString &path, const QUrlQuery &query) const;
    void setBusy(bool value);

    QNetworkAccessManager *nam_{nullptr};
    QString host_{QStringLiteral("127.0.0.1")};
    quint16 port_{8080};
    bool busy_{false};
    bool snapshot_busy_{false};
};

} // namespace speed::client::rest

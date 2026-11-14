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
    bool useTls() const;
    bool isBusy() const;
    bool isSnapshotBusy() const;
    bool isHistoryBusy() const;

public Q_SLOTS:
    void setEndpoint(const QString &host, quint16 port, bool useTls = false);
    void fetchHealth();
    void fetchEcho(const QString &text);
    void fetchOpsSnapshot();
    void fetchOpsHistoryDaily(int days = 7);
    void fetchOpsHistoryHeatmap(int year = 0);

Q_SIGNALS:
    void hostChanged();
    void portChanged();
    void useTlsChanged();
    void busyChanged();
    void snapshotBusyChanged();
    void historyBusyChanged();
    void errorOccurred(const QString &message);
    void healthReceived(const QString &body, int httpStatus);
    void echoReceived(const QString &body, int httpStatus);
    void opsSnapshotReceived(const QString &body, int httpStatus);
    void opsHistoryDailyReceived(const QString &body, int httpStatus);
    void opsHistoryHeatmapReceived(const QString &body, int httpStatus);

private:
    QUrl buildUrl(const QString &path, const QUrlQuery &query) const;
    void setBusy(bool value);
    void setHistoryBusy(bool value);

    QNetworkAccessManager *nam_{nullptr};
    QString host_{QStringLiteral("127.0.0.1")};
    quint16 port_{443};
    bool use_tls_{true};
    bool busy_{false};
    bool snapshot_busy_{false};
    int history_inflight_{0};
};

} // namespace speed::client::rest
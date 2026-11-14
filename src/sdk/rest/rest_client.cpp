#include "rest/rest_client.hpp"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrlQuery>

namespace speed::client::rest {

RestClient::RestClient(QObject *parent)
    : QObject(parent)
    , nam_(new QNetworkAccessManager(this))
{
}

QString RestClient::host() const
{
    return host_;
}

quint16 RestClient::port() const
{
    return port_;
}

bool RestClient::isBusy() const
{
    return busy_;
}

bool RestClient::isSnapshotBusy() const
{
    return snapshot_busy_;
}

bool RestClient::isHistoryBusy() const
{
    return history_inflight_ > 0;
}

void RestClient::setHistoryBusy(bool value)
{
    if (value) {
        ++history_inflight_;
    } else if (history_inflight_ > 0) {
        --history_inflight_;
    }
    Q_EMIT historyBusyChanged();
}

void RestClient::setEndpoint(const QString &host, quint16 port)
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

void RestClient::fetchHealth()
{
    if (busy_) {
        Q_EMIT errorOccurred(QStringLiteral("REST 请求进行中"));
        return;
    }
    setBusy(true);

    QNetworkRequest request(buildUrl(QStringLiteral("/health"), QUrlQuery()));
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("SpeedClient/0.1"));

    QNetworkReply *reply = nam_->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const int status =
            reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() != QNetworkReply::NoError) {
            Q_EMIT errorOccurred(reply->errorString());
            Q_EMIT healthReceived(QString(), status > 0 ? status : 0);
        } else {
            Q_EMIT healthReceived(QString::fromUtf8(reply->readAll()), status);
        }
        reply->deleteLater();
        setBusy(false);
    });
}

void RestClient::fetchOpsSnapshot()
{
    if (snapshot_busy_) {
        Q_EMIT errorOccurred(QStringLiteral("Ops snapshot 请求进行中"));
        return;
    }
    snapshot_busy_ = true;
    Q_EMIT snapshotBusyChanged();

    QNetworkRequest request(buildUrl(QStringLiteral("/api/v1/ops/snapshot"), QUrlQuery()));
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("SpeedClient/0.1"));

    QNetworkReply *reply = nam_->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const int status =
            reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() != QNetworkReply::NoError) {
            Q_EMIT errorOccurred(reply->errorString());
            Q_EMIT opsSnapshotReceived(QString(), status > 0 ? status : 0);
        } else {
            Q_EMIT opsSnapshotReceived(QString::fromUtf8(reply->readAll()), status);
        }
        reply->deleteLater();
        snapshot_busy_ = false;
        Q_EMIT snapshotBusyChanged();
    });
}

void RestClient::fetchOpsHistoryDaily(int days)
{
    setHistoryBusy(true);

    QUrlQuery query;
    query.addQueryItem(QStringLiteral("days"), QString::number(qBound(1, days, 366)));
    QNetworkRequest request(buildUrl(QStringLiteral("/api/v1/ops/history/daily"), query));
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("SpeedClient/0.1"));

    QNetworkReply *reply = nam_->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const int status =
            reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() != QNetworkReply::NoError) {
            Q_EMIT errorOccurred(reply->errorString());
            Q_EMIT opsHistoryDailyReceived(QString(), status > 0 ? status : 0);
        } else {
            Q_EMIT opsHistoryDailyReceived(QString::fromUtf8(reply->readAll()), status);
        }
        reply->deleteLater();
        setHistoryBusy(false);
    });
}

void RestClient::fetchOpsHistoryHeatmap(int year)
{
    setHistoryBusy(true);

    QUrlQuery query;
    if (year > 0) {
        query.addQueryItem(QStringLiteral("year"), QString::number(year));
    }
    QNetworkRequest request(buildUrl(QStringLiteral("/api/v1/ops/history/heatmap"), query));
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("SpeedClient/0.1"));

    QNetworkReply *reply = nam_->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const int status =
            reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() != QNetworkReply::NoError) {
            Q_EMIT errorOccurred(reply->errorString());
            Q_EMIT opsHistoryHeatmapReceived(QString(), status > 0 ? status : 0);
        } else {
            Q_EMIT opsHistoryHeatmapReceived(QString::fromUtf8(reply->readAll()), status);
        }
        reply->deleteLater();
        setHistoryBusy(false);
    });
}

void RestClient::fetchEcho(const QString &text)
{
    if (busy_) {
        Q_EMIT errorOccurred(QStringLiteral("REST 请求进行中"));
        return;
    }
    setBusy(true);

    QUrlQuery query;
    query.addQueryItem(QStringLiteral("text"), text);
    QNetworkRequest request(buildUrl(QStringLiteral("/api/v1/echo"), query));
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("SpeedClient/0.1"));

    QNetworkReply *reply = nam_->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const int status =
            reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() != QNetworkReply::NoError) {
            Q_EMIT errorOccurred(reply->errorString());
            Q_EMIT echoReceived(QString(), status > 0 ? status : 0);
        } else {
            Q_EMIT echoReceived(QString::fromUtf8(reply->readAll()), status);
        }
        reply->deleteLater();
        setBusy(false);
    });
}

QUrl RestClient::buildUrl(const QString &path, const QUrlQuery &query) const
{
    QUrl url;
    url.setScheme(QStringLiteral("http"));
    url.setHost(host_);
    url.setPort(port_);
    url.setPath(path);
    url.setQuery(query);
    return url;
}

void RestClient::setBusy(bool value)
{
    if (busy_ == value) {
        return;
    }
    busy_ = value;
    Q_EMIT busyChanged();
}

} // namespace speed::client::rest

#include "config/network_proxy.hpp"

#include <QNetworkProxy>

namespace speed::client {

void applyNetworkProxy(const ClientSettings &cfg)
{
    QNetworkProxy proxy(QNetworkProxy::NoProxy);
    if (cfg.proxy_enabled) {
        const QString host = cfg.proxy_host.trimmed().isEmpty() ? QStringLiteral("127.0.0.1")
                                                                  : cfg.proxy_host.trimmed();
        proxy.setHostName(host);
        proxy.setPort(cfg.proxy_port > 0 ? cfg.proxy_port : 7890);
        proxy.setType(cfg.proxy_socks5 ? QNetworkProxy::Socks5Proxy : QNetworkProxy::HttpProxy);
    }
    QNetworkProxy::setApplicationProxy(proxy);
}

QString proxySummaryLabel(const ClientSettings &cfg)
{
    if (!cfg.proxy_enabled) {
        return QStringLiteral("直连");
    }
    const QString kind = cfg.proxy_socks5 ? QStringLiteral("SOCKS5") : QStringLiteral("HTTP");
    return QStringLiteral("%1 · %2:%3").arg(kind, cfg.proxy_host, QString::number(cfg.proxy_port));
}

} // namespace speed::client
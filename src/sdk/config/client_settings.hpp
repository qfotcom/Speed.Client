#pragma once

#include <QString>

namespace speed::client {

struct ClientSettings {
    QString legacy_host;
    QString rest_host;
    quint16 legacy_port{9001};
    quint16 rest_port{443};
    bool rest_use_https{true};
    QString subscribe_topic{QStringLiteral("quote.test")};
    QString echo_text{QStringLiteral("world")};
    bool auto_connect_on_startup{true};
    /** 默认关闭：不使用本机代理 */
    bool proxy_enabled{false};
    QString proxy_host{QStringLiteral("127.0.0.1")};
    quint16 proxy_port{7890};
    /** true = SOCKS5（推荐，Legacy TCP + REST 共用）；false = HTTP 代理 */
    bool proxy_socks5{true};

    static ClientSettings load();
    void save() const;
    /** 生产环境默认（单域名双端口，见 Speed.Server docs/speed-client-connectivity.md） */
    static ClientSettings productionDefaults();
    /** @deprecated 使用 productionDefaults() */
    static ClientSettings cpolarDefaults();

    static QString defaultLegacyHost();
    static QString defaultRestHost();
    static quint16 defaultLegacyPort();
    static quint16 defaultRestPort();
    static bool defaultRestUseHttps();
};

} // namespace speed::client

#pragma once

#include <QString>

namespace speed::client {

struct ClientSettings {
    QString legacy_host;
    QString rest_host;
    quint16 legacy_port{9001};
    quint16 rest_port{8080};
    QString subscribe_topic{QStringLiteral("quote.test")};
    QString echo_text{QStringLiteral("world")};
    bool auto_connect_on_startup{true};

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
};

} // namespace speed::client

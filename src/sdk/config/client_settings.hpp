#pragma once

#include <QString>

namespace speed::client {

struct ClientSettings {
    QString legacy_host;
    QString rest_host;
    quint16 legacy_port{20771};
    quint16 rest_port{10513};
    QString subscribe_topic{QStringLiteral("quote.test")};
    QString echo_text{QStringLiteral("world")};
    bool auto_connect_on_startup{true};

    static ClientSettings load();
    void save() const;
    /** cpolar 双通道默认（Legacy / REST 主机不同） */
    static ClientSettings cpolarDefaults();

    /** 默认 cpolar 预留 TCP（见 Speed.Server docs/speed-client-connectivity.md） */
    static QString defaultLegacyHost();
    static QString defaultRestHost();
    static quint16 defaultLegacyPort();
    static quint16 defaultRestPort();
};

} // namespace speed::client

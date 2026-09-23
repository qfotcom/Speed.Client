#pragma once

#include <QString>

namespace speed::client {

struct ClientSettings {
    QString host;
    quint16 legacy_port{9001};
    quint16 rest_port{8080};
    QString subscribe_topic{QStringLiteral("quote.test")};
    QString echo_text{QStringLiteral("world")};

    static ClientSettings load();
    void save() const;

    static QString defaultHost();
};

} // namespace speed::client

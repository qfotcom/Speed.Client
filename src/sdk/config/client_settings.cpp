#include "config/client_settings.hpp"

#include <QSettings>

#if defined(Q_OS_ANDROID)
#include <QtGlobal>
#endif

namespace speed::client {

QString ClientSettings::defaultHost()
{
#if defined(Q_OS_ANDROID)
    return QStringLiteral("10.0.2.2");
#else
    return QStringLiteral("127.0.0.1");
#endif
}

ClientSettings ClientSettings::load()
{
    QSettings settings(QStringLiteral("SpeedClient"), QStringLiteral("SpeedClient"));
    ClientSettings cfg;
    cfg.host = settings.value(QStringLiteral("host"), defaultHost()).toString();
    cfg.legacy_port = static_cast<quint16>(
        settings.value(QStringLiteral("legacy_port"), 9001).toUInt());
    cfg.rest_port =
        static_cast<quint16>(settings.value(QStringLiteral("rest_port"), 8080).toUInt());
    cfg.subscribe_topic =
        settings.value(QStringLiteral("subscribe_topic"), QStringLiteral("quote.test"))
            .toString();
    cfg.echo_text = settings.value(QStringLiteral("echo_text"), QStringLiteral("world")).toString();
    return cfg;
}

void ClientSettings::save() const
{
    QSettings settings(QStringLiteral("SpeedClient"), QStringLiteral("SpeedClient"));
    settings.setValue(QStringLiteral("host"), host);
    settings.setValue(QStringLiteral("legacy_port"), legacy_port);
    settings.setValue(QStringLiteral("rest_port"), rest_port);
    settings.setValue(QStringLiteral("subscribe_topic"), subscribe_topic);
    settings.setValue(QStringLiteral("echo_text"), echo_text);
    settings.sync();
}

} // namespace speed::client

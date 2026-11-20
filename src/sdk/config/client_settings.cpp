#include "config/client_settings.hpp"

#include <QSettings>

namespace speed::client {

QString ClientSettings::defaultLegacyHost()
{
    return QStringLiteral("api.qfot.com");
}

QString ClientSettings::defaultRestHost()
{
    return QStringLiteral("api.qfot.com");
}

quint16 ClientSettings::defaultLegacyPort()
{
    return 9001;
}

quint16 ClientSettings::defaultRestPort()
{
    return 443;
}

bool ClientSettings::defaultRestUseHttps()
{
    return true;
}

static bool inferRestUseHttps(quint16 restPort, const QString &restHost)
{
    if (restPort == 443) {
        return true;
    }
    if (restPort == 8080) {
        return false;
    }
    return restPort == ClientSettings::defaultRestPort()
            && restHost == ClientSettings::defaultRestHost();
}

static bool isLanPortPair(quint16 legacyPort, quint16 restPort)
{
    return legacyPort == 9001 && restPort == 8080;
}

static bool isArchivedCpolarEndpoints(const ClientSettings &cfg)
{
    return cfg.legacy_host == QStringLiteral("1.tcp.cpolar.cn")
            && cfg.rest_host == QStringLiteral("6.tcp.cpolar.cn")
            && cfg.legacy_port == 20771 && cfg.rest_port == 10513;
}

ClientSettings ClientSettings::load()
{
    QSettings settings(QStringLiteral("SpeedClient"), QStringLiteral("SpeedClient"));
    ClientSettings cfg;

    const QString legacyHostKey = QStringLiteral("legacy_host");
    const QString restHostKey = QStringLiteral("rest_host");
    const QString oldHostKey = QStringLiteral("host");

    const bool hasLegacyHost = settings.contains(legacyHostKey);
    const bool hasRestHost = settings.contains(restHostKey);
    const bool hasOldHost = settings.contains(oldHostKey);

    const quint16 savedLegacyPort = static_cast<quint16>(
        settings.value(QStringLiteral("legacy_port"), defaultLegacyPort()).toUInt());
    const quint16 savedRestPort = static_cast<quint16>(
        settings.value(QStringLiteral("rest_port"), defaultRestPort()).toUInt());

    if (hasLegacyHost || hasRestHost) {
        // 新版：两通道各自 host，缺省的一路用该通道默认值（勿再共用）
        cfg.legacy_host = hasLegacyHost
                ? settings.value(legacyHostKey).toString().trimmed()
                : defaultLegacyHost();
        cfg.rest_host = hasRestHost ? settings.value(restHostKey).toString().trimmed()
                                    : defaultRestHost();
    } else if (hasOldHost) {
        const QString shared = settings.value(oldHostKey).toString().trimmed();
        if (isLanPortPair(savedLegacyPort, savedRestPort)) {
            cfg.legacy_host = shared;
            cfg.rest_host = shared;
        } else {
            cfg.legacy_host = defaultLegacyHost();
            cfg.rest_host = defaultRestHost();
        }
    } else {
        cfg.legacy_host = defaultLegacyHost();
        cfg.rest_host = defaultRestHost();
    }

    if (cfg.legacy_host.isEmpty()) {
        cfg.legacy_host = defaultLegacyHost();
    }
    if (cfg.rest_host.isEmpty()) {
        cfg.rest_host = defaultRestHost();
    }

    const bool lanFromOldHost = hasOldHost && !hasLegacyHost && !hasRestHost
            && isLanPortPair(savedLegacyPort, savedRestPort);

    cfg.legacy_port = lanFromOldHost ? savedLegacyPort : static_cast<quint16>(
        settings.value(QStringLiteral("legacy_port"), defaultLegacyPort()).toUInt());
    cfg.rest_port = lanFromOldHost ? savedRestPort : static_cast<quint16>(
        settings.value(QStringLiteral("rest_port"), defaultRestPort()).toUInt());

    if (settings.contains(QStringLiteral("rest_use_https"))) {
        cfg.rest_use_https = settings.value(QStringLiteral("rest_use_https")).toBool();
    } else {
        cfg.rest_use_https = inferRestUseHttps(cfg.rest_port, cfg.rest_host);
    }

    if (cfg.rest_host == defaultRestHost() && cfg.rest_port == 8080) {
        cfg.rest_port = defaultRestPort();
        cfg.rest_use_https = true;
        cfg.save();
    }

    cfg.subscribe_topic =
        settings.value(QStringLiteral("subscribe_topic"), QStringLiteral("quote.test"))
            .toString();
    cfg.echo_text =
        settings.value(QStringLiteral("echo_text"), QStringLiteral("world")).toString();
    cfg.auto_connect_on_startup =
        settings.value(QStringLiteral("auto_connect_on_startup"), true).toBool();

    cfg.proxy_enabled = settings.value(QStringLiteral("proxy_enabled"), false).toBool();
    cfg.proxy_host =
        settings.value(QStringLiteral("proxy_host"), QStringLiteral("127.0.0.1")).toString();
    cfg.proxy_port = static_cast<quint16>(
        settings.value(QStringLiteral("proxy_port"), 7890).toUInt());
    cfg.proxy_socks5 = settings.value(QStringLiteral("proxy_socks5"), true).toBool();

    if (isArchivedCpolarEndpoints(cfg)) {
        const ClientSettings prod = productionDefaults();
        cfg.legacy_host = prod.legacy_host;
        cfg.rest_host = prod.rest_host;
        cfg.legacy_port = prod.legacy_port;
        cfg.rest_port = prod.rest_port;
        cfg.rest_use_https = prod.rest_use_https;
        cfg.save();
    }

    return cfg;
}

void ClientSettings::save() const
{
    QSettings settings(QStringLiteral("SpeedClient"), QStringLiteral("SpeedClient"));
    settings.setValue(QStringLiteral("legacy_host"), legacy_host);
    settings.setValue(QStringLiteral("rest_host"), rest_host);
    settings.remove(QStringLiteral("host"));
    settings.setValue(QStringLiteral("legacy_port"), legacy_port);
    settings.setValue(QStringLiteral("rest_port"), rest_port);
    settings.setValue(QStringLiteral("rest_use_https"), rest_use_https);
    settings.setValue(QStringLiteral("subscribe_topic"), subscribe_topic);
    settings.setValue(QStringLiteral("echo_text"), echo_text);
    settings.setValue(QStringLiteral("auto_connect_on_startup"), auto_connect_on_startup);
    settings.setValue(QStringLiteral("proxy_enabled"), proxy_enabled);
    settings.setValue(QStringLiteral("proxy_host"), proxy_host);
    settings.setValue(QStringLiteral("proxy_port"), proxy_port);
    settings.setValue(QStringLiteral("proxy_socks5"), proxy_socks5);
    settings.setValue(QStringLiteral("endpoints_schema"), 2);
    settings.sync();
}

ClientSettings ClientSettings::productionDefaults()
{
    ClientSettings cfg;
    cfg.legacy_host = defaultLegacyHost();
    cfg.rest_host = defaultRestHost();
    cfg.legacy_port = defaultLegacyPort();
    cfg.rest_port = defaultRestPort();
    cfg.rest_use_https = defaultRestUseHttps();
    return cfg;
}

ClientSettings ClientSettings::cpolarDefaults()
{
    return productionDefaults();
}

} // namespace speed::client

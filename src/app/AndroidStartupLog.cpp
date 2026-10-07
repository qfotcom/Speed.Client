#include "AndroidStartupLog.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QMutex>
#include <QMutexLocker>
#include <QStandardPaths>
#include <QTextStream>
#include <QTimeZone>

#if defined(Q_OS_ANDROID)
#include <android/log.h>

__attribute__((constructor)) static void speedClientLibraryLoaded()
{
    __android_log_print(ANDROID_LOG_INFO, "SpeedClient", "libSpeedClient .so loaded (pre-main)");
}
#endif

namespace {

QMutex g_mutex;
QString g_logPath;
QString g_mirrorPath;
bool g_installed = false;

#if defined(Q_OS_ANDROID)
QtMessageHandler g_previousHandler = nullptr;

void startupMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    if (g_previousHandler) {
        g_previousHandler(type, context, msg);
    }
    AndroidStartupLog::write(QStringLiteral("[%1] %2")
                                 .arg(context.category ? context.category : "default", msg));
}
#endif

QString formatBeijingLogStamp()
{
    static const QTimeZone beijing(QByteArray("Asia/Shanghai"));
    const QDateTime now = QDateTime::currentDateTime(beijing);
    const int offsetSec = now.offsetFromUtc();
    const int hours = offsetSec / 3600;
    const int mins = qAbs((offsetSec / 60) % 60);
    const QChar sign = offsetSec >= 0 ? QLatin1Char('+') : QLatin1Char('-');
    const QString offset = QStringLiteral("%1%2:%3")
                               .arg(sign)
                               .arg(qAbs(hours), 2, 10, QLatin1Char('0'))
                               .arg(mins, 2, 10, QLatin1Char('0'));
    return now.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz")) + QLatin1Char(' ') + offset;
}

} // namespace

void AndroidStartupLog::appendLine(const QString &line)
{
    QMutexLocker lock(&g_mutex);

#if defined(Q_OS_ANDROID)
    __android_log_print(ANDROID_LOG_INFO, "SpeedClient", "%s", line.toUtf8().constData());
#endif

    if (!g_logPath.isEmpty()) {
        QFile file(g_logPath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            QTextStream out(&file);
            out << line << '\n';
        }
    }

    if (!g_mirrorPath.isEmpty()) {
        QFile mirror(g_mirrorPath);
        if (mirror.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            QTextStream out(&mirror);
            out << line << '\n';
        }
    }
}

void AndroidStartupLog::install(QGuiApplication *app)
{
    if (!app || g_installed) {
        return;
    }
    g_installed = true;

    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    g_logPath = dir + QStringLiteral("/speed_startup.log");

    const QString downloadRoot = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    if (!downloadRoot.isEmpty()) {
        const QString mirrorDir = downloadRoot + QStringLiteral("/SpeedClient");
        if (QDir().mkpath(mirrorDir)) {
            g_mirrorPath = mirrorDir + QStringLiteral("/speed_startup.log");
        }
    }

#if defined(Q_OS_ANDROID)
    g_previousHandler = qInstallMessageHandler(startupMessageHandler);
#endif

    write(QStringLiteral("AndroidStartupLog installed"));
}

void AndroidStartupLog::write(const QString &line)
{
    const QString stamped = formatBeijingLogStamp() + QLatin1Char(' ') + line;
    appendLine(stamped);
}

QString AndroidStartupLog::logFilePath()
{
    return g_logPath;
}

QString AndroidStartupLog::logMirrorPath()
{
    return g_mirrorPath;
}

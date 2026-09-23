#include "ClientBackend.h"

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QFile>
#include <QTimer>
#include <QFont>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlError>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QSGRendererInterface>

#include "SenseAnimations.h"
#include "SenseIconRegistry.h"
#include "SenseShadows.h"
#include "SenseSpacing.h"
#include "SenseTheme.h"
#include "SenseTypography.h"

#if defined(Q_OS_ANDROID)
#include "AndroidStartupLog.h"
#include "AndroidSystemUiBridge.h"
#include <android/log.h>
#endif

namespace {

void logQmlErrors(const char *context, const QList<QQmlError> &errors)
{
    for (const QQmlError &error : errors) {
        qCritical().noquote() << context << error.toString();
    }
}

#if defined(Q_OS_ANDROID)
void androidQtMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    int priority = ANDROID_LOG_INFO;
    switch (type) {
    case QtDebugMsg:
        priority = ANDROID_LOG_DEBUG;
        break;
    case QtWarningMsg:
        priority = ANDROID_LOG_WARN;
        break;
    case QtCriticalMsg:
    case QtFatalMsg:
        priority = ANDROID_LOG_ERROR;
        break;
    default:
        break;
    }
    const QByteArray local = msg.toLocal8Bit();
    const QByteArray tag = context.category && context.category[0] != '\0' ? context.category
                                                                           : "SpeedClient";
    __android_log_print(priority, tag.constData(), "%s", local.constData());
    if (type == QtFatalMsg) {
        abort();
    }
}
#endif

} // namespace

int main(int argc, char *argv[])
{
#if defined(Q_OS_ANDROID)
    qInstallMessageHandler(androidQtMessageHandler);
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
#endif

    QQuickStyle::setStyle("Basic");

    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("SpeedClient"));
    app.setOrganizationName(QStringLiteral("Speed"));
    app.setOrganizationDomain(QStringLiteral("speed.client"));
    app.setApplicationVersion(QStringLiteral("0.1.0"));

#if defined(Q_OS_ANDROID)
    AndroidStartupLog::install(&app);
#endif

#if defined(Q_OS_ANDROID)
    QFont appFont(app.font());
    appFont.setPointSize(14);
    app.setFont(appFont);
#endif

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    parser.process(app);

    Q_INIT_RESOURCE(qmake_SpeedClient);
    Q_INIT_RESOURCE(SpeedClient_raw_qml_0);

    Q_INIT_RESOURCE(src);
    Q_INIT_RESOURCE(icons_svg);
    Q_INIT_RESOURCE(qmake_SenseDesign);
    Q_INIT_RESOURCE(SenseDesign_raw_qml_0);
    Q_INIT_RESOURCE(SenseDesign_raw_res_0);
    Q_INIT_RESOURCE(qmake_SenseAppShell);
    Q_INIT_RESOURCE(SenseAppShell_raw_qml_0);

    SenseIconRegistry *iconRegistry = SenseIconRegistry::instance();
    iconRegistry->loadFonts();
    iconRegistry->loadRegistry();

    qmlRegisterSingletonInstance("SenseDesign", 1, 0, "SenseTheme", SenseTheme::instance());
    qmlRegisterSingletonInstance("SenseDesign", 1, 0, "SenseTypography", SenseTypography::instance());
    qmlRegisterSingletonInstance("SenseDesign", 1, 0, "SenseSpacing", SenseSpacing::instance());
    qmlRegisterSingletonInstance("SenseDesign", 1, 0, "SenseShadows", SenseShadows::instance());
    qmlRegisterSingletonInstance("SenseDesign", 1, 0, "SenseAnimations", SenseAnimations::instance());
    qmlRegisterSingletonInstance("SenseDesign", 1, 0, "SenseIconRegistry", iconRegistry);

    ClientBackend backend;
#if defined(Q_OS_ANDROID)
    AndroidSystemUiBridge androidSystemUiBridge;
#endif

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("ClientBackend"), &backend);
#if defined(Q_OS_ANDROID)
    engine.rootContext()->setContextProperty(QStringLiteral("AndroidSystemUiBridge"),
                                             &androidSystemUiBridge);
#endif

    engine.addImportPath(QStringLiteral("qrc:/"));
    engine.addImportPath(QStringLiteral("qrc:/qt/qml"));

    QObject::connect(
        &engine, &QQmlApplicationEngine::warnings, &app,
        [](const QList<QQmlError> &warnings) { logQmlErrors("QML warning:", warnings); });

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app, []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

#if defined(Q_OS_ANDROID)
    qInfo() << "QML resource Main.qml exists:"
            << QFile(QStringLiteral(":/qt/qml/SpeedClient/Main.qml")).exists();
#endif

    engine.loadFromModule(QStringLiteral("SpeedClient"), QStringLiteral("Main"));
    if (engine.rootObjects().isEmpty()) {
        qCritical() << "Failed to load SpeedClient/Main from module";
        return -1;
    }

#if defined(Q_OS_ANDROID)
    QTimer::singleShot(0, &androidSystemUiBridge, &AndroidSystemUiBridge::ensureStatusBarVisible);
#endif

    return app.exec();
}

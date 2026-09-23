#include "AndroidSystemUiBridge.h"

#include <functional>

#if defined(Q_OS_ANDROID)
#include <QJniObject>
#include <QtCore/qcoreapplication_platform.h>
#endif

namespace {

#if defined(Q_OS_ANDROID)
constexpr int kWindowFullscreenFlag = 1024;
constexpr int kAppearanceLightStatusBars = 8;
constexpr int kSystemUiFlagLightStatusBar = 0x00002000;

QJniObject androidActivityWindow()
{
    QJniObject activity = QJniObject::callStaticObjectMethod(
        "org/qtproject/qt/android/QtNative", "activity", "()Landroid/app/Activity;");
    if (!activity.isValid()) {
        return {};
    }
    return activity.callObjectMethod("getWindow", "()Landroid/view/Window;");
}

void clearAndroidFullscreenFlag()
{
    QJniObject window = androidActivityWindow();
    if (!window.isValid()) {
        return;
    }
    window.callMethod<void>("clearFlags", "(I)V", kWindowFullscreenFlag);
}

void applyStatusBarOnAndroid(const QColor &color, bool lightStatusBarIcons)
{
    QJniObject window = androidActivityWindow();
    if (!window.isValid()) {
        return;
    }

    window.callMethod<void>("clearFlags", "(I)V", kWindowFullscreenFlag);

    const int argb = (color.alpha() << 24) | (color.red() << 16) | (color.green() << 8)
                     | color.blue();
    window.callMethod<void>("setStatusBarColor", "(I)V", argb);

    QJniObject decorView = window.callObjectMethod("getDecorView", "()Landroid/view/View;");
    if (!decorView.isValid()) {
        return;
    }

    QJniObject insetsController =
        window.callObjectMethod("getInsetsController", "()Landroid/view/WindowInsetsController;");
    if (insetsController.isValid()) {
        if (lightStatusBarIcons) {
            insetsController.callMethod<void>("setSystemBarsAppearance", "(II)V",
                                              kAppearanceLightStatusBars, kAppearanceLightStatusBars);
        } else {
            insetsController.callMethod<void>("setSystemBarsAppearance", "(II)V", 0,
                                              kAppearanceLightStatusBars);
        }
        return;
    }

    int visibility = decorView.callMethod<jint>("getSystemUiVisibility", "()I");
    if (lightStatusBarIcons) {
        visibility |= kSystemUiFlagLightStatusBar;
    } else {
        visibility &= ~kSystemUiFlagLightStatusBar;
    }
    decorView.callMethod<void>("setSystemUiVisibility", "(I)V", visibility);
}

void runOnAndroidUiThread(const std::function<void()> &work)
{
#if QT_CONFIG(future)
    QNativeInterface::QAndroidApplication::runOnAndroidMainThread(work);
#else
    work();
#endif
}
#endif

} // namespace

AndroidSystemUiBridge::AndroidSystemUiBridge(QObject *parent)
    : QObject(parent)
{
}

bool AndroidSystemUiBridge::isLightBackground(const QColor &color)
{
    const QColor opaque = color.alpha() < 255 ? QColor(color.red(), color.green(), color.blue())
                                              : color;
    const qreal luminance = 0.2126 * opaque.redF() + 0.7152 * opaque.greenF()
                            + 0.0722 * opaque.blueF();
    return luminance > 0.55;
}

void AndroidSystemUiBridge::ensureStatusBarVisible()
{
#if defined(Q_OS_ANDROID)
    runOnAndroidUiThread([]() { clearAndroidFullscreenFlag(); });
#endif
}

void AndroidSystemUiBridge::applyStatusBarColor(const QString &colorString)
{
    applyStatusBarColorValue(QColor(colorString));
}

void AndroidSystemUiBridge::applyStatusBarColorValue(const QColor &color)
{
#if defined(Q_OS_ANDROID)
    if (!color.isValid()) {
        return;
    }
    const QColor copy = color;
    const bool lightIcons = isLightBackground(copy);
    runOnAndroidUiThread([copy, lightIcons]() { applyStatusBarOnAndroid(copy, lightIcons); });
#else
    Q_UNUSED(color);
#endif
}

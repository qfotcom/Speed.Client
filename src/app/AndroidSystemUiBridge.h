#pragma once

#include <QColor>
#include <QObject>
#include <QString>

class AndroidSystemUiBridge : public QObject
{
    Q_OBJECT

public:
    explicit AndroidSystemUiBridge(QObject *parent = nullptr);

    /** @brief 退出全屏并显示系统状态栏 */
    Q_INVOKABLE void ensureStatusBarVisible();

    /**
     * @brief 设置状态栏背景色与图标明暗（浅色背景用深色图标）
     * @param colorString #RRGGBB 或主题色字符串
     */
    Q_INVOKABLE void applyStatusBarColor(const QString &colorString);
    Q_INVOKABLE void applyStatusBarColorValue(const QColor &color);

    /** @brief darkTheme=true → 浅色状态栏图标（深色背景）；false → 深色图标 */
    Q_INVOKABLE void applyStatusBarForTheme(bool darkTheme, const QString &backgroundColor);

private:
    static bool isLightBackground(const QColor &color);
};

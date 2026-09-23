#pragma once

#include <QColor>
#include <QObject>
#include <QString>

class AndroidSystemUiBridge : public QObject
{
    Q_OBJECT

public:
    explicit AndroidSystemUiBridge(QObject *parent = nullptr);

    Q_INVOKABLE void ensureStatusBarVisible();
    Q_INVOKABLE void applyStatusBarColor(const QString &colorString);
    Q_INVOKABLE void applyStatusBarColorValue(const QColor &color);

private:
    static bool isLightBackground(const QColor &color);
};

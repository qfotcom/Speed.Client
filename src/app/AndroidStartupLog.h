#pragma once

#include <QGuiApplication>
#include <QString>

class AndroidStartupLog
{
public:
    static void install(QGuiApplication *app);
    static void write(const QString &line);
    static QString logFilePath();
    static QString logMirrorPath();

private:
    static void appendLine(const QString &line);
};

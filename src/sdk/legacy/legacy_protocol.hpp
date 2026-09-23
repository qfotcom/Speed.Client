#pragma once

#include <QString>
#include <QStringList>

namespace speed::client::legacy {

struct FrameParseResult {
    QStringList push_lines;
    QString primary_line;
};

/** @brief 4-byte big-endian length + UTF-8 payload (may contain embedded newlines). */
QByteArray encodeFrame(const QString &line);

/** @brief Split workflow bundled response: PUSH lines then final OK/PONG/ERR/... */
FrameParseResult parseFrameBody(const QString &body);

} // namespace speed::client::legacy

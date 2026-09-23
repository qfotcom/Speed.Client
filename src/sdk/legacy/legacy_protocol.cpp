#include "legacy/legacy_protocol.hpp"

#include <QtEndian>

#include <cstring>

namespace speed::client::legacy {

QByteArray encodeFrame(const QString &line)
{
    const QByteArray payload = line.toUtf8();
    const quint32 len = static_cast<quint32>(payload.size());
    const quint32 be = qToBigEndian(len);
    QByteArray frame;
    frame.resize(4 + payload.size());
    std::memcpy(frame.data(), &be, 4);
    std::memcpy(frame.data() + 4, payload.constData(), static_cast<size_t>(payload.size()));
    return frame;
}

FrameParseResult parseFrameBody(const QString &body)
{
    FrameParseResult result;
    if (body.isEmpty()) {
        return result;
    }

    const QStringList lines = body.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    if (lines.isEmpty()) {
        return result;
    }

    if (lines.size() == 1) {
        result.primary_line = lines.front();
        return result;
    }

    for (int i = 0; i < lines.size() - 1; ++i) {
        const QString &line = lines.at(i);
        if (line.startsWith(QStringLiteral("PUSH "))) {
            result.push_lines.push_back(line);
        } else {
            result.push_lines.push_back(line);
        }
    }
    result.primary_line = lines.last();
    return result;
}

} // namespace speed::client::legacy

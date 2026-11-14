import QtQuick
import SenseDesign

/**
 * 按数值档位着色的状态胶囊（封装 ClientStatusBadge + ShadcnStyles.valueTierTone）。
 * 默认档位与 Legacy Ping 一致：<80 ms 绿、<200 ms 灰边、≥200 ms 红。
 */
Item {
    id: root

    implicitWidth: badge.implicitWidth
    implicitHeight: badge.implicitHeight

    property real value: -1
    property bool pending: false
    property string text: ""
    /** 升序 `{ upTo, tone }` 列表 */
    property var tiers: [
        { upTo: 79, tone: "success" },
        { upTo: 199, tone: "neutral" },
        { upTo: 2147483647, tone: "danger" }
    ]

    readonly property string resolvedTone: ShadcnStyles.valueTierTone(value, pending, tiers)
    readonly property color valueForeground: root._foregroundForTone(resolvedTone)

    function _foregroundForTone(tone) {
        switch (tone) {
        case "danger":
            return ShadcnTheme.c("destructive")
        case "success":
            return ShadcnTheme.darkMode ? "#4ade80" : "#15803d"
        case "pending":
            return ShadcnTheme.c("muted-foreground")
        default:
            return ShadcnTheme.c("foreground")
        }
    }

    ClientStatusBadge {
        id: badge
        text: root.text
        tone: root.resolvedTone
    }
}
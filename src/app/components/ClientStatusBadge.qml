import QtQuick
import QtQuick.Controls
import SenseDesign

/** 连接/可用性状态：success 绿、danger 红（destructive）、pending/neutral 辅助态 */
Control {
    id: control

    property string text: ""
    /** @brief success | danger | neutral | pending */
    property string tone: "neutral"

    implicitHeight: 22
    implicitWidth: Math.max(contentItem.implicitWidth + padding * 2, 32)
    padding: 4

    readonly property color _successFg: ShadcnTheme.darkMode ? "#4ade80" : "#15803d"

    readonly property var _colors: {
        switch (control.tone) {
        case "success":
            return {
                bg: ShadcnStyles.withAlpha(_successFg, ShadcnTheme.darkMode ? 0.2 : 0.12),
                fg: _successFg,
                border: ShadcnStyles.withAlpha(_successFg, 0.45)
            }
        case "danger":
            return {
                bg: ShadcnTheme.c("destructive"),
                fg: ShadcnTheme.c("destructive-foreground"),
                border: "transparent"
            }
        case "pending":
            return {
                bg: ShadcnTheme.c("muted"),
                fg: ShadcnTheme.c("muted-foreground"),
                border: ShadcnTheme.c("border")
            }
        default:
            return {
                bg: "transparent",
                fg: ShadcnTheme.c("muted-foreground"),
                border: ShadcnTheme.c("border")
            }
        }
    }

    contentItem: Text {
        text: control.text
        color: control._colors.fg
        font.pixelSize: ShadcnTypography.fontSizeXs
        font.weight: ShadcnTypography.fontWeightMedium
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    background: Rectangle {
        radius: height / 2
        color: control._colors.bg
        border.width: control._colors.border !== "transparent" ? 1 : 0
        border.color: control._colors.border
    }
}

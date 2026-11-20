import QtQuick
import QtQuick.Controls
import SenseDesign

/** 连接/可用性状态：success 绿、danger 红（destructive）、pending/neutral 辅助态 */
Control {
    id: control

    property string text: ""
    /** @brief success | danger | neutral | pending */
    property string tone: "neutral"
    /**
     * 短时传输态（如 Legacy PING 往返）。不改变文案与 tone，仅显示防抖后的活动点，
     * 避免「在线 / 通信中」与绿/灰底色来回闪。
     */
    property bool activity: false
    /** 为活动点预留宽度，避免亮/灭时胶囊左右伸缩 */
    property bool reserveActivitySlot: false
    /** 常驻状态点（开/关泳道）；与 activity 脉冲并存，脉冲时优先闪动 */
    property bool showStatusDot: false

    readonly property bool _showDotSlot: reserveActivitySlot || showStatusDot
            || control._activityVisible
    readonly property real _staticDotOpacity: {
        switch (control.tone) {
        case "success":
        case "danger":
            return 1
        case "pending":
            return 0.78
        default:
            return ShadcnTheme.darkMode ? 0.62 : 0.52
        }
    }
    readonly property int _dotSlotWidth: _showDotSlot ? 6 : 0
    readonly property int _dotGap: _showDotSlot ? 5 : 0

    implicitHeight: Math.max(label.implicitHeight, 14) + padding * 2
    implicitWidth: Math.max(_dotSlotWidth + _dotGap + label.implicitWidth + padding * 2, 32)
    padding: 4

    property bool _activityVisible: false

    Timer {
        id: activityHold
        interval: 450
        repeat: false
        onTriggered: control._activityVisible = false
    }

    onActivityChanged: {
        if (activity) {
            activityHold.stop()
            control._activityVisible = true
        } else if (control._activityVisible) {
            activityHold.restart()
        }
    }

    readonly property color _successFg: ShadcnTheme.darkMode ? "#4ade80" : "#15803d"

    readonly property var _colors: {
        switch (control.tone) {
        case "success":
            return {
                bg: ShadcnStyles.withAlpha(_successFg, ShadcnTheme.darkMode ? 0.2 : 0.12),
                fg: _successFg,
                border: ShadcnStyles.withAlpha(_successFg, 0.45),
                dot: _successFg
            }
        case "danger":
            return {
                bg: ShadcnTheme.c("destructive"),
                fg: ShadcnTheme.c("destructive-foreground"),
                border: "transparent",
                dot: ShadcnTheme.c("destructive-foreground")
            }
        case "pending":
            return {
                bg: ShadcnTheme.c("muted"),
                fg: ShadcnTheme.c("muted-foreground"),
                border: ShadcnTheme.c("border"),
                dot: ShadcnTheme.c("muted-foreground")
            }
        default:
            return {
                bg: ShadcnTheme.darkMode
                        ? ShadcnStyles.withAlpha(ShadcnTheme.c("muted"), 0.38)
                        : "transparent",
                fg: ShadcnTheme.c("muted-foreground"),
                border: ShadcnTheme.darkMode
                        ? ShadcnStyles.withAlpha(ShadcnTheme.c("muted-foreground"), 0.48)
                        : ShadcnTheme.c("border"),
                dot: ShadcnTheme.c("muted-foreground")
            }
        }
    }

    contentItem: Item {
        Row {
            id: row
            anchors.centerIn: parent
            spacing: control._dotGap

            Item {
                width: control._dotSlotWidth
                height: label.height

                Rectangle {
                    anchors.centerIn: parent
                    width: 6
                    height: 6
                    radius: 3
                    color: control._colors.dot
                    opacity: control._activityVisible ? pulseLevel
                            : (control.showStatusDot ? control._staticDotOpacity : 0)

                    property real pulseLevel: 0.7

                    SequentialAnimation on pulseLevel {
                        running: control._activityVisible
                        loops: Animation.Infinite
                        NumberAnimation { from: 0.35; to: 0.95; duration: 550; easing.type: Easing.InOutQuad }
                        NumberAnimation { from: 0.95; to: 0.35; duration: 550; easing.type: Easing.InOutQuad }
                    }
                }
            }

            Text {
                id: label
                anchors.verticalCenter: parent.verticalCenter
                text: control.text
                color: control._colors.fg
                font.pixelSize: ShadcnTypography.fontSizeXs
                font.weight: ShadcnTypography.fontWeightMedium
            }
        }
    }

    background: Rectangle {
        radius: height / 2
        color: control._colors.bg
        border.width: control._colors.border !== "transparent" ? 1 : 0
        border.color: control._colors.border
    }
}

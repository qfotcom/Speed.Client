import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import SenseDesign 1.0

SenseCard {
    id: root

    property var model: []

    Layout.fillWidth: true
    // 勿设 preferredHeight：小于卡片真实高度时会在页面底部被裁切

    title: "事件日志"
    subtitle: model.length > 0 ? (model.length + " 条记录 · 最新在上") : "暂无记录"
    iconName: "list-ul"

    // 列表可视区：移动端约 4–6 条（含 badge/换行），桌面略低
    readonly property int _logViewportHeight: SensePlatform.isMobile ? 320 : 260

    function logKind(line) {
        if (!line)
            return "default"
        if (line.indexOf("[PUSH]") >= 0)
            return "push"
        if (line.indexOf("ERR]") >= 0 || line.indexOf("[REST ERR]") >= 0 || line.indexOf("[Legacy ERR]") >= 0)
            return "error"
        if (line.indexOf("[REST") >= 0)
            return "rest"
        if (line.indexOf("[Legacy]") >= 0)
            return "legacy"
        return "default"
    }

    function logBadge(kind) {
        switch (kind) {
        case "push": return "PUSH"
        case "error": return "ERR"
        case "rest": return "REST"
        case "legacy": return "Legacy"
        default: return "LOG"
        }
    }

    function logBadgeColor(kind) {
        switch (kind) {
        case "push": return "success"
        case "error": return "danger"
        case "rest": return "info"
        case "legacy": return "primary"
        default: return "secondary"
        }
    }

    function logAccentColor(kind) {
        var theme = SenseTheme.currentTheme
        switch (kind) {
        case "push": return theme.success
        case "error": return theme.danger
        case "rest": return theme.info
        case "legacy": return theme.primary
        default: return theme.border
        }
    }

    body: Component {
        ColumnLayout {
            width: parent ? parent.width : 360
            spacing: SenseSpacing.spacer3

            RowLayout {
                Layout.fillWidth: true
                spacing: SenseSpacing.spacer2

                SenseBadge {
                    text: model.length > 0 ? String(model.length) : "0"
                    color: model.length > 0 ? "primary" : "secondary"
                    size: "sm"
                }
                SenseText {
                    Layout.fillWidth: true
                    text: model.length > 0 ? "滚动查看完整内容" : "操作后会在此显示 REST / Legacy / PUSH 日志"
                    size: "xs"
                    textColor: SenseTheme.currentTheme.textSecondary
                    autoWrap: true
                }
                SenseButton {
                    text: "清空"
                    variant: "outline"
                    size: "sm"
                    enabled: model.length > 0
                    onClicked: ClientBackend.clearLog()
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: root._logViewportHeight
                radius: SenseSpacing.borderRadius
                color: SenseTheme.withOpacity(SenseTheme.currentTheme.textPrimary, 0.02)
                border.width: 1
                border.color: SenseTheme.withOpacity(SenseTheme.currentTheme.border, 0.55)
                clip: true

                Item {
                    anchors.fill: parent
                    anchors.margins: SenseSpacing.spacer2

                    ListView {
                        id: logView
                        anchors.fill: parent
                        model: root.model
                        spacing: SenseSpacing.spacer2
                        clip: true
                        boundsBehavior: Flickable.StopAtBounds
                        ScrollBar.vertical: ScrollBar {
                            policy: ScrollBar.AsNeeded
                        }

                        onCountChanged: Qt.callLater(function () { positionViewAtBeginning() })

                        delegate: Rectangle {
                        id: row
                        width: logView.width
                        radius: SenseSpacing.borderRadiusSm
                        color: SenseTheme.withOpacity(SenseTheme.currentTheme.bgSurface, 0.85)
                        border.width: 1
                        border.color: SenseTheme.withOpacity(SenseTheme.currentTheme.border, 0.35)

                        readonly property string line: modelData
                        readonly property string kind: root.logKind(line)

                        implicitHeight: rowLayout.implicitHeight + SenseSpacing.spacer2 * 2

                        Rectangle {
                            width: 3
                            anchors.left: parent.left
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            anchors.topMargin: SenseSpacing.spacer2
                            anchors.bottomMargin: SenseSpacing.spacer2
                            radius: 2
                            color: root.logAccentColor(kind)
                        }

                        RowLayout {
                            id: rowLayout
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.leftMargin: SenseSpacing.spacer2 + 3
                            anchors.rightMargin: SenseSpacing.spacer2
                            spacing: SenseSpacing.spacer2

                            SenseBadge {
                                text: root.logBadge(kind)
                                color: root.logBadgeColor(kind)
                                size: "sm"
                                Layout.alignment: Qt.AlignTop
                            }

                            SenseText {
                                Layout.fillWidth: true
                                text: line
                                size: "xs"
                                autoWrap: true
                                textColor: kind === "error"
                                    ? SenseTheme.currentTheme.danger
                                    : SenseTheme.currentTheme.textPrimary
                                font.family: "Consolas, Cascadia Mono, Courier New, monospace"
                            }
                        }
                    }
                    }

                    SenseText {
                        anchors.centerIn: parent
                        visible: root.model.length === 0
                        width: parent.width - SenseSpacing.spacer4
                        horizontalAlignment: Text.AlignHCenter
                        text: "暂无日志"
                        size: "sm"
                        textColor: SenseTheme.currentTheme.textSecondary
                        autoWrap: true
                    }
                }
            }
        }
    }
}

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import SenseAppShell 1.0
import SenseDesign

ShadcnCard {
    id: root

    property var model: []
    property bool startExpanded: true

    width: parent ? parent.width : implicitWidth
    contentMargin: ShadcnAppPlatform.isMobile ? 14 : 20

    readonly property int _logViewportHeight: ShadcnAppPlatform.isMobile ? 280 : 240

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

    function logBadgeTone(kind) {
        switch (kind) {
        case "push": return "success"
        case "error": return "danger"
        default: return "neutral"
        }
    }

    function logAccent(kind) {
        switch (kind) {
        case "push":
            return ShadcnTheme.darkMode ? "#4ade80" : "#15803d"
        case "error": return ShadcnTheme.c("destructive")
        case "rest": return ShadcnTheme.c("chart-2")
        case "legacy": return ShadcnTheme.c("chart-4")
        default: return ShadcnTheme.c("border")
        }
    }

    ShadcnCollapsible {
        width: parent.width
        title: qsTr("事件日志")
        summary: model.length > 0
                 ? qsTr("%1 条 · 最新在上").arg(model.length)
                 : qsTr("REST / Legacy / PUSH 操作记录")
        expanded: root.startExpanded

        ColumnLayout {
            width: parent.width
            spacing: 10

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                ShadcnBadge {
                    text: model.length > 0 ? String(model.length) : "0"
                    variant: model.length > 0 ? "default" : "secondary"
                }

                ShadcnLabel {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    font.pixelSize: ShadcnTypography.fontSizeXs
                    color: ShadcnTheme.c("muted-foreground")
                    text: model.length > 0
                          ? qsTr("在列表内滑动查看")
                          : qsTr("执行请求或连接 Legacy 后会出现日志")
                }

                ShadcnButton {
                    text: qsTr("清空")
                    variant: "outline"
                    size: "sm"
                    enabled: model.length > 0
                    onClicked: ClientBackend.clearLog()
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: root._logViewportHeight
                radius: ShadcnTheme.radiusMd
                color: ShadcnTheme.c("muted")
                opacity: 0.35
                border.color: ShadcnTheme.c("border")
                border.width: 1
                clip: true

                Item {
                    anchors.fill: parent
                    anchors.margins: 8

                    ListView {
                        id: logView
                        anchors.fill: parent
                        model: root.model
                        spacing: 8
                        clip: true
                        boundsBehavior: Flickable.StopAtBounds
                        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                        onCountChanged: Qt.callLater(function () { positionViewAtBeginning() })

                        delegate: Rectangle {
                            width: logView.width
                            radius: ShadcnTheme.radiusSm
                            color: ShadcnTheme.c("card")
                            border.width: 1
                            border.color: ShadcnTheme.c("border")
                            implicitHeight: rowLayout.implicitHeight + 16

                            readonly property string line: modelData
                            readonly property string kind: root.logKind(line)

                            Rectangle {
                                width: 3
                                anchors.left: parent.left
                                anchors.top: parent.top
                                anchors.bottom: parent.bottom
                                anchors.margins: 8
                                radius: 2
                                color: root.logAccent(kind)
                            }

                            RowLayout {
                                id: rowLayout
                                anchors.fill: parent
                                anchors.leftMargin: 14
                                anchors.rightMargin: 10
                                anchors.topMargin: 8
                                anchors.bottomMargin: 8
                                spacing: 8

                                ClientStatusBadge {
                                    text: root.logBadge(kind)
                                    tone: root.logBadgeTone(kind)
                                    Layout.alignment: Qt.AlignTop
                                }

                                ShadcnLabel {
                                    Layout.fillWidth: true
                                    wrapMode: Text.WordWrap
                                    font.pixelSize: ShadcnTypography.fontSizeXs
                                    font.family: "Geist Mono, Consolas, monospace"
                                    color: kind === "error"
                                           ? ShadcnTheme.c("destructive")
                                           : ShadcnTheme.c("foreground")
                                    text: line
                                }
                            }
                        }
                    }

                    ShadcnLabel {
                        anchors.centerIn: parent
                        visible: root.model.length === 0
                        width: parent.width - 24
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                        color: ShadcnTheme.c("muted-foreground")
                        font.pixelSize: ShadcnTypography.fontSizeSm
                        text: qsTr("暂无日志")
                    }
                }
            }
        }
    }
}

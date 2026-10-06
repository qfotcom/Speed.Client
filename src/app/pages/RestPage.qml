import QtQuick
import QtQuick.Layouts
import SenseAppShell 1.0
import SenseDesign
import "../components"
import "../components/ClientHttpStatus.js" as HttpStatus

Item {
    id: root
    property string pageId: "rest"

    width: parent && parent.width > 0 ? parent.width : 360
    implicitWidth: width
    implicitHeight: page.implicitHeight

    ClientMobilePage {
        id: page
        width: parent.width
        title: qsTr("REST 测试")
        subtitle: qsTr("请求发往 REST 端点 %1（与 Legacy 主机独立）。").arg(ClientBackend.restEndpoint)

        ShadcnCard {
            Layout.fillWidth: true
            contentMargin: ShadcnAppPlatform.isMobile ? 14 : 20

            ColumnLayout {
                width: parent.width
                spacing: 14

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    ShadcnIcon { name: "cloud"; size: 18 }
                    ShadcnLabel {
                        Layout.fillWidth: true
                        text: qsTr("HTTP 探针")
                        font.weight: ShadcnTypography.fontWeightSemibold
                    }
                    ClientStatusBadge {
                        readonly property bool _lastFailed: !ClientBackend.restBusy
                                && (HttpStatus.isHttpFailure(ClientBackend.lastRestHealth)
                                    || HttpStatus.isHttpFailure(ClientBackend.lastRestEcho))
                        readonly property bool _lastOk: !ClientBackend.restBusy
                                && (HttpStatus.isHttpSuccess(ClientBackend.lastRestHealth)
                                    || HttpStatus.isHttpSuccess(ClientBackend.lastRestEcho))
                        text: ClientBackend.restBusy
                              ? qsTr("请求中")
                              : (_lastFailed ? qsTr("失败 / 超时")
                                 : (_lastOk ? qsTr("可用") : qsTr("就绪")))
                        tone: ClientBackend.restBusy
                              ? "pending"
                              : (_lastFailed ? "danger" : (_lastOk ? "success" : "neutral"))
                    }
                }

                ShadcnSeparator {}

                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    columnSpacing: 10
                    rowSpacing: 10

                    ShadcnButton {
                        Layout.fillWidth: true
                        Layout.columnSpan: ShadcnAppPlatform.isMobile ? 2 : 1
                        text: qsTr("Health")
                        iconName: "heart-pulse"
                        enabled: !ClientBackend.restBusy
                        onClicked: ClientBackend.restHealth()
                    }

                    ShadcnButton {
                        Layout.fillWidth: true
                        Layout.columnSpan: ShadcnAppPlatform.isMobile ? 2 : 1
                        text: qsTr("Echo")
                        variant: "secondary"
                        iconName: "message-square"
                        enabled: !ClientBackend.restBusy
                        onClicked: ClientBackend.restEcho()
                    }
                }

                ShadcnLabel {
                    Layout.fillWidth: true
                    text: qsTr("/health")
                    font.pixelSize: ShadcnTypography.fontSizeXs
                    color: ShadcnTheme.c("muted-foreground")
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: healthText.implicitHeight + 20
                    radius: ShadcnTheme.radiusMd
                    color: ShadcnTheme.c("muted")
                    opacity: 0.4
                    border.color: ShadcnTheme.c("border")
                    border.width: 1

                    ShadcnLabel {
                        id: healthText
                        anchors.fill: parent
                        anchors.margins: 10
                        wrapMode: Text.WordWrap
                        font.family: "Geist Mono, Consolas, monospace"
                        font.pixelSize: ShadcnTypography.fontSizeXs
                        text: ClientBackend.lastRestHealth.length > 0
                              ? ClientBackend.lastRestHealth
                              : "—"
                    }
                }

                ShadcnLabel {
                    Layout.fillWidth: true
                    text: qsTr("/api/v1/echo")
                    font.pixelSize: ShadcnTypography.fontSizeXs
                    color: ShadcnTheme.c("muted-foreground")
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: echoText.implicitHeight + 20
                    radius: ShadcnTheme.radiusMd
                    color: ShadcnTheme.c("muted")
                    opacity: 0.4
                    border.color: ShadcnTheme.c("border")
                    border.width: 1

                    ShadcnLabel {
                        id: echoText
                        anchors.fill: parent
                        anchors.margins: 10
                        wrapMode: Text.WordWrap
                        font.family: "Geist Mono, Consolas, monospace"
                        font.pixelSize: ShadcnTypography.fontSizeXs
                        text: ClientBackend.lastRestEcho.length > 0
                              ? ClientBackend.lastRestEcho
                              : "—"
                    }
                }
            }
        }

        EventLogPanel {
            Layout.fillWidth: true
            model: ClientBackend.eventLog
            startExpanded: !ShadcnAppPlatform.isMobile
        }
    }
}

import QtQuick
import QtQuick.Layouts
import SenseAppShell 1.0
import SenseDesign
import "../components"

Item {
    id: root
    property string pageId: "legacy"

    width: parent && parent.width > 0 ? parent.width : 360
    implicitWidth: width
    implicitHeight: page.implicitHeight

    ClientMobilePage {
        id: page
        width: parent.width
        title: qsTr("Legacy 与推送")
        subtitle: qsTr("行协议 + 4 字节帧。Workflow 模式下 PUSH 随下一次响应下发；开启轮询 PING 可及时收到推送。")

        ShadcnCard {
            Layout.fillWidth: true
            contentMargin: ShadcnAppPlatform.isMobile ? 14 : 20

            ColumnLayout {
                width: parent.width
                spacing: 14

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    ShadcnIcon { name: "radio"; size: 18 }
                    ShadcnLabel {
                        Layout.fillWidth: true
                        text: qsTr("Legacy 命令")
                        font.weight: ShadcnTypography.fontWeightSemibold
                    }
                    ClientStatusBadge {
                        text: ClientBackend.legacyBusy && ClientBackend.legacyConnected
                              ? qsTr("通信中")
                              : (ClientBackend.legacyConnected ? qsTr("已连接") : qsTr("未连接"))
                        tone: ClientBackend.legacyBusy && ClientBackend.legacyConnected
                              ? "pending"
                              : (ClientBackend.legacyConnected ? "success" : "danger")
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
                        text: qsTr("PING")
                        variant: "outline"
                        iconName: "activity"
                        enabled: ClientBackend.legacyConnected && !ClientBackend.legacyBusy
                        onClicked: ClientBackend.legacyPing()
                    }

                    ShadcnButton {
                        Layout.fillWidth: true
                        text: qsTr("REQ Echo")
                        variant: "secondary"
                        iconName: "terminal"
                        enabled: ClientBackend.legacyConnected && !ClientBackend.legacyBusy
                        onClicked: ClientBackend.legacyEcho()
                    }

                    ShadcnButton {
                        Layout.fillWidth: true
                        Layout.columnSpan: ShadcnAppPlatform.isMobile ? 2 : 1
                        text: qsTr("SUB")
                        iconName: "bell"
                        enabled: ClientBackend.legacyConnected && !ClientBackend.legacyBusy
                        onClicked: ClientBackend.legacySubscribe()
                    }

                    ShadcnButton {
                        Layout.fillWidth: true
                        Layout.columnSpan: ShadcnAppPlatform.isMobile ? 2 : 1
                        text: qsTr("UNSUB")
                        variant: "ghost"
                        enabled: ClientBackend.legacyConnected && !ClientBackend.legacyBusy
                        onClicked: ClientBackend.legacyUnsubscribe()
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    ShadcnSwitch {
                        Layout.fillWidth: true
                        text: qsTr("轮询 PING（接收 PUSH）")
                        checked: ClientBackend.pushPollEnabled
                        onToggled: ClientBackend.pushPollEnabled = checked
                    }
                }

                ShadcnLabel {
                    Layout.fillWidth: true
                    text: qsTr("最近响应")
                    font.pixelSize: ShadcnTypography.fontSizeXs
                    color: ShadcnTheme.c("muted-foreground")
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: lastLine.implicitHeight + 20
                    radius: ShadcnTheme.radiusMd
                    color: ShadcnTheme.c("muted")
                    opacity: 0.4
                    border.color: ShadcnTheme.c("border")
                    border.width: 1

                    ShadcnLabel {
                        id: lastLine
                        anchors.fill: parent
                        anchors.margins: 10
                        wrapMode: Text.WordWrap
                        font.family: "Geist Mono, Consolas, monospace"
                        font.pixelSize: ShadcnTypography.fontSizeXs
                        text: ClientBackend.lastLegacyLine.length > 0
                              ? ClientBackend.lastLegacyLine
                              : "—"
                    }
                }

                ClientStatusAlert {
                    Layout.fillWidth: true
                    tone: "neutral"
                    title: qsTr("联调提示")
                    description: qsTr("服务端执行 PUB %1 <payload>；订阅并轮询 PING 后应看到 [PUSH] 日志。")
                                 .arg(ClientBackend.subscribeTopic)
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

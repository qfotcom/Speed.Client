import QtQuick
import QtQuick.Layouts
import SenseAppShell 1.0
import SenseDesign
import "../components"

Item {
    id: root
    property string pageId: "connection"

    width: parent && parent.width > 0 ? parent.width : 360
    implicitWidth: width
    implicitHeight: page.implicitHeight

    ClientMobilePage {
        id: page
        width: parent.width
        title: qsTr("服务器连接")
        subtitle: qsTr("模拟器访问本机请用 10.0.2.2；真机填写 PC 局域网 IP。服务端需监听 0.0.0.0。")

        ShadcnCard {
            Layout.fillWidth: true
            contentMargin: ShadcnAppPlatform.isMobile ? 14 : 20

            ColumnLayout {
                width: parent.width
                spacing: 12

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    ShadcnIcon { name: "server"; size: 18 }
                    ShadcnLabel {
                        Layout.fillWidth: true
                        text: qsTr("端点配置")
                        font.weight: ShadcnTypography.fontWeightSemibold
                    }
                    ClientStatusBadge {
                        text: ClientBackend.legacyBusy && ClientBackend.legacyConnected
                              ? qsTr("通信中")
                              : (ClientBackend.legacyConnected ? qsTr("Legacy 在线") : qsTr("Legacy 离线"))
                        tone: ClientBackend.legacyBusy && ClientBackend.legacyConnected
                              ? "pending"
                              : (ClientBackend.legacyConnected ? "success" : "danger")
                    }
                }

                ShadcnSeparator {}

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    ShadcnLabel { text: qsTr("主机") }
                    ShadcnInput {
                        Layout.fillWidth: true
                        placeholderText: qsTr("10.0.2.2 或 192.168.x.x")
                        text: ClientBackend.host
                        onEditingFinished: ClientBackend.host = text
                    }
                    ShadcnLabel {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        font.pixelSize: ShadcnTypography.fontSizeXs
                        color: ShadcnTheme.c("muted-foreground")
                        text: qsTr("保存后 REST 与 Legacy 共用此地址")
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: ShadcnAppPlatform.isMobile ? 10 : 14

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6
                        ShadcnLabel { text: qsTr("Legacy 端口") }
                        ShadcnInput {
                            Layout.fillWidth: true
                            text: String(ClientBackend.legacyPort)
                            inputMethodHints: Qt.ImhDigitsOnly
                            onEditingFinished: ClientBackend.legacyPort = parseInt(text) || 9001
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6
                        ShadcnLabel { text: qsTr("REST 端口") }
                        ShadcnInput {
                            Layout.fillWidth: true
                            text: String(ClientBackend.restPort)
                            inputMethodHints: Qt.ImhDigitsOnly
                            onEditingFinished: ClientBackend.restPort = parseInt(text) || 8080
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    ShadcnLabel { text: qsTr("Echo 文本") }
                    ShadcnInput {
                        Layout.fillWidth: true
                        placeholderText: qsTr("REST / Legacy echo 默认内容")
                        text: ClientBackend.echoText
                        onEditingFinished: ClientBackend.echoText = text
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    ShadcnLabel { text: qsTr("订阅 Topic") }
                    ShadcnInput {
                        Layout.fillWidth: true
                        placeholderText: qsTr("例如 quote.test")
                        text: ClientBackend.subscribeTopic
                        onEditingFinished: ClientBackend.subscribeTopic = text
                    }
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: ShadcnAppPlatform.isMobile ? 1 : 2
                    columnSpacing: 10
                    rowSpacing: 10

                    ShadcnButton {
                        Layout.fillWidth: true
                        text: qsTr("保存配置")
                        iconName: "save"
                        onClicked: ClientBackend.saveSettings()
                    }

                    ShadcnButton {
                        Layout.fillWidth: true
                        text: ClientBackend.legacyConnected ? qsTr("断开 Legacy") : qsTr("连接 Legacy")
                        variant: ClientBackend.legacyConnected ? "outline" : "default"
                        iconName: ClientBackend.legacyConnected ? "unplug" : "plug"
                        onClicked: ClientBackend.legacyConnected
                                     ? ClientBackend.disconnectLegacy()
                                     : ClientBackend.connectLegacy()
                    }
                }

                ClientStatusAlert {
                    Layout.fillWidth: true
                    tone: ClientBackend.legacyConnected ? "success" : "danger"
                    title: ClientBackend.legacyConnected
                           ? qsTr("Legacy 已连接")
                           : qsTr("Legacy 未连接")
                    description: ClientBackend.legacyConnected
                                 ? qsTr("REST 为无状态 HTTP，无需单独「连接」。")
                                 : qsTr("请检查主机、端口与服务端是否监听 0.0.0.0；超时或拒绝连接会显示为离线。")
                }
            }
        }

        EventLogPanel {
            Layout.fillWidth: true
            model: ClientBackend.eventLog
            startExpanded: true
        }
    }
}

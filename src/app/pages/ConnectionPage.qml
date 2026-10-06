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
        subtitle: qsTr("Legacy 与 REST 为两条独立通道，主机与端口分别配置（cpolar 下公网域名通常不同）。")

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

                ShadcnLabel {
                    Layout.fillWidth: true
                    text: qsTr("Legacy 通道（TCP 行协议）")
                    font.pixelSize: ShadcnTypography.fontSizeXs
                    font.weight: ShadcnTypography.fontWeightMedium
                    color: ShadcnTheme.c("muted-foreground")
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: ShadcnAppPlatform.isMobile ? 10 : 14

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        spacing: 6
                        ShadcnLabel { text: qsTr("Legacy 主机") }
                        ShadcnInput {
                            Layout.fillWidth: true
                            placeholderText: qsTr("1.tcp.cpolar.cn")
                            text: ClientBackend.legacyHost
                            onEditingFinished: ClientBackend.legacyHost = text
                        }
                    }

                    ColumnLayout {
                        Layout.preferredWidth: ShadcnAppPlatform.isMobile ? 120 : 140
                        spacing: 6
                        ShadcnLabel { text: qsTr("Legacy 端口") }
                        ShadcnInput {
                            Layout.fillWidth: true
                            text: String(ClientBackend.legacyPort)
                            inputMethodHints: Qt.ImhDigitsOnly
                            onEditingFinished: ClientBackend.legacyPort = parseInt(text) || 20771
                        }
                    }
                }

                ShadcnLabel {
                    Layout.fillWidth: true
                    font.pixelSize: ShadcnTypography.fontSizeXs
                    color: ShadcnTheme.c("muted-foreground")
                    text: qsTr("Legacy 当前：%1").arg(ClientBackend.legacyEndpoint)
                }

                ShadcnSeparator {}

                ShadcnLabel {
                    Layout.fillWidth: true
                    text: qsTr("REST 通道（HTTP）")
                    font.pixelSize: ShadcnTypography.fontSizeXs
                    font.weight: ShadcnTypography.fontWeightMedium
                    color: ShadcnTheme.c("muted-foreground")
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: ShadcnAppPlatform.isMobile ? 10 : 14

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        spacing: 6
                        ShadcnLabel { text: qsTr("REST 主机") }
                        ShadcnInput {
                            Layout.fillWidth: true
                            placeholderText: qsTr("6.tcp.cpolar.cn")
                            text: ClientBackend.restHost
                            onEditingFinished: ClientBackend.restHost = text
                        }
                    }

                    ColumnLayout {
                        Layout.preferredWidth: ShadcnAppPlatform.isMobile ? 120 : 140
                        spacing: 6
                        ShadcnLabel { text: qsTr("REST 端口") }
                        ShadcnInput {
                            Layout.fillWidth: true
                            text: String(ClientBackend.restPort)
                            inputMethodHints: Qt.ImhDigitsOnly
                            onEditingFinished: ClientBackend.restPort = parseInt(text) || 10513
                        }
                    }
                }

                ShadcnLabel {
                    Layout.fillWidth: true
                    font.pixelSize: ShadcnTypography.fontSizeXs
                    color: ShadcnTheme.c("muted-foreground")
                    text: qsTr("REST 当前：%1").arg(ClientBackend.restEndpoint)
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    ShadcnSwitch {
                        checked: ClientBackend.autoConnectOnStartup
                        onToggled: ClientBackend.autoConnectOnStartup = checked
                    }
                    ShadcnLabel {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        text: qsTr("启动时自动连接 Legacy，并探活 REST /health；断线后约 8 秒自动重连")
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
                        text: qsTr("cpolar 默认")
                        variant: "outline"
                        iconName: "globe"
                        onClicked: ClientBackend.applyCpolarDefaults()
                    }

                    ShadcnButton {
                        Layout.fillWidth: true
                        text: qsTr("立即连接两通道")
                        iconName: "plug"
                        onClicked: {
                            ClientBackend.connectLegacy()
                            ClientBackend.restHealth()
                        }
                    }

                    ShadcnButton {
                        Layout.fillWidth: true
                        text: ClientBackend.legacyConnected ? qsTr("断开 Legacy") : qsTr("仅连 Legacy")
                        variant: "outline"
                        iconName: ClientBackend.legacyConnected ? "unplug" : "plug"
                        onClicked: ClientBackend.legacyConnected
                                     ? ClientBackend.disconnectLegacy()
                                     : ClientBackend.connectLegacy()
                    }
                }

                ShadcnLabel {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    font.pixelSize: ShadcnTypography.fontSizeXs
                    color: ShadcnTheme.c("muted-foreground")
                    text: qsTr("启动后自动连 Legacy 并探活 REST。局域网联调：两路主机填同一 PC IP，端口 9001 / 8080。")
                }

                ClientStatusAlert {
                    Layout.fillWidth: true
                    tone: ClientBackend.legacyConnected ? "success" : "danger"
                    title: ClientBackend.legacyConnected
                           ? qsTr("Legacy 已连接")
                           : qsTr("Legacy 未连接")
                    description: ClientBackend.legacyConnected
                                 ? qsTr("REST 走 %1，与 Legacy 主机无关。").arg(ClientBackend.restEndpoint)
                                 : qsTr("请检查两路主机/端口；若仍显示同一旧地址，点「cpolar 默认」。")
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

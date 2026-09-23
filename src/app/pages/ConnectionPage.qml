import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import SenseDesign 1.0
import "../components"

ClientPageLayout {
    title: "连接"
    description: "配置 Speed.Server 地址。模拟器访问本机 PC 请用 10.0.2.2；真机用电脑局域网 IP。服务端需监听 0.0.0.0。"

    SenseCard {
        Layout.fillWidth: true
        title: "服务器"
        iconName: "server"

        body: Component {
            ColumnLayout {
                width: parent ? parent.width : 360
                spacing: SenseSpacing.spacer3

                SenseInput {
                    Layout.fillWidth: true
                    label: "主机"
                    placeholder: "10.0.2.2 或 192.168.x.x"
                    text: ClientBackend.host
                    onInputFocusChanged: function (focused) {
                        if (!focused)
                            ClientBackend.host = text
                    }
                    onAccepted: ClientBackend.host = text
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: SenseSpacing.spacer3

                    SenseInput {
                        Layout.fillWidth: true
                        label: "Legacy 端口"
                        text: String(ClientBackend.legacyPort)
                        textField.inputMethodHints: Qt.ImhDigitsOnly
                        onInputFocusChanged: function (focused) {
                            if (!focused)
                                ClientBackend.legacyPort = parseInt(text) || 9001
                        }
                        onAccepted: ClientBackend.legacyPort = parseInt(text) || 9001
                    }
                    SenseInput {
                        Layout.fillWidth: true
                        label: "REST 端口"
                        text: String(ClientBackend.restPort)
                        textField.inputMethodHints: Qt.ImhDigitsOnly
                        onInputFocusChanged: function (focused) {
                            if (!focused)
                                ClientBackend.restPort = parseInt(text) || 8080
                        }
                        onAccepted: ClientBackend.restPort = parseInt(text) || 8080
                    }
                }

                SenseInput {
                    Layout.fillWidth: true
                    label: "Echo 文本"
                    text: ClientBackend.echoText
                    onInputFocusChanged: function (focused) {
                        if (!focused)
                            ClientBackend.echoText = text
                    }
                    onAccepted: ClientBackend.echoText = text
                }

                SenseInput {
                    Layout.fillWidth: true
                    label: "订阅 Topic"
                    text: ClientBackend.subscribeTopic
                    onInputFocusChanged: function (focused) {
                        if (!focused)
                            ClientBackend.subscribeTopic = text
                    }
                    onAccepted: ClientBackend.subscribeTopic = text
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: SenseSpacing.spacer3

                    SenseButton {
                        text: "保存配置"
                        variant: "primary"
                        iconLeft: "floppy-disk"
                        Layout.fillWidth: true
                        onClicked: ClientBackend.saveSettings()
                    }
                    SenseButton {
                        text: ClientBackend.legacyConnected ? "断开 Legacy" : "连接 Legacy"
                        variant: ClientBackend.legacyConnected ? "outline" : "secondary"
                        iconLeft: ClientBackend.legacyConnected ? "plug-circle-xmark" : "plug-circle-check"
                        Layout.fillWidth: true
                        onClicked: ClientBackend.legacyConnected
                                     ? ClientBackend.disconnectLegacy()
                                     : ClientBackend.connectLegacy()
                    }
                }

                SenseAlert {
                    Layout.fillWidth: true
                    type: ClientBackend.legacyConnected ? "success" : "info"
                    title: ClientBackend.legacyConnected ? "Legacy 已连接" : "Legacy 未连接"
                    description: "REST 为无状态 HTTP，无需单独连接。"
                }
            }
        }
    }

    EventLogPanel { model: ClientBackend.eventLog }
}
